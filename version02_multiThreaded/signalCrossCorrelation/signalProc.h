/**
 *  \file signalProc.h (interface file)
 *
 *  \brief Cross Correlation Problem Monitor methods definition.
 *
 *  The program 'signalCrossCorrelation' deploys worker threads that read in
 * succession the values of pairs of signals stored in several data files whose
 * names are provided, compute the circular cross-correlation of each pair and
 * append it to the corresponding file. Threads synchronization is based on
 * monitors. Both threads and the monitor are implemented using the pthread
 * library which enables the creation of a monitor of the Lampson / Redell type.
 *  Definition of the operations carried out by the workers:
 *     \li getSignalAndTau
 *     \li savePartialResults
 *     \li presentFilenames
 *     \li writeOrPrintResults.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef TEXTPROC_H
#define TEXTPROC_H

#include "results.h"
#include "signal.h"

/**
 *  \brief Retrieval of the signals of a file and a given tau value.
 *
 *  Monitor retrieves the signal values of a file and assigns its processing
 * with a given tau value for the worker that called the method.
 *
 *  \param workerId internal worker thread identifier.
 *  \param signal structure containing the signal and tau values of a given
 * file. \param results structure to store the cross correlation results for a
 * given file.
 *
 */
extern bool getSignalAndTau(int workerId, struct signal* signal,
                            struct results* results);

/**
 *  \brief Update of global results.
 *
 *  Monitor updates the global results with the results achieved by the worker
 * that called the method.
 *
 *  \param workerId internal worker thread identifier.
 *  \param results structure containing the cross correlation results for a
 * given file.
 *
 */
extern void savePartialResults(int workerId, struct results* results);

/**
 *  \brief Presentation of all the files to be processed.
 *
 *  Monitor finds the files to be processed through their paths and opens them
 * for processing.
 *
 *  \param size number of files to be presented.
 *  \param fileNames array containing the paths to the files.
 *
 */
extern void presentFilenames(int size, char** filenames);

/**
 *  \brief Presentation of the global results achieved by all worker threads.
 *
 *  Monitor prints in a formatted form the results of the
 * 'signalCrossCorrelation' program execution.
 *
 *  \param write boolean variable that tells if the results are to be written to
 * a file or simply printed into the terminal.
 *
 */
extern void writeOrPrintResults(bool show);

/**
 *  \brief Destruction of monitor variables.
 *
 *  Monitor frees memory allocated for its variables.
 *
 */
extern void destroy();

#endif /* TEXTPROC_H */
