/**
 *  \file textProc.c (implementation file)
 *
 *  \brief Word Count Problem Data transfer region implemented as a monitor.
 *
 *  The program 'wordCount' deploys worker threads that read in succession several text files text#.txt and calculate the occurring frequency of word lengths and the number of vowels in each word for each of the supplied texts, in the end the program prints the results achieved by its workers.
 *  Threads synchronization is based on monitors. Both threads and the monitor are implemented using the pthread library which enables the creation of a monitor of the Lampson / Redell type.
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

#include "controlInfo.h"
#include "wordCount.h"

/** \brief array containing all the characters defined as word delimiters */
char delimiters[25][MAXCHARSIZE] = {
    " ", "-", "–", "—",  ".",  ",",  ":",  ";", "(", ")", "[", "]", "{",
    "}", "?", "!", "\n", "\t", "\r", "\"", "“", "”", "«", "»", "…"};

/** \brief worker threads return status array */
extern int statusWorker[NUMWORKERS];

/** \brief main thread return status value */
extern int statusMain;

/** \brief boolean defining wether files have been read and monitor is ready for use or not */
bool areFilenamesPresented;

pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

pthread_once_t init = PTHREAD_ONCE_INIT;

pthread_cond_t filenamesPresented;

char tmpWord[MAXSIZE];
FILE** files;
char** filenames;
int** wordSizeResults;
int*** vowelCountResults;
int* numberWordsResults;
int* maximumSizeWordResults;
int* minimumSizeWordResults;
int filesSize;
int currentFileIdx = 0;
char symbol;
char completeSymbol[MAXCHARSIZE];  // buffer for complex character construction
int ones;
bool incrementFileIdx;
bool stillExistsText;

/** 
 *  \brief Monitor initialization.
 * 
 *  Monitor conditions and variables are initialized.
 * 
 */
void initialization(void) {
    strcpy(tmpWord, "");
    incrementFileIdx = false;

    pthread_cond_init(&filenamesPresented, NULL);
    printf("Monitor initialized.\n");
}

/** 
 *  \brief Retrieval of a portion of text (called text chunk).
 * 
 *  Monitor retrieves a portion of text (called text chunk) and assigns its processing for the worker that called the method.
 * 
 *  \param workerId internal worker thread identifier.
 *  \param textChunk portion of text to be processed by the worker.
 *  \param controlInfo structure containing control variables regarding the results of the worker.
 * 
 */
bool getTextChunk(int workerId, char* textChunk, struct controlInfo controlInfo) {
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

    strcat(textChunk, tmpWord);
    strcpy(tmpWord, "");
    if (currentFileIdx < filesSize) {
        while (strlen(textChunk) < BUFFERSIZE) {
            symbol = getc(files[currentFileIdx]);

            // Verify if the current file has ended
            if (symbol == EOF) {
                if (strlen(tmpWord) + strlen(textChunk) < BUFFERSIZE) {
                    strcat(textChunk, tmpWord);
                    strcpy(tmpWord, "");
                }
                incrementFileIdx = true;
                break;
            }
            strcpy(completeSymbol, "");
            ones = 0;

            // Verify the number of 1s in the most significant bits
            for (int i = sizeof(symbol) * CHAR_BIT - 1; i >= 0; --i) {
                int bit = (symbol >> i) & 1;
                if (bit == 0) {
                    break;
                }
                ones++;
            }

            // Build the complete character (if it consists of more than 1byte)
            strncat(completeSymbol, &symbol, 1);
            for (int i = 1; i < ones; i++) {
                symbol = getc(files[currentFileIdx]);
                strncat(completeSymbol, &symbol, 1);
            }

            // Check if character is a delimiter
            bool leaveLoop = false;
            for (int i = 0; i < sizeof(delimiters) / sizeof(delimiters[0]);
                 i++) {
                if (strcmp(completeSymbol, delimiters[i]) == 0) {
                    if (strlen(tmpWord) + strlen(textChunk) < BUFFERSIZE) {
                        strcat(textChunk, tmpWord);
                        strcpy(tmpWord, "");
                    } else {
                        leaveLoop = true;
                    }
                    break;
                }
            }

            strcat(tmpWord, completeSymbol);
            if (leaveLoop) {
                break;
            }
        }
    }
    controlInfo.fileId = currentFileIdx;
    if (strlen(textChunk) <= 0) {
        stillExistsText = false;
    } else {
        stillExistsText = true;
    }

    if (incrementFileIdx) {
        currentFileIdx++;
        incrementFileIdx = false;
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
 *  Monitor updates the global results with the results achieved by the worker that called the method.
 * 
 *  \param workerId internal worker thread identifier.
 *  \param controlInfo structure containing control variables regarding the results of the worker.
 * 
 */
void savePartialResults(int workerId, struct controlInfo controlInfo) {
    // Enter monitor
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) != 0) {
        errno = statusWorker[workerId];
        perror("Error on entering monitor(CF).\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    pthread_once(&init, initialization);

    // Update global counts
    for (int i = 0; i < controlInfo.maxWordSize; i++) {
        wordSizeResults[controlInfo.fileId][i] += controlInfo.wordSize[i];
        numberWordsResults[controlInfo.fileId] += controlInfo.wordSize[i];
        if (i > maximumSizeWordResults[controlInfo.fileId] &&
            controlInfo.wordSize[i] > 0) {
            maximumSizeWordResults[controlInfo.fileId] = i;
        }
        if (i < minimumSizeWordResults[controlInfo.fileId] &&
            controlInfo.wordSize[i] > 0) {
            minimumSizeWordResults[controlInfo.fileId] = i;
        }
    }
    for (int i = 0; i < controlInfo.maxVowelCount; i++) {
        for (int j = 0; j < controlInfo.maxWordSize; j++) {
            vowelCountResults[controlInfo.fileId][i][j] +=
                controlInfo.vowelCount[i][j];
        }
    }

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
 *  Monitor finds the files to be processed through their paths and opens them for processing.
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
        wordSizeResults = malloc(sizeof(int*) * (filesSize));
        vowelCountResults = malloc(sizeof(int*) * (filesSize));
        maximumSizeWordResults = malloc(sizeof(int) * (filesSize));
        minimumSizeWordResults = malloc(sizeof(int) * (filesSize));
        numberWordsResults = malloc(sizeof(int) * (filesSize));
        if (wordSizeResults == NULL || vowelCountResults == NULL ||
            maximumSizeWordResults == NULL || minimumSizeWordResults == NULL ||
            numberWordsResults == NULL) {
            errno = statusMain;
            perror("Error while allocating memory in presentFilenames.\n");
            statusMain = EXIT_FAILURE;
            pthread_exit(&statusMain);
        }

        // Process files given as input
        for (int i = 0; i < size; i++) {
            maximumSizeWordResults[i] = 0;
            minimumSizeWordResults[i] = MAXSIZE;
            numberWordsResults[i] = 0;
            wordSizeResults[i] = malloc(sizeof(int) * (MAXSIZE));
            vowelCountResults[i] = malloc(sizeof(int*) * (MAXSIZE));
            if (wordSizeResults[i] == NULL || vowelCountResults[i] == NULL) {
                errno = statusMain; /* save error in errno */
                perror("Error while allocating memory in presentFilenames.\n");
                statusMain = EXIT_FAILURE;
                pthread_exit(&statusMain);
            }
            for (int j = 0; j < MAXSIZE; j++) {
                wordSizeResults[i][j] = 0;
                if ((vowelCountResults[i][j] =
                         malloc(sizeof(int) * (MAXSIZE))) == NULL) {
                    errno = statusMain; /* save error in errno */
                    perror(
                        "Error while allocating memory in presentFilenames.\n");
                    statusMain = EXIT_FAILURE;
                    pthread_exit(&statusMain);
                }
                for (int l = 0; l < MAXSIZE; l++) {
                    vowelCountResults[i][j][l] = 0;
                }
            }
            files[i] = fopen(filenames[i], "r");
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
 *  \brief Presentation of the global results achieved by all worker threads.
 * 
 *  Monitor prints in a formatted form the results of the 'wordCount' program execution.
 * 
 */
void printResults() {
    // Enter monitor
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on entering monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
    pthread_once(&init, initialization);

    // Print results from counts for all files
    int i;
    for (int k = 0; k < filesSize; k++) {
        printf("File name: %s\n", filenames[k]);
        printf("Total number of words: %d\n", numberWordsResults[k]);
        printf("Word length\n");

        printf("   ");
        for (i = 1; i < maximumSizeWordResults[k] + 1; i++) {
            printf("%6d", i);
        }
        printf("\n   ");
        for (i = 1; i < maximumSizeWordResults[k] + 1; i++) {
            printf("%6d", wordSizeResults[k][i]);
        }
        printf("\n   ");
        for (i = 1; i < maximumSizeWordResults[k] + 1; i++) {
            printf("%6.2f", ((float)wordSizeResults[k][i] * 100.0) /
                                (float)numberWordsResults[k]);
        }
        for (i = 0; i < maximumSizeWordResults[k] + 1; i++) {
            printf("\n%2d ", i);
            for (int j = 1; j < i; j++) {
                printf("%6s", " ");
            }
            if (i == 0) {
                for (int j = 1; j < maximumSizeWordResults[k] + 1; j++) {
                    if (wordSizeResults[k][j] > 0) {
                        printf("%6.1f", (vowelCountResults[k][i][j] * 100.0) /
                                            (float)wordSizeResults[k][j]);
                    } else {
                        printf("%6.1f", 0.0);
                    }
                }
            } else {
                for (int j = i; j < maximumSizeWordResults[k] + 1; j++) {
                    if (wordSizeResults[k][j] > 0) {
                        printf("%6.1f", (vowelCountResults[k][i][j] * 100.0) /
                                            (float)wordSizeResults[k][j]);
                    } else {
                        printf("%6.1f", 0.0);
                    }
                }
            }
        }
        printf("\n\n");
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
    free(maximumSizeWordResults);
    free(minimumSizeWordResults);
    free(numberWordsResults);
    for (int i = 0; i < filesSize; i++) {
        for (int j = 0; j < MAXSIZE; j++) {
            free(vowelCountResults[i][j]);
            wordSizeResults[i][j] = 0;
        }
        free(wordSizeResults[i]);
        free(vowelCountResults[i]);
    }
    free(wordSizeResults);
    free(vowelCountResults);
    printf("Monitor destroyed.\n");

    // Leave monitor
    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) {
        errno = statusMain;
        perror("Error on exiting monitor(CF).\n");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
}
