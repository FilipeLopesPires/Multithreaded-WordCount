/**
 *  \file fifo.c (implementation file)
 *
 *  \brief Problem name: Producers / Consumers.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which
 * enables the creation of a monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the producers / consumers:
 *     \li putVal
 *     \li getVal.
 *
 *  \author António Rui Borges - March 2019
 */

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "probConst.h"

/** \brief producer threads return status array */
extern int statusWorker[N];

pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

char* getTextChunk() {
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) !=
        0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    // TODO   ...................................

    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) !=
        0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    return;
}
void savePartialResults(int*, int*) {
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) !=
        0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    // TODO   ...................................

    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) !=
        0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
}
void presentFileNames(char*) {
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) !=
        0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    // TODO   ...................................

    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) !=
        0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
}
void printResults() {
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) !=
        0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    // TODO   ...................................

    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) !=
        0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
}
