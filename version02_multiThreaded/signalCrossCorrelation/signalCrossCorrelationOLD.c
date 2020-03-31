#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    // Validate program arguments
    int opt;
    int numberSignals = 2;
    bool compareEnabled = false;

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

    // Declare useful variables
    FILE *file;
    int nElem;  // number of elements, given by the first 4 bytes of the
                // processed file
    double *sig[numberSignals];  // array containing the values of the signals
                                 // present in the file
    double curSig;  // auxiliar variable containing the current signal under
                    // analysis
    // double
    //     *results;  // array containing the results of the similarity
    //     evaluation

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
            printf("\nNum. Errors Found: %d\n", numErrors);
        }

        fclose(file);
    }
}