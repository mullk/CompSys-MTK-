// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "job_queue.h"

#include <err.h>
#include "histogram.h"

// Globalt histogram og mutex — fælles for alle tråde
// Vi har dette uden for main for at gøre det lettere for worker-tråde at få adgang til det.
// Alle workers skal have adgang til den samme globale histogram og lås.
int global_histogram[8] = {0};
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;

// err.h contains various nonstandard BSD extensions, but they are
// very handy.



void* worker(void *arg) {
    struct job_queue *jq = arg;
    char *path;

    while (1) {
        // Pop job fra køen (blokkerer hvis tom)
        if (job_queue_pop(jq, (void**)&path) != 0) {
            break; // Køen er lukket → afslut tråden
        }

        // --- Lokalt arbejde ---
        int local_hist[8] = {0};
        FILE *f = fopen(path, "r");
        if (f == NULL) {
            warn("failed to open %s", path);
            free(path);
            continue;
        }

        unsigned char c;
        int count = 0;
        while (fread(&c, sizeof(c), 1, f) == 1) {
            update_histogram(local_hist, c);
            count++;

            if (count % 100000 == 0) {
                pthread_mutex_lock(&global_lock);
                merge_histogram(local_hist, global_histogram);
                print_histogram(global_histogram);
                pthread_mutex_unlock(&global_lock);
            }
        }

        fclose(f);

        // Flet det sidste restresultat ind globalt
        pthread_mutex_lock(&global_lock);
        merge_histogram(local_hist, global_histogram);
        print_histogram(global_histogram);
        pthread_mutex_unlock(&global_lock);

        free(path);
    }

    return NULL;
}







int main(int argc, char * const *argv) {
    if (argc < 2) {
        err(1, "usage: paths...");
        exit(1);
    }

    int num_threads = 1;
    char * const *paths = &argv[1];

    // Håndter -n flag
    if (argc > 3 && strcmp(argv[1], "-n") == 0) {
        num_threads = atoi(argv[2]);
        if (num_threads < 1) err(1, "invalid thread count: %s", argv[2]);
        paths = &argv[3];
    }

    // --- Init job queue ---
    struct job_queue jq;
    job_queue_init(&jq, 64);

    // --- Start worker-tråde ---
    pthread_t *threads = calloc(num_threads, sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker, &jq) != 0)
            err(1, "pthread_create() failed");
    }

    // --- Læs alle filer og push til køen ---
    int fts_options = FTS_LOGICAL | FTS_NOCHDIR;
    FTS *ftsp;
    if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL)
        err(1, "fts_open() failed");

    FTSENT *p;
    while ((p = fts_read(ftsp)) != NULL) {
        if (p->fts_info == FTS_F) {
            job_queue_push(&jq, strdup(p->fts_path)); // Brug strdup()
        }
    }

    fts_close(ftsp);

    // --- Luk køen og vent på threads ---
    job_queue_destroy(&jq);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    move_lines(9);
    return 0;
}

