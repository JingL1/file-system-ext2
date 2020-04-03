/*
 * max.h
 *
 * description: int max function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef MAX_H_
#define MAX_H_

/**
 * Integer max function
 *
 * @param a first int
 * @param b second int
 * @return the greater of the ints
 */
static inline int max(int a, int b)
{
    return a > b ? a : b;
}


#endif /* FS_UTIL_MAX_H_ */
