/**
 *  \file signalCrossCorrelation.c (implementation file)
 *
 *  \brief Program that computes the circular cross-correlation of the values of two signals.
 *
 *  The program 'signalCrossCorrelation' reads in succession the values of pairs of signals stored in several data files whose names are provided, computes the circular cross-correlation of each pair and appends it to the corresponding file.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/** 
 *  \brief Main function called when the program is executed.
 * 
 *  Main function of the 'signalCrossCorrelation' program containing all of its logic.
 *  The function receives the paths to the files containing the signals.
 * 
 *  \param argc number of files passed to the program.
 *  \param argv paths to the signal files.
 * 
 */
int main(int argc, char **argv) {

    // Declare useful variables
    
    /** \brief number of signals each file contains. */
    int numberSignals = 2;

    /** \brief auxiliar variable to retrieve the result of getopt() for all arguments passed to the program. */
    int opt;

    /** \brief auxiliar variable to determine whether the operator -c was used in the program's arguments or not. */
    bool compareEnabled = false;

    /** \brief pointer to the file currently under processing. */
    FILE *file;

    /** \brief number of elements, given by the first 4 bytes of the current file. */
    int nElem;

    /** \brief array containing the values of the signals present in the current file. */
    double *sig[numberSignals];

    /** \brief auxiliar variable containing the current signal under analysis, belonging to the current file. */
    double curSig;

    // double *results; // array containing the results of the similarity evaluation

    // Validate program arguments

    while ((opt = getopt(argc, argv, "c")) != -1) {
        switch (opt) {
            case 'c':
                numberSignals = 3;
                compareEnabled = true;
                break;
        }
    }

    if ((argc - optind) < 1) {
        printf("The program need at least one text file to parse!\n");
        exit(1);
    }

    // Process all files passed as arguments and calculate the cross correlation in each

    for (int fileIndex = optind; fileIndex < argc; fileIndex++) {
        printf("Processing file %s\n", argv[fileIndex]);

        // Read file
        file = fopen(argv[fileIndex], "r+");
        if (file == NULL) {
            // end of file error
            printf("Error while opening file!\n");
            exit(1);
        }

        // Get number of elements
        fread(&nElem, 4, 1, file);

        // Process remainder of file (actual signals)
        for (int n = 0; n < numberSignals; n++) {
            if ((sig[n] = malloc(nElem * sizeof(double))) == NULL) {
                // malloc error message
                printf("Error while allocating memory for signal!\n");
                exit(1);
            }
            for (int i = 0; i < nElem; i++) {
                // if(reaches EOF) {
                //    // file format error message
                //    ...
                //}
                fread(&curSig, 8, 1, file);
                sig[n][i] = curSig;
            }
        }

        // Allocate memory for results
        // if ((results = malloc(nElem * sizeof(double))) == NULL) {
        //     // malloc error message
        //     printf("Error while allocating memory for results!");
        //     exit(1);
        // }

        // Calculate cross correlation
        int mod;
        int numErrors = 0;

        if (compareEnabled) {
            printf("Comparing one by one:\n");
        }
        for (int t = 0; t < nElem; t++) {
            double curSum = 0.0;
            for (int n = 0; n < nElem; n++) {
                mod = (t + n) % nElem;
                curSum += sig[0][n] * sig[1][mod];
            }
            // results[t] = curSum;

            if (compareEnabled) {
                if (curSum == sig[2][t]) {
                    printf("1");
                } else {
                    numErrors++;
                    printf("0");
                }
            } else {
                fwrite(&curSum, 8, 1, file);
            }
        }

        if (compareEnabled) {
            printf("Num. Errors Found: %d\n", numErrors);
        }

        fclose(file);
    }
}