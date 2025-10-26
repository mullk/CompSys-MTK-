#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "job_queue.h"

int job_queue_init(struct job_queue *q, int capacity) {
    q->buffer = malloc(sizeof(void*) * capacity);   // Allokerer dynamisk et array af void-pegepinde*, der skal holde vores jobs
    if (q->buffer == NULL) return -1;               // Tjekker om malloc fejlede (f.eks. hvis systemet ikke havde nok hukommelse).

    q->capacity = capacity;
    q->count = 0;
    q->front = 0;
    q->rear = 0;
    q->destroyed = 0;

    pthread_mutex_init(&q->push_lock, NULL);
    pthread_mutex_init(&q->pop_lock, NULL);
    pthread_mutex_init(&q->count_lock, NULL);

    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return 0;

// Initialiserer to condition variables:
// not_full: signalerer når køen ikke længere er fuld (så en producer kan tilføje igen).
// not_empty: signalerer når køen ikke længere er tom (så en consumer kan tage et job).
// Disse bruges i push() og pop() sammen med pthread_cond_wait() og pthread_cond_signal().
}






int job_queue_push(struct job_queue *q, void *data) {
    assert(q != NULL);                // assert(q != NULL) beskytter mod programmeringsfejl — vi må aldrig kalde funktionen med en null-pointer.
    pthread_mutex_lock(&q->push_lock);    // pthread_mutex_lock låser køen, så kun én tråd ad gangen må ændre dens indre variabler (count, rear, buffer osv.).

    // Vent hvis fuld
    while (1) {
        pthread_mutex_lock(&q->count_lock);
        int full = (q->count == q->capacity);
        int dead = q->destroyed;
        pthread_mutex_unlock(&q->count_lock);

        if (dead) {
            pthread_mutex_unlock(&q->push_lock);
            return -1;
        }
        if (!full) break;

        pthread_cond_wait(&q->not_full, &q->push_lock);
    }

    q->buffer[q->rear] = data;
    q->rear = (q->rear + 1) % q->capacity;

    pthread_mutex_lock(&q->count_lock);
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->count_lock);

    pthread_mutex_unlock(&q->push_lock);
    return 0;

// Når vi har tilføjet et element, kan en ventende consumer-tråd (som sidder fast i job_queue_pop()) nu vækkes, fordi køen ikke længere er tom.
// pthread_cond_signal(&q->not_empty) vækker én af de tråde, der venter på at kunne hente et job.
// Derefter frigiver vi låsen (pthread_mutex_unlock) så andre tråde kan tilgå køen. return 0 signalerer succes.
}






int job_queue_pop(struct job_queue *q, void **data) {
    assert(q != NULL); // Tjekker at køen ikke er NULL. God praksis — forhindrer segmentation faults.
    pthread_mutex_lock(&q->pop_lock); // Låser køen, så kun én tråd ad gangen kan ændre dens interne tilstand (front, rear, count, osv.).

    while (1) {
        pthread_mutex_lock(&q->count_lock);
        int empty = (q->count == 0);
        int dead = q->destroyed;
        pthread_mutex_unlock(&q->count_lock);

        if (dead && empty) {
            pthread_mutex_unlock(&q->pop_lock);
            return -1;
        }
        if (!empty) break;

        pthread_cond_wait(&q->not_empty, &q->pop_lock);
    }


    *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;

    pthread_mutex_lock(&q->count_lock);
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->count_lock);

    pthread_mutex_unlock(&q->pop_lock);
    return 0;
}





int job_queue_destroy(struct job_queue *q) {
    pthread_mutex_lock(&q->count_lock);
    q->destroyed = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->count_lock);

    while (1) {
        pthread_mutex_lock(&q->count_lock);
        int remaining = q->count;
        pthread_mutex_unlock(&q->count_lock);

        if (remaining == 0) break;
        sched_yield();
    }

    pthread_mutex_destroy(&q->push_lock);
    pthread_mutex_destroy(&q->pop_lock);
    pthread_mutex_destroy(&q->count_lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
    free(q->buffer);
    return 0;
}


// Til sidst rydder vi ordentligt op — og rækkefølgen er rigtig:
// Her ødelægges synkroniseringsprimitive, efter at ingen bruger dem længere.
// Derefter frigiver vi hukommelsen.