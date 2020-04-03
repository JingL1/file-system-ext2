/*
 * fs_utime.c
 *
 * description: fs_utime function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <utime.h>

#include "fs_util_meta.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"

/**
 * utime - change access and modification times.
 *
 * Errors:
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *
 * @param path the file or directory path.
 * @param ut utimbuf - see man 'utime' for description.
 * @return 0 if successful, or -error number
 */
int fs_utime(const char* path, struct utimbuf *ut)
{
	// get inode number of specified path
    int inum = get_inode_of_file_path(path);

    // ensure that inode exists
    if (inum < 0) {
        return inum;
    }

    // set new mod time for inode
    fs.inodes[inum].mtime = ut->modtime;  // OK thorough 2100

    // mark inode dirty and flush metadata blocks
    mark_inode(inum);
    flush_metadata();

    return 0;
}


