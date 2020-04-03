/*
 * fs_op_realeasedir.c
 *
 * description: fs_releasedir function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <fuse.h>

/**
 * Release resources when directory is closed.
 * If you allocate memory in fs_opendir, free it here.
 *
 * @param path the directory path
 * @param fi fuse file system information
 * @return 0 if successful
 */
int fs_releasedir(const char* path, struct fuse_file_info* fi)
{
	if (fi != NULL) {
		fi->fh = 0;  // remove saved inode number
	}
    return 0;
}
