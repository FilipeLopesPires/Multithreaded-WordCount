/**
 *  \file signal.h (interface file)
 *
 *  \brief Definition of the structure containing the signal values of a given
 * file.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef SIGNAL_H
#define SIGNAL_H

/**
 *  \brief structure containing the signal values of a given file.
 */
struct signal {
    /** \brief size of the signals (number of values). */
    int signalSize;

    /** \brief values of the signals present in the file. */
    double* values[2];

    /** \brief tau value to be used in the cross correlation. */
    int tau;
};

#endif /* SIGNAL_H */