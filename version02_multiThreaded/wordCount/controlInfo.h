/**
 *  \file controlInfo.h (interface file)
 *
 *  \brief Definition of the structure containing control variables for the 'wordCount' program.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#include "wordCount.h"

#ifndef CONTROLINFO_H
#define CONTROLINFO_H

/**
 *  \brief structure containing control variables regarding the results of a worker thread for the 'wordCount' program.
 */
struct controlInfo {

    /** \brief array containing the results of the occurrence frequence for each word size. */
    int wordSize[MAXSIZE];

    /** \brief 2D array containing the results of the vowels count for each word size. */
    int vowelCount[MAXSIZE][MAXSIZE];

    /** \brief maximum size possible for a word. */
    int maxWordSize;

    /** \brief maximum number of vowels in a word. */
    int maxVowelCount;

    /** \brief identifier of the current file under processing. */
    int fileId;

};

#endif /* CONTROLINFO_H */