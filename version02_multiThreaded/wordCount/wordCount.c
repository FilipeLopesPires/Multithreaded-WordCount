/**
 *  \file wordCount.c (implementation file)
 *
 *  \brief Program that reads in succession several text files and prints a
 * listing of the occurring frequency of word lengths and the number of vowels
 * in each word for each of the supplied texts.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which
 * enables the creation of a monitor of the Lampson / Redell type.
 *
 *  Generator thread of the intervening entities.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <pthread.h>
//#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "chunk.h"
#include "textProc.h"
#include "wordCount.h"

/** \brief worker life cycle routine */
static void *worker(void *id);

/** \brief worker threads return status array */
int statusWorker[NUMWORKERS];

/** \brief main thread return status value */
int statusMain;

int tmp = 0;

int main(int argc, char **argv) {
    // Validate number of arguments passed to the program
    if (argc <= 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Declare useful variables
    pthread_t workerThreadID[NUMWORKERS];  // workers internal thread id array
    unsigned int
        workerID[NUMWORKERS];  // workers application defined thread id array
    int *status_p;             // pointer to execution status
    int i;                     // aux variable for local loops
    double t0, t1;             // time limits

    // Initialization of thread IDs
    for (i = 0; i < NUMWORKERS; i++) {
        workerID[i] = i;
    }
    srandom((unsigned int)getpid());
    t0 = ((double)clock()) / CLOCKS_PER_SEC;

    // Retrieval of filenames
    char *files[argc - 1];
    for (i = 1; i < argc; i++) {
        files[i - 1] = argv[i];
    }

    presentFilenames(argc - 1, files);

    // Generation of worker threads
    for (i = 0; i < NUMWORKERS; i++) {
        if (pthread_create(&workerThreadID[i], NULL, worker, &workerID[i]) !=
            0) {
            perror("error on creating thread worker");
            exit(EXIT_FAILURE);
        }
    }
    printf("Threads created.\n");

    // Report post task completion (by workers)
    for (i = 0; i < NUMWORKERS; i++) {
        if (pthread_join(workerThreadID[i], (void *)&status_p) != 0) {
            perror("error on waiting for thread worker");
            exit(EXIT_FAILURE);
        }
        printf("thread worker, with id %u, has terminated: ", i);
        printf("its status was %d\n", *status_p);
    }
    printResults();

    // Execution time calculation
    t1 = ((double)clock()) / CLOCKS_PER_SEC;
    printf("\nElapsed time = %.6f s\n", t1 - t0);

    exit(EXIT_SUCCESS);
}

/**
 *  \brief Function worker.
 *
 *  Its role is to simulate the life cycle of a worker.
 *
 *  \param par pointer to application defined worker identification
 */

static void *worker(void *par) {
    unsigned int id = *((unsigned int *)par);  // worker id

    // char *files[] = {"filename0", "file"};
    // presentFilenames(2, files);

    Chunk chunk = getTextChunk(id);
    // printf("%s", chunk.textChunk);
    while (strcmp(chunk.textChunk, "") == 0) {
        chunk = getTextChunk(id);
    }

    statusWorker[id] = EXIT_SUCCESS;
    pthread_exit(&statusWorker[id]);
}