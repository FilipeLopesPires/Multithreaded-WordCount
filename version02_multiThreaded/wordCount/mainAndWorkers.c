
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "probConst.h"
#include "textProc.h"

/** \brief worker life cycle routine */
static void *workers(void *id);

/** \brief worker threads return status array */
int statusWorker[N];

/** \brief main thread return status value */
int statusMain;

int main(void) {
    pthread_t tIdWorker[N];
    unsigned int worker[N];
    int i;         /* counting variable */
    int *status_p; /* pointer to execution status */

    /* initializing the application defined thread id arrays for the workers
       and the consumers and the random number generator */

    double t0, t1; /* time limits */

    for (i = 0; i < N; i++) worker[i] = i;
    srandom((unsigned int)getpid());
    t0 = ((double)clock()) / CLOCKS_PER_SEC;

    /* generation of intervening entities threads */

    for (i = 0; i < N; i++)
        if (pthread_create(&tIdWorker[i], NULL, worker, &worker[i]) !=
            0) /* thread worker */
        {
            perror("error on creating thread worker");
            exit(EXIT_FAILURE);
        }

    /* waiting for the termination of the intervening entities threads */

    printf("\nFinal report\n");
    for (i = 0; i < N; i++) {
        if (pthread_join(tIdWorker[i], (void *)&status_p) !=
            0) /* thread worker */
        {
            perror("error on waiting for thread worker");
            exit(EXIT_FAILURE);
        }
        printf("thread worker, with id %u, has terminated: ", i);
        printf("its status was %d\n", *status_p);
    }
    t1 = ((double)clock()) / CLOCKS_PER_SEC;
    printf("\nElapsed time = %.6f s\n", t1 - t0);

    exit(EXIT_SUCCESS);
}

static void *worker(void *par) {
    unsigned int id = *((unsigned int *)par), /* worker id */
        val;                                  /* produced value */
    int i;                                    /* counting variable */

    for (i = 0; i < M; i++) {
        val = 1000 * id + i; /* produce a value */
        putVal(id, val);     /* store a value */
        usleep((unsigned int)floor(40.0 * random() / RAND_MAX +
                                   1.5)); /* do something else */
    }

    statusWorker[id] = EXIT_SUCCESS;
    pthread_exit(&statusWorker[id]);
}
