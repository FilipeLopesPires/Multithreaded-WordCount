
#ifndef TEXTPROC_H
#define TEXTPROC_H

extern char* getTextChunk(int workerId);
extern void savePartialResults(int workerId, int* wordCount, int* vowelCount);
extern void presentFilenames(char* filenames);
extern void printResults();

#endif /* TEXTPROC_H */
