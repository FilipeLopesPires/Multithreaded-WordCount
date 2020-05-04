/**
 *  \file results.h (interface file)
 *
 *  \brief Definition of the structure containing the cross correlation results
 * for a given file.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef RESULTS_H
#define RESULTS_H

/**
 *  \brief structure containing the cross correlation results for a given file.
 */
struct results {
    /** \brief identifier of the file the results belongs to. */
    int fileId;

    /** \brief tau value used. */
    int tau;

    /** \brief results of the cross correlation. */
    double value;
};

#endif /* RESULTS_H */