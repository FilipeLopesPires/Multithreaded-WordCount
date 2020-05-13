/**
 *  \file textProc.h (interface file)
 *
 *  \brief Word Count Problem Monitor methods definition.
 *
 *  The program 'wordCount' deploys worker threads that read in succession several text files text#.txt and calculate the occurring frequency of word lengths and the number of vowels in each word for each of the supplied texts, in the end the program prints the results achieved by its workers.
 *  Threads synchronization is based on monitors. Both threads and the monitor are implemented using the pthread library which enables the creation of a monitor of the Lampson / Redell type.
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

#include "controlInfo.h"

/** 
 *  \brief Retrieval of a portion of text (called text chunk).
 * 
 *  Monitor retrieves a portion of text (called text chunk) and assigns its processing for the worker that called the method.
 * 
 *  \param workerId internal worker thread identifier.
 *  \param textChunk portion of text to be processed by the worker.
 *  \param controlInfo structure containing control variables regarding the results of the worker.
 * 
 */
extern bool getTextChunk(int workerId, char* textChunk, struct controlInfo controlInfo);

/** 
 *  \brief Update of global results.
 * 
 *  Monitor updates the global results with the results achieved by the worker that called the method.
 * 
 *  \param workerId internal worker thread identifier.
 *  \param controlInfo structure containing control variables regarding the results of the worker.
 * 
 */
extern void savePartialResults(int workerId, struct controlInfo controlInfo);

/** 
 *  \brief Presentation of all the files to be processed.
 * 
 *  Monitor finds the files to be processed through their paths and opens them for processing.
 * 
 *  \param size number of files to be presented.
 *  \param fileNames array containing the paths to the files.
 * 
 */
extern void presentFilenames(int size, char** filenames);

/** 
 *  \brief Presentation of the global results achieved by all worker threads.
 * 
 *  Monitor prints in a formatted form the results of the 'wordCount' program execution.
 * 
 */
extern void printResults();

/** 
 *  \brief Destruction of monitor variables.
 * 
 *  Monitor frees memory allocated for its variables.
 * 
 */
extern void destroy();

#endif /* TEXTPROC_H */
