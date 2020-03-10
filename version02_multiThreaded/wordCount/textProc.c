#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "probConst.h"

char delimeters[25][MAXCHARSIZE] = {
    " ", "-", "–", "—",  ".",  ",",  ":",  ";", "(", ")", "[", "]", "{",
    "}", "?", "!", "\n", "\t", "\r", "\"", "“", "”", "«", "»", "…"};

char textBuffer[1000];
char tmpWord[50];

/** \brief worker threads return status array */
extern int statusWorker[N];

/** \brief main thread return status value */
extern int statusMain;

pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t filenamesPresented;

char* getTextChunk(int workerId) {
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
void savePartialResults(int workerId, int* wordCount, int* vowelCount) {
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
void presentFilenames(char* filenames) {
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    // TODO   ...................................

    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
}
void printResults() {
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    // TODO   ...................................

    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
}
