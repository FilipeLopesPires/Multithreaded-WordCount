/**
 *  \file textProc.c (implementation file)
 *
 *  \brief Program that reads in succession several text files and prints a
 * listing of the occurring frequency of word lengths and the number of vowels
 * in each word for each of the supplied texts.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which
 * enables the creation of a monitor of the Lampson / Redell type.
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

char delimiters[25][MAXCHARSIZE] = {
    " ", "-", "–", "—",  ".",  ",",  ":",  ";", "(", ")", "[", "]", "{",
    "}", "?", "!", "\n", "\t", "\r", "\"", "“", "”", "«", "»", "…"};

/** \brief worker threads return status array */
extern int statusWorker[NUMWORKERS];

/** \brief main thread return status value */
extern int statusMain;

bool areFilenamesPresented;

pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

pthread_once_t init = PTHREAD_ONCE_INIT;

pthread_cond_t filenamesPresented;

char textBuffer[BUFFERSIZE];
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

void initialization(void) {
    strcpy(textBuffer, "");
    strcpy(tmpWord, "");
    incrementFileIdx = false;

    pthread_cond_init(&filenamesPresented, NULL);
    printf("Monitor initialized.\n");
}

Chunk getTextChunk(int workerId) {
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) != 0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    pthread_once(&init, initialization);

    while (!areFilenamesPresented) {
        if ((statusWorker[workerId] =
                 pthread_cond_wait(&filenamesPresented, &accessCR)) != 0) {
            errno = statusWorker[workerId]; /* save error in errno */
            perror("error on waiting in fifoFull");
            statusWorker[workerId] = EXIT_FAILURE;
            pthread_exit(&statusWorker[workerId]);
        }
    }

    strcpy(textBuffer, tmpWord);
    strcpy(tmpWord, "");
    while (strlen(textBuffer) <= BUFFERSIZE) {
        symbol = getc(files[currentFileIdx]);
        if (symbol == EOF) {
            if (strlen(tmpWord) + strlen(textBuffer) <= BUFFERSIZE) {
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

        // Build the complete character (if it consists of more than 1 byte)
        strncat(completeSymbol, &symbol, 1);
        for (int i = 1; i < ones; i++) {
            symbol = getc(files[currentFileIdx]);
            strncat(completeSymbol, &symbol, 1);
        }

        bool leaveLoop = false;
        // Check if character is a delimiter
        for (int i = 0; i < sizeof(delimiters) / sizeof(delimiters[0]); i++) {
            if (strcmp(completeSymbol, delimiters[i]) == 0) {
                if (strlen(tmpWord) + strlen(textBuffer) <= BUFFERSIZE) {
                    strcat(textBuffer, tmpWord);
                    strcpy(tmpWord, "");
                } else {
                    leaveLoop = true;
                }
            }
        }
        if (leaveLoop) {
            break;
        }

        strcat(tmpWord, completeSymbol);
    }

    struct Chunk chunk = {.fileId = -1, .textChunk = textBuffer};
    if (strlen(textBuffer) != 0) {
        chunk.fileId = currentFileIdx;
        // strcat(chunk.textChunk, textBuffer);
    }

    if (incrementFileIdx) {
        currentFileIdx++;
        incrementFileIdx = false;
    }

    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) != 0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }

    return chunk;
}

void savePartialResults(int workerId, int fileId, int* wordSize, int wordSizeSize, int** vowelCount, int vowelCountSizeX, int vowelCountSizeY) {
    if ((statusWorker[workerId] = pthread_mutex_lock(&accessCR)) != 0) /* enter monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
    pthread_once(&init, initialization);

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
            vowelCountResults[fileId][i][j] = vowelCount[i][j];
        }
    }

    if ((statusWorker[workerId] = pthread_mutex_unlock(&accessCR)) != 0) /* exit monitor */
    {
        errno = statusWorker[workerId]; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusWorker[workerId] = EXIT_FAILURE;
        pthread_exit(&statusWorker[workerId]);
    }
}

void presentFilenames(int size, char** fileNames) {
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) /* enter monitor */
    {
        errno = statusMain; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
    pthread_once(&init, initialization);

    if (size > 0) {
        files = malloc(sizeof(FILE*) * (size));

        filesSize = size;
        wordSizeResults = malloc(sizeof(int*) * (size));
        vowelCountResults = malloc(sizeof(int*) * (size));
        maximumSizeWordResults = malloc(sizeof(int) * (size));
        minimumSizeWordResults = malloc(sizeof(int) * (size));
        numberWordsResults = malloc(sizeof(int) * (size));
        for (int i = 0; i < size; i++) {
            maximumSizeWordResults[i] = 0;
            minimumSizeWordResults[i] = MAXSIZE;
            numberWordsResults[i] = 0;
        }
        filenames = fileNames;
        for (int i = 0; i < size; i++) {
            wordSizeResults[i] = malloc(sizeof(int) * (MAXSIZE));
            vowelCountResults[i] = malloc(sizeof(int) * (MAXSIZE));
            for (int j = 0; j < MAXSIZE; j++) {
                wordSizeResults[i][j] = 0;
                vowelCountResults[i][j] = malloc(sizeof(int) * (MAXSIZE));
                for (int l = 0; l < MAXSIZE; l++) {
                    vowelCountResults[i][j][l] = 0;
                }
            }
            files[i] = fopen(filenames[i], "r");
        }

        areFilenamesPresented = true;
        printf("Files presented.\n");
    }

    if ((statusMain = pthread_cond_signal(&filenamesPresented)) != 0) {
        errno = statusMain; /* save error in errno */
        perror("error on signaling in fifoEmpty");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }

    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) /* exit monitor */
    {
        errno = statusMain; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
}

void printResults() {
    if ((statusMain = pthread_mutex_lock(&accessCR)) != 0) /* enter monitor */
    {
        errno = statusMain; /* save error in errno */
        perror("error on entering monitor(CF)");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
    pthread_once(&init, initialization);
    int i;
    for (int k = 0; k < filesSize; k++) {
        // Print information table
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
                        printf("%6.1f", (vowelCountResults[k][i][j] * 100.0) / (float)wordSizeResults[k][j]);
                    } else {
                        printf("%6.1f", 0.0);
                    }
                }
            } else {
                for (int j = i; j < maximumSizeWordResults[k] + 1; j++) {
                    if (wordSizeResults[k][j] > 0) {
                        printf("%6.1f", (vowelCountResults[k][i][j] * 100.0) / (float)wordSizeResults[k][j]);
                    } else {
                        printf("%6.1f", 0.0);
                    }
                }
            }
        }
        printf("\n\n");
    }

    if ((statusMain = pthread_mutex_unlock(&accessCR)) != 0) /* exit monitor */
    {
        errno = statusMain; /* save error in errno */
        perror("error on exiting monitor(CF)");
        statusMain = EXIT_FAILURE;
        pthread_exit(&statusMain);
    }
}
