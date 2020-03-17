/**
 *  \file wordCount.c (implementation file)
 *
 *  \brief Program that reads in succession several text files and prints a listing of the occurring 
 *  frequency of word lengths and the number of vowels in each word for each of the supplied texts.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of the Lampson / Redell type.
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

#include "wordCount.h"

/** \brief worker life cycle routine */
static void *worker (void *id);

/** \brief worker threads return status array */
int statusWorker[NUMWORKERS];

int main (int argc, char **argv) {

    // Validate number of arguments passed to the program
    if (argc <= 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Declare useful variables
    pthread_t workerThreadID[NUMWORKERS];   // workers internal thread id array
    unsigned int workerID[NUMWORKERS];      // workers application defined thread id array
    int *status_p;                          // pointer to execution status
    int i, j;                               // aux variables for local loops
    double t0, t1;                          // time limits

    // Initialize thread IDs
    for (i = 0; i < NUMWORKERS; i++) {
        workerID[i] = i;
    }
    srandom ((unsigned int) getpid ());
    t0 = ((double) clock ()) / CLOCKS_PER_SEC;

    // Generation of worker threads
    for (i = 0; i < NUMWORKERS; i++) {
        if (pthread_create (&workerThreadID[i], NULL, worker, &workerID[i]) != 0) { 
            perror ("error on creating thread worker");
            exit (EXIT_FAILURE);
        }
    }

    // Report printing after workers have completed their tasks
    for (i = 0; i < NUMWORKERS; i++) { 
        if (pthread_join (workerThreadID[i], (void *) &status_p) != 0) {
            perror ("error on waiting for thread worker");
            exit (EXIT_FAILURE);
        }
        printf ("thread worker, with id %u, has terminated: ", i);
        printf ("its status was %d\n", *status_p);
    }
    t1 = ((double) clock ()) / CLOCKS_PER_SEC;
    printf ("\nElapsed time = %.6f s\n", t1 - t0);

    exit (EXIT_SUCCESS);
        
}