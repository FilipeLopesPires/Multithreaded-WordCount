/**
 *  \file wordCount.h (interface file)
 *
 *  \brief Program that reads in succession several text files and prints a listing of the occurring 
 *  frequency of word lengths and the number of vowels in each word for each of the supplied texts.
 *
 *  Problem simulation parameters.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef WORDCOUNT_H
#define WORDCOUNT_H

/* Generic parameters */

/** \brief number of worker threads */
#define NUMWORKERS      10

/** \brief maximum size possible for a word */
#define MAXSIZE         10

/** \brief maximum size (number of bytes) possible for a character */
#define MAXCHARSIZE     6

#endif /* WORDCOUNT_H */

