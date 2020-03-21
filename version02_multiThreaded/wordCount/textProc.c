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

#include "chunk.h"
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

char* textBuffer;
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
struct Chunk chunk;

void initialization(void) {
    strcpy(tmpWord, "");
    incrementFileIdx = false;

    pthread_cond_init(&filenamesPresented, NULL);
    printf("Monitor initialized.\n");
}

struct Chunk getTextChunk(int workerId) {
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
        if ((statusWorker[workerId] = pthread_cond_wait(&filenamesPresented, &accessCR)) != 0) {
            errno = statusWorker[workerId]; 
            perror("Error on waiting in fifoFull.\n");
            statusWorker[workerId] = EXIT_FAILURE;
            pthread_exit(&statusWorker[workerId]);
        }
    }

    // Retrieve text chunk from current file
    //textBuffer = malloc(sizeof(char) * BUFFERSIZE);
    if((textBuffer = malloc(sizeof(char) * BUFFERSIZE)) == NULL) {
        errno = statusWorker[workerId]; 
        perror("Error while allocating memory in getTextChunk.\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    strcpy(textBuffer, tmpWord);
    strcpy(tmpWord, "");
    if (currentFileIdx < filesSize) {
        while (strlen(textBuffer) < BUFFERSIZE) {
            symbol = getc(files[currentFileIdx]);

            // Verify if the current file has ended
            if (symbol == EOF) {
                if (strlen(tmpWord) + strlen(textBuffer) < BUFFERSIZE) {
                    strcat(textBuffer, tmpWord);
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
                    if (strlen(tmpWord) + strlen(textBuffer) < BUFFERSIZE) {
                        strcat(textBuffer, tmpWord);
                        strcpy(tmpWord, "");
                    } else {
                        leaveLoop = true;
                    }
                }
            }

            strcat(tmpWord, completeSymbol);
            if (leaveLoop) {
                break;
            }
        }
    }
    chunk.fileId = currentFileIdx;
    chunk.textChunk = strdup(textBuffer);
    if (incrementFileIdx) {
        currentFileIdx++;
        incrementFileIdx = false;
    }

    // Free used memory
    free(textBuffer);

    // Leave monitor
    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) != 0) { 
        errno = statusWorker[workerId]; 
        perror("Error on exiting monitor(CF).\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    return chunk;
}

void savePartialResults(int workerId, int fileId, int* wordSize,
                        int wordSizeSize, int** vowelCount, int vowelCountSizeX,
                        int vowelCountSizeY) {
    // Enter monitor
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) != 0) { 
        errno = statusWorker[workerId];
        perror("Error on entering monitor(CF).\n");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    pthread_once(&init, initialization);

    // Update global counts
    for (int i = 0; i < wordSizeSize; i++) {
        wordSizeResults[fileId][i] += wordSize[i];
        numberWordsResults[fileId] += wordSize[i];
        if (i > maximumSizeWordResults[fileId] && wordSize[i] > 0) {
            maximumSizeWordResults[fileId] = i;
        }
        if (i < minimumSizeWordResults[fileId] && wordSize[i] > 0) {
            minimumSizeWordResults[fileId] = i;
        }
    }
    for (int i = 0; i < vowelCountSizeX; i++) {
        for (int j = 0; j < vowelCountSizeY; j++) {
            vowelCountResults[fileId][i][j] += vowelCount[i][j];
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
        if((files = malloc(sizeof(FILE*) * (filesSize))) == NULL) {
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
        if(wordSizeResults == NULL || vowelCountResults == NULL || maximumSizeWordResults == NULL || minimumSizeWordResults == NULL || numberWordsResults == NULL) {
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
            if(wordSizeResults[i] == NULL || vowelCountResults[i] == NULL) {
                errno = statusMain; /* save error in errno */
                perror("Error while allocating memory in presentFilenames.\n");
                statusMain = EXIT_FAILURE;
                pthread_exit(&statusMain);
            }
            for (int j = 0; j < MAXSIZE; j++) {
                wordSizeResults[i][j] = 0;
                if((vowelCountResults[i][j] = malloc(sizeof(int) * (MAXSIZE))) == NULL) {
                    errno = statusMain; /* save error in errno */
                    perror("Error while allocating memory in presentFilenames.\n");
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
    free(textBuffer);
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
