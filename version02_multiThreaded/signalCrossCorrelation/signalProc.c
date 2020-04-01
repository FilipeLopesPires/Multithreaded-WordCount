/**
 *  \file textProc.c (implementation file)
 *
 *  \brief Program that reads in succession several text files and prints a
 *  listing of the occurring frequency of word lengths and the number of vowels
 *  in each word for each of the supplied texts.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which
 *  enables the creation of a monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the workers:
 *     \li getTextChunk
 *     \li savePartialResults
 *     \li presentFilenames
 *     \li printResults.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "results.h"
#include "signal.h"
#include "signalCrossCorrelation.h"

/** \brief worker threads return status array */
extern int statusWorker[NUMWORKERS];

/** \brief main thread return status value */
extern int statusMain;

/** \brief boolean defining wether files have been read and monitor is ready for
 * use or not */
bool areFilenamesPresented;

pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

pthread_once_t init = PTHREAD_ONCE_INIT;

pthread_cond_t filenamesPresented;

FILE** files;
char** filenames;
int filesSize;
int currentFileIdx;
double** results;
int nElem;
int currentTau;
double* currentFile[2];
int* signalSizes;
double curSig;
bool stillExistsText;

void initialization(void) {
    currentFileIdx = 0;
    currentTau = 0;
    currentFile[0] = malloc(1);
    currentFile[1] = malloc(1);

    pthread_cond_init(&filenamesPresented, NULL);
    printf("Monitor initialized.\n");
}

bool getSignalAndTau(int workerId, struct signal* signal,
                     struct results* results) {
    // Enter monitor
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) != 0) {
        errno = statusWorker[workerId];
        perror("Error on entering monitor(CF).\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    pthread_once(&init, initialization);

    // Wait for files to be read
    while (!areFilenamesPresented) {
        if ((statusWorker[workerId] =
                 pthread_cond_wait(&filenamesPresented, &accessCR)) != 0) {
            errno = statusWorker[workerId];
            perror("Error on waiting in fifoFull.\n");
            statusWorker[workerId] = EXIT_FAILURE;
            pthread_exit(&statusWorker[workerId]);
        }
    }

    stillExistsText = false;
    if (currentFileIdx < filesSize) {
        if (currentTau == 0) {
            free(currentFile[0]);
            free(currentFile[1]);

            for (int n = 0; n < 2; n++) {
                if ((currentFile[n] = malloc(signalSizes[currentFileIdx] *
                                             sizeof(double))) == NULL) {
                    // malloc error message
                    perror(
                        "Error while allocating memory in presentFilenames.\n");
                    statusWorker[workerId] = EXIT_FAILURE;
                    pthread_exit(&statusWorker[workerId]);
                }
                for (int i = 0; i < signalSizes[currentFileIdx]; i++) {
                    fread(&curSig, 8, 1, files[currentFileIdx]);
                    currentFile[n][i] = curSig;
                }
            }
        }

        if (currentTau < signalSizes[currentFileIdx]) {
            results->fileId = currentFileIdx;
            results->tau = currentTau;
            signal->tau = currentTau;
            signal->values = currentFile;
            signal->signalSize = signalSizes[currentFileIdx];
            currentTau++;
            // printf("%d ", currentTau);
        } else {
            currentTau = 0;
            currentFileIdx++;
        }
        stillExistsText = true;
    }

    // Leave monitor
    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) != 0) {
        errno = statusWorker[workerId];
        perror("Error on exiting monitor(CF).\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    return stillExistsText;
}

void savePartialResults(int workerId, struct results* res) {
    // Enter monitor
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) != 0) {
        errno = statusWorker[workerId];
        perror("Error on entering monitor(CF).\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    pthread_once(&init, initialization);

    results[res->fileId][res->tau] = res->value;

    // Leave monitor
    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) != 0) {
        errno = statusWorker[workerId];
        perror("Error on exiting monitor(CF).\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
}

void presentFilenames(int size, char** fileNames) {
    // Enter monitor
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on entering monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
    pthread_once(&init, initialization);

    if (size > 0) {
        filesSize = size;
        filenames = fileNames;

        // Allocate memory
        if ((files = malloc(sizeof(FILE*) * (filesSize))) == NULL) {
            errno = statusMain;
            perror("Error while allocating memory in presentFilenames.\n");
            statusMain = EXIT_FAILURE;
            pthread_exit(&statusMain);
        }
        if ((results = malloc(sizeof(double*) * (filesSize))) == NULL) {
            errno = statusMain;
            perror("Error while allocating memory in presentFilenames.\n");
            statusMain = EXIT_FAILURE;
            pthread_exit(&statusMain);
        }

        if ((signalSizes = malloc(sizeof(int) * (filesSize))) == NULL) {
            errno = statusMain;
            perror("Error while allocating memory in presentFilenames.\n");
            statusMain = EXIT_FAILURE;
            pthread_exit(&statusMain);
        }

        // Process files given as input
        for (int i = 0; i < size; i++) {
            files[i] = fopen(filenames[i], "r+");
            if (files[i] == NULL) {
                // end of file error
                printf("Error while opening file!");
                exit(1);
            }

            fread(&nElem, 4, 1, files[i]);
            signalSizes[i] = nElem;

            if ((results[i] = malloc(sizeof(double) * (nElem))) == NULL) {
                errno = statusMain; /* save error in errno */
                perror("Error while allocating memory in presentFilenames.\n");
                statusMain = EXIT_FAILURE;
                pthread_exit(&statusMain);
            }

            for (int l = 0; l < nElem; l++) {
                results[i][l] = 0;
            }
        }
        areFilenamesPresented = true;
        printf("Files presented.\n");

        // Signal all that files have been read
        if ((statusMain = pthread_cond_signal(&filenamesPresented)) != 0) {
            errno = statusMain;
            perror("Error on signaling in fifoEmpty.\n");
            statusMain = EXIT_FAILURE;
            pthread_exit(&statusMain);
        }
    }

    // Leave monitor
    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on exiting monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
}

void writeOrPrintResults(bool write) {
    // Enter monitor
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on entering monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
    pthread_once(&init, initialization);

    for (int i = 0; i < filesSize; i++) {
        printf("Processing file %s\n", filenames[i]);

        int numErrors = 0;
        if (!write) {
            printf("Comparing one by one:\n");
        }
        for (int j = 0; j < signalSizes[i]; j++) {
            if (write) {
                fwrite(&results[i][j], 8, 1, files[i]);
            } else {
                fread(&curSig, 8, 1, files[i]);
                if (curSig == results[i][j]) {
                    printf("1");
                } else {
                    numErrors++;
                    printf("0");
                }
            }
        }
        if (!write) {
            printf("\nNum. Errors Found: %d\n", numErrors);
        }
    }

    // Leave monitor
    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on exiting monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
}

void destroy(void) {
    // Enter monitor
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on entering monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
    pthread_once(&init, initialization);

    // Free allocated memory
    free(files);
    free(results);
    printf("Monitor destroyed.\n");

    // Leave monitor
    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on exiting monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
}
