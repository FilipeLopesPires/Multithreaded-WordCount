/**
 *  \file textProc.h (interface file)
 *
 *  \brief Program that reads in succession several text files and prints a
 * listing of the occurring frequency of word lengths and the number of vowels
 * in each word for each of the supplied texts.
 *
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which
 * enables the creation of a monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the workers:
 *     \li getTextChunk
 *     \li savePartialResults
 *     \li presentFilenames
 *     \li printResults.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef TEXTPROC_H
#define TEXTPROC_H

#include "chunk.h"

extern struct Chunk getTextChunk(int workerId);
extern void savePartialResults(int workerId, int fileId, int* wordSize,
                               int wordSizeSize, int** vowelCount,
                               int vowelCountSizeX, int vowelCountSizeY);
extern void presentFilenames(int size, char** filenames);
extern void printResults();

#endif /* TEXTPROC_H */
