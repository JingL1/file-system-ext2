/*
 * min.h
 *
 * description: int min utility function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef MIN_H_
#define MIN_H_

/**
 * Integer min function.
 *
 * @param a first int
 * @param b second int
 * @return the lesser of the ints
 */
static inline int min(int a, int b)
{
    return a < b ? a : b;
}


#endif /* FS_UTIL_MIN_H_ */
