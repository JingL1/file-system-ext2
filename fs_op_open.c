/*
 * fs_op_open.c
 *
 * description: fs_open function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fuse.h>

#include "fs_util_path.h"
#include "fs_util_vol.h"

/**
 * Open a filesystem file or directory path.
 *
 * Errors:
 *   -ENOENT  - file does not exist
 *   -ENOTDIR - component of path not a directory
 *   -EISDIR  - file is a directory
 *
 * @param path the path
 * @param fuse file info data
 * @return 0 successful, or -error number
 */
int fs_open(const char* path, struct fuse_file_info* fi)
{
    if (fi != NULL) {
    	// get inode for path
        int inum = get_inode_of_file_path(path);

        // report error if not found
        if (inum < 0) {
            return inum;
        }

        /* cannot open if it is directory */
        if (S_ISDIR(fs.inodes[inum].mode)) {
            return -EISDIR;
        }

        // set inode number to fi->fh for fs_read() operations
        fi->fh = inum;
    }
    return 0;
}


