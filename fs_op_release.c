/*
 * fs_op_release.c
 *
 * description: fs_release function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <fuse.h>

/**
 * Release resources created by pending open call.
 *
 * Errors:
 *   -ENOENT  - file does not exist
 *   -ENOTDIR - component of path not a directory
 *
 * @param path the file name
 * @param fi the fuse file info
 * @return 0 successful, or -error number
 */
int fs_release(const char* path, struct fuse_file_info* fi)
{
	if (fi != NULL) {
		fi->fh = 0;  // remove saved inode number
	}
    return 0;
}

