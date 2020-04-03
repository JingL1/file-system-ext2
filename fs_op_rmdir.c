/*
 * fs_op_rmdir.c
 *
 * description: fs_rmdir function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <errno.h>
#include <sys/select.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"
#include "max.h"

/**
 * rmdir - remove a directory.
 *
 * Errors
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *   -ENOTDIR  - path not a directory
 *   -ENOTEMPTY - directory not empty
 *
 * @param path the path of the directory
 * @return 0 if successful, or -error number
 */
int fs_rmdir(const char* path)
{
	// get inode and leaf for directory of specified path
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_file_path_dir(path, leaf);

    /* ensure inode exists */
    if (dir_inum < 0) {
        return dir_inum;
    }

    // report error if inode not a directory
    struct fs_inode *din = &fs.inodes[dir_inum];
    if (!S_ISDIR(din->mode)) {
        return -ENOTDIR;
    }


    /** find entry in directory */
    int blkno;
    char buf[FS_BLOCK_SIZE];
    int entno = get_dir_entry_block(dir_inum, buf, &blkno, leaf);
    if (entno < 0) {
    	return -ENOENT;  // entry not found
    }

    // get inode of directory entry
    struct fs_dirent *de = (void*)buf;
    int inum = de[entno].inode;

    // ensure that entry being removed is a directory
    if (!S_ISDIR(fs.inodes[inum].mode)) {
        return -ENOTDIR;  // entry must be directory
    }

	// ensure directory being removed is empty
    // 0 indicates not empty, < 0 indicates error
	if (is_dir_empty(inum) != 1) {
		return -ENOTEMPTY;
    }

    // mark directory inode free and flush its block
	de[entno].valid = 0;
    disk->ops->write(disk, blkno, 1, buf);

	// truncate all blocks of unlinked directory inode
    do_truncate(inum, 0);
    mark_inode(inum);

    // free unlinked directory inode
    return_inode(inum);

    // decrease size of directory by one fs_dirent
    // NOTE: add logging to report errors like this
    din->size = max(0, din->size - sizeof(struct fs_dirent));
    mark_inode(dir_inum);

    // flush dirty metadata blocks
    flush_metadata();

    return 0;
}

