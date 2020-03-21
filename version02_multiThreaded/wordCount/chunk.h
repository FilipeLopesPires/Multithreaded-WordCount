/**
 *  \file chunk.h (interface file)
 *
 *  \brief Program that reads in succession several text files and prints a
 * listing of the occurring frequency of word lengths and the number of vowels
 * in each word for each of the supplied texts.
 *
 *  Definition of the structure containing a portion of text from a given file.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef CHUNK_H
#define CHUNK_H

struct Chunk {
    int fileId;
    char* textChunk;
};

#endif /* CHUNK_H */