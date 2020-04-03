/*
 * fs_op_opendir.c
 *
 * description: fs_opendir function for CS 5600 / 7600 file system
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
 * opendir - open file directory.
 *
 * You can save information about the open directory in
 * fi->fh. If you allocate memory, free it in fs_releasedir.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @param fi fuse file system information
 * @return 0 if successful, or -error number
 *
 */
int fs_opendir(const char* path, struct fuse_file_info* fi)
{
    if (fi != NULL) {
    	// get inode for specified path
        int inum = get_inode_of_file_path(path);

        // return error code if not found
        if (inum < 0) {
            return inum;
        }

        /* cannot open if it is not a directory */
        if (!S_ISDIR(fs.inodes[inum].mode)) {
            return -ENOTDIR;
        }

        // save directory inum in fuse file info
        fi->fh = inum;
    }
    return 0;
}

