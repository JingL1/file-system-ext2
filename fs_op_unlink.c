/*
 * fs_op_unlink.c
 *
 * description: fs_opendir function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <errno.h>
#include <sys/stat.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"
#include "max.h"

/**
 * unlink - delete a file.
 *
 * Errors
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *   -EISDIR   - cannot unlink a directory
 *
 * @param path path to file
 * @return 0 if successful, or -error number
 */
int fs_unlink(const char* path)
{
	// get inode and leaf for directory of specified path
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_file_path_dir(path, leaf);

    /* ensure inode exists */
    if (dir_inum < 0) {
        return dir_inum;
    }

    // ensure inode is a directory
    struct fs_inode *din = &fs.inodes[dir_inum];
    if (!S_ISDIR(din->mode)) {
        return -ENOTDIR;
    }


    /* find directory entry block and index */
    int blkno;
    char buf[FS_BLOCK_SIZE];
    int entno = get_dir_entry_block(dir_inum, buf, &blkno, leaf);
    if (entno < 0) {
        return -ENOENT;
    }

    // get inode of directory entry
    struct fs_dirent *de = (void*)buf;
    int inum = de[entno].inode;

    /* ensure that entry being removed is not a directory */
    if (S_ISDIR(fs.inodes[inum].mode)) {
        return -EISDIR;
    }
    
    do_unlink(inum, dir_inum, leaf);

    return 0;
}
