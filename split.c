/*
 * split.c
 *
 * description: split function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "split.h"

/**
 * Split string into array of at most n tokens.
 * Tokens returned are allocated and must be freed.
 *
 * @param str the character string
 * @param toks token array
 * @param ntoks max number of tokens to retrieve, 0 = unlimited
 * @param delim the delimiter string between tokens
 */
int split(const char *str, char *toks[], int ntoks, const char *delim)
{
	if (ntoks == 0) {
		ntoks = INT_MAX;
	}

	// copy because strtok modifies its argument
	char localstr[strlen(str)+1];
	strcpy(localstr, str);

	char* p = localstr;
    char *tok;
    char *lasts = NULL;
    int i;
    for (i = 0; i < ntoks && (tok = strtok_r(p, delim, &lasts)) != NULL; i++) {
    	p = NULL;
    	if (toks != NULL) {
    		toks[i] = strdup(tok);
    	}
    }

    return i;
}

/**
 * Free tokens allocated by split() and sets array
 * elements to NULL.
 *
 * @param toks the token array.
 * @paam ntoks the number of tokens
 */
void free_split_tokens(char *toks[], int ntoks) {
	for (int i = 0; i < ntoks; i++) {
		free(toks[i]);
		toks[i] = NULL;
	}
}


