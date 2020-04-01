/**
 *  \file wordCount.h (interface file)
 *
 *  \brief Word Count Problem simulation parameters.
 *
 *  The program 'wordCount' reads in succession several text files text#.txt and prints a listing of the occurring frequency of word lengths and the number of vowels in each word for each of the supplied texts.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef WORDCOUNT_H
#define WORDCOUNT_H

/** \brief number of worker threads. */
#define NUMWORKERS 5

/** \brief maximum size possible for a word. */
#define MAXSIZE 50

/** \brief maximum size (number of bytes) possible for a character. */
#define MAXCHARSIZE 6

/** \brief memory space (number of bytes) available for words under processing. */
#define BUFFERSIZE 1000

#endif /* WORDCOUNT_H */
