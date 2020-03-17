
#ifndef TEXTPROC_H
#define TEXTPROC_H

#include "chunk.h"

extern Chunk getTextChunk(int workerId);
extern void savePartialResults(int workerId, int fileId, int* wordCount,
                               int wordSizeSize, int* vowelCount,
                               int vowelCountSize);
extern void presentFilenames(int size, char** filenames);
extern void printResults();

#endif /* TEXTPROC_H */
