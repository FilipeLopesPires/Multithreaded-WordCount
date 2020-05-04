/**
 *  \file signalCrossCorrelation.c (implementation file)
 *
 *  \brief Multi-threaded implementation of the program that computes the
 * circular cross-correlation of the values of two signals.
 *
 *  The program 'signalCrossCorrelation' reads in succession the values of pairs
 * of signals stored in several data files whose names are provided, computes
 * the circular cross-correlation of each pair and appends it to the
 * corresponding file. In this implementation, threads synchronization is based
 * on monitors. Both threads and the monitor are implemented using the pthread
 * library which enables the creation of a monitor of the Lampson / Redell type.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include "signalCrossCorrelation.h"

#include <dirent.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "results.h"
#include "signal.h"
#include "signalProc.h"

#define BILLION 1000000000.0

/** \brief worker life cycle routine */
static void *worker(void *id);

/** \brief worker threads return status array */
int statusWorker[NUMWORKERS];

/** \brief main thread return status value */
int statusMain;

/**
 *  \brief Main function called when the program is executed.
 *
 *  Main function of the 'signalCrossCorrelation' program responsible for
 * creating worker threads and managing the monitor for delivering the desired
 * results. The function receives the paths to the text files.
 *
 *  \param argc number of files passed to the program.
 *  \param argv paths to the signal files.
 *
 */
int main(int argc, char **argv) {
    // Validate program arguments

    int opt;
    // int numberSignals = 2;
    bool compareEnabled = false;

    while ((opt = getopt(argc, argv, "c")) != -1) {
        switch (opt) {
            case 'c':
                compareEnabled = true;
                break;
        }
    }

    if ((argc - optind) < 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Declare useful variables

    pthread_t workerThreadID[NUMWORKERS];  // workers internal thread id array
    unsigned int
        workerID[NUMWORKERS];  // workers application defined thread id array
    int *status_p;             // pointer to execution status
    int i;                     // aux variable for local loops
    //double t0, t1;             // time limits
    struct timespec t0, t1;    // time variables to calculate execution time

    // Initialization of thread IDs

    for (i = 0; i < NUMWORKERS; i++) {
        workerID[i] = i;
    }
    srandom((unsigned int)getpid());
    //t0 = ((double)clock()) / CLOCKS_PER_SEC;
    clock_gettime(CLOCK_REALTIME, &t0);

    // Retrieval of filenames

    char *files[argc - optind];
    for (i = optind; i < argc; i++) {
        files[i - optind] = argv[i];
    }
    presentFilenames(argc - optind, files);

    // Generation of worker threads

    for (i = 0; i < NUMWORKERS; i++) {
        if (pthread_create(&workerThreadID[i], NULL, worker, &workerID[i]) !=
            0) {
            perror("Error on creating thread worker.\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("Threads created.\n");

    // Report post task completion (by workers)

    for (i = 0; i < NUMWORKERS; i++) {
        if (pthread_join(workerThreadID[i], (void *)&status_p) != 0) {
            perror("Error on waiting for thread worker.\n");
            exit(EXIT_FAILURE);
        }
        printf("thread worker, with id %u, has terminated: ", i);
        printf("its status was %d\n", *status_p);
    }
    writeOrPrintResults(compareEnabled);
    destroy();

    // Execution time calculation

    //t1 = ((double)clock()) / CLOCKS_PER_SEC;
    //printf("\nElapsed time = %.6f s\n", t1 - t0);

    clock_gettime(CLOCK_REALTIME, &t1);
    double exec_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / BILLION;
    printf("\nElapsed time = %.6f s\n", exec_time);

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
    // Instantiate and initialize thread variables

    int id = *((int *)par);  // worker ID
    struct signal signal;
    struct results results;
    int mod;
    double curSum;
    int count = 0;

    // Calculate cross correlation
    bool run = getSignalAndTau(id, &signal, &results);
    while (run) {
        curSum = 0.0;
        count++;

        for (int n = 0; n < signal.signalSize; n++) {
            mod = (results.tau + n) % signal.signalSize;
            curSum += signal.values[0][n] * signal.values[1][mod];
        }

        // Save processing results for the current tau value

        results.value = curSum;
        savePartialResults(id, &results);
        run = getSignalAndTau(id, &signal, &results);
    }

    statusWorker[id] = EXIT_SUCCESS;
    pthread_exit(&statusWorker[id]);
}