/**
 *  \file signalProc.c (implementation file)
 *
 *  \brief Cross Correlation Problem Data transfer region implemented as a
 * monitor.
 *
 *  The program 'signalCrossCorrelation' deploys worker threads that read in
 * succession the values of pairs of signals stored in several data files whose
 * names are provided, compute the circular cross-correlation of each pair and
 * append it to the corresponding file. Threads synchronization is based on
 * monitors. Both threads and the monitor are implemented using the pthread
 * library which enables the creation of a monitor of the Lampson / Redell type.
 *  Definition of the operations carried out by the workers:
 *     \li getSignalAndTau
 *     \li savePartialResults
 *     \li presentFilenames
 *     \li writeOrPrintResults.
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

/**
 *  \brief Monitor initialization.
 *
 *  Monitor conditions and variables are initialized.
 *
 */
void initialization(void) {
    currentFileIdx = 0;
    currentTau = -1;

    pthread_cond_init(&filenamesPresented, NULL);
    printf("Monitor initialized.\n");
}

/**
 *  \brief Retrieval of the signals of a file and a given tau value.
 *
 *  Monitor retrieves the signal values of a file and assigns its processing
 * with a given tau value for the worker that called the method.
 *
 *  \param workerId internal worker thread identifier.
 *  \param signal structure containing the signal and tau values of a given
 * file. \param results structure to store the cross correlation results for a
 * given file.
 *
 */
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

    bool stillExistsText;
    if (currentFileIdx < filesSize) {
        if (currentTau == -1) {
            for (int n = 0; n < 2; n++) {
                if ((currentFile[n] = malloc(signalSizes[currentFileIdx] *
                                             sizeof(double))) == NULL) {
                    // malloc error message
                    perror(
                        "Error while allocating memory in getSignalAndTau.\n");
                    statusWorker[workerId] = EXIT_FAILURE;
                    pthread_exit(&statusWorker[workerId]);
                }
                for (int i = 0; i < signalSizes[currentFileIdx]; i++) {
                    fread(&curSig, 8, 1, files[currentFileIdx]);
                    currentFile[n][i] = curSig;
                }
            }
        }

        results->fileId = currentFileIdx;
        signal->values[0] = currentFile[0];
        signal->values[1] = currentFile[1];
        signal->signalSize = signalSizes[currentFileIdx];

        currentTau++;
        results->tau = currentTau;
        signal->tau = currentTau;
        if (currentTau > signalSizes[currentFileIdx]) {
            currentTau = -1;
            currentFileIdx++;
        }
    }

    if (currentFileIdx >= filesSize) {
        stillExistsText = false;
    } else {
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

/**
 *  \brief Update of global results.
 *
 *  Monitor updates the global results with the results achieved by the
 * worker that called the method.
 *
 *  \param workerId internal worker thread identifier.
 *  \param results structure containing the cross correlation results for a
 * given file.
 *
 */
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

/**
 *  \brief Presentation of all the files to be processed.
 *
 *  Monitor finds the files to be processed through their paths and opens
 * them for processing.
 *
 *  \param size number of files to be presented.
 *  \param fileNames array containing the paths to the files.
 *
 */
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
                printf("Error while opening file!\n");
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

/**
 *  \brief Presentation of the global results achieved by all worker
 * threads.
 *
 *  Monitor prints in a formatted form the results of the
 * 'signalCrossCorrelation' program execution.
 *
 *  \param write boolean variable that tells if the results are to be
 * written to a file or simply printed into the terminal.
 *
 */
void writeOrPrintResults(bool show) {
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
        if (show) {
            printf("Comparing one by one:\n");
        }
        for (int j = 0; j < signalSizes[i]; j++) {
            if (!show) {
                fwrite(&results[i][j], 8, 1, files[i]);
            }
            // else {
            //     fread(&curSig, 8, 1, files[i]);
            //     if (curSig == results[i][j]) {
            //         printf("1");
            //     } else {
            //         numErrors++;
            //         printf("0");
            //         printf(" %d ", j);
            //     }
            // }
        }
        if (show) {
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

/**
 *  \brief Destruction of monitor variables.
 *
 *  Monitor frees memory allocated for its variables.
 *
 */
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
    free(signalSizes);
    printf("Monitor destroyed.\n");

    // Leave monitor
    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on exiting monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
}
