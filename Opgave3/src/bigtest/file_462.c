// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>

#include <pthread.h> 

#include "job_queue.h"

//funktionen "fauxgrep_file" kaldes i 
// "worker_function", hvorfor den er kopieret ind her.
int fauxgrep_file(char const *needle, char const *path) {
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    warn("failed to open %s", path);
    return -1;
  }
  char *line = NULL;
  size_t linelen = 0;
  int lineno = 1;
  while (getline(&line, &linelen, f) != -1) {
    if (strstr(line, needle) != NULL) {
      printf("%s:%d: %s", path, lineno, line);
    }
    lineno++;
  }
  free(line);
  fclose(f);
  return 0;
}

const char *needle;

void initializeJobQueue(struct job_queue* jobs) {
    int capacity = 128; 
    if (job_queue_init(jobs, capacity)!= 0) {
        printf("Failed to initialize job queue\n");
        exit(1); 
    }
}

// Nye hjælpefunktioner: 
void* worker_function(void *arg) {
    struct job_queue *jobs = arg;   
    char *filepath;
    while (job_queue_pop(jobs, (void**)&filepath) == 0) {
        fauxgrep_file(needle, filepath); 
        free(filepath);                  
    }
    return NULL; //Hvis ingen værdi returneres.
}//https://www.geeksforgeeks.org/c/thread-functions-in-c-c/

int main(int argc, char * const *argv) {  
  if (argc < 2) {
    err(1, "usage: [-n INT] STRING paths...");
    exit(1);
  }

  int num_threads = 1;
  needle = argv[1]; 
  //char const *needle = argv[1];
  char * const *paths = &argv[2];


  if (argc > 3 && strcmp(argv[1], "-n") == 0) {
    // Since atoi() simply returns zero on syntax errors, we cannot
    // distinguish between the user entering a zero, or some
    // non-numeric garbage.  In fact, we cannot even tell whether the
    // given option is suffixed by garbage, i.e. '123foo' returns
    // '123'.  A more robust solution would use strtol(), but its
    // interface is more complicated, so here we are.
    num_threads = atoi(argv[2]);

    if (num_threads < 1) {
      err(1, "invalid thread count: %s", argv[2]);
    }

    needle = argv[3];
    paths = &argv[4];

  } else {
    needle = argv[1];
    paths = &argv[2];
  }

  //assert(0); // Initialise the job queue and some worker threads here:
  struct job_queue jobs;
  initializeJobQueue(&jobs);
  pthread_t threads[num_threads];
  for (int i = 0; i < num_threads; i++) {
    pthread_create(&threads[i], NULL, worker_function, &jobs);
  }

  // FTS_LOGICAL = follow symbolic links
  // FTS_NOCHDIR = do not change the working directory of the process
  //
  // (These are not particularly important distinctions for our simple
  // uses.)
  int fts_options = FTS_LOGICAL | FTS_NOCHDIR;

  FTS *ftsp;
  if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL) {
    err(1, "fts_open() failed");
    return -1;
  }

  FTSENT *p;
  while ((p = fts_read(ftsp)) != NULL) {
    switch (p->fts_info) {
    case FTS_D:
      break;
    case FTS_F: {
      //assert(0); Process the file p->fts_path, somehow.
        char *filepath = strdup(p->fts_path);
        if (filepath == NULL) {
          printf("Failed to find file path\n");
        continue;
        } //Filer sendes til job-køen
      job_queue_push(&jobs, filepath); }
      break;
    default:
      break;
    }
  }

  fts_close(ftsp);
  //assert(0); Shut down the job queue and the worker threads here.
  job_queue_destroy(&jobs);
  //Kalder "destroy" fra job_queue.h
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  
  return 0;
}
