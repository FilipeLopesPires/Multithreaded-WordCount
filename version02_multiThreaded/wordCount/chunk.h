/**
 *  \file chunk.h (interface file)
 *
 *  \brief Definition of the structure containing a portion of text from a given file.
 *
 *  \author Filipe Pires (85122) and João Alegria (85048) - March 2020
 */

#ifndef CHUNK_H
#define CHUNK_H

/**
 *  \brief structure containing a portion of text from a given file.
 */
struct Chunk {

    /** \brief identifier of the file the text chunk belongs to. */
    int fileId;

    /** \brief portion of text belonging to a text file. */
    char* textChunk;

};

#endif /* CHUNK_H */