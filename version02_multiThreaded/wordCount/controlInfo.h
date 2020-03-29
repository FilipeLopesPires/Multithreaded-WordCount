#include "wordCount.h"

#ifndef CONTROLINFO_H
#define CONTROLINFO_H

struct controlInfo {
    int wordSize[MAXSIZE];
    int vowelCount[MAXSIZE][MAXSIZE];
    int maxWordSize;
    int maxVowelCount;
    int fileId;
};

#endif /* CONTROLINFO_H */