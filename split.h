/*
 * split.h
 *
 * description: split function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef SPLIT_H_
#define SPLIT_H_

/**
 * Split string into array of at most n tokens.
 * Tokens returned are allocated and must be freed.
 *
 * @param str the character string
 * @param toks token array
 * @param ntoks max number of tokens to retrieve, 0 = unlimited
 * @param delim the delimiter string between tokens
 */
int split(const char *p, char *toks[], int ntoks, const char *delim);

/**
 * Free tokens allocated by split() and sets array
 * elements to NULL.
 *
 * @param toks the token array.
 * @paam ntoks the number of tokens
 */
void free_split_tokens(char *toks[], int ntoks);

#endif /* SPLIT_H_ */
