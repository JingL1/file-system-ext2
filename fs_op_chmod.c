/*
 * fs_op_chmod.c
 *
 * description: fs_chmod function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <sys/stat.h>

#include "fs_util_meta.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"


/**
 * chmod - change file permissions.
 *
 * Errors:
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *
 * @param path the file or directory path
 * @param mode the mode_t mode value -- see man 'chmod'
 *   for description
 * @return 0 if successful, or -error number
 */
int fs_chmod(const char* path, mode_t mode)
{
	// get inode for specified path
    int inum = get_inode_of_file_path(path);

    // ensure that inode exists
    if (inum < 0) {
        return inum;
    }

    // set new permissions for inode
    fs.inodes[inum].mode =  // ensures only permissions modified
    	(fs.inodes[inum].mode & S_IFMT) | (mode & ~S_IFMT);

    // mark inode dirty and flush metadata blocks
    mark_inode(inum);
    flush_metadata();

    return 0;
}

