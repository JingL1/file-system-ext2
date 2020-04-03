/*
 * fs_op_rename.c
 *
 * description: fs_rename function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"

/**
 * rename - rename a file or directory.
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 *
 * Errors:
 *   -ENOENT   - source file or directory does not exist
 *   -ENOTDIR  - component of source or target path not a directory
 *   -EEXIST   - destination already exists
 *   -EINVAL   - source and destination not in the same directory
 *
 * @param src_path the source path
 * @param dst_path the destination path.
 * @return 0 if successful, or -error number
 */
int fs_rename(const char* src_path, const char* dst_path)
{
	// get inode and leaf for directory of specified source path
    char src_leaf[FS_FILENAME_SIZE];
    int srcdir_inum = get_inode_of_file_path_dir(src_path, src_leaf);

    // report error if src leaf is "." or ".."
    if (strcmp(src_leaf, ".") == 0 || strcmp(src_leaf, "..") == 0) {
        return EINVAL;
    }

	// get inode and leaf for directory of specified dest path
    char dst_leaf[FS_FILENAME_SIZE];
    int dstdir_inum = get_inode_of_file_path_dir(dst_path, dst_leaf);

    // report error if dst leaf is "." or ".."
    if (strcmp(dst_leaf, ".") == 0 || strcmp(dst_leaf, "..") == 0) {
        return EINVAL;
    }

    // get source and target inode
    int src_inum = get_inode_of_file_path(src_path);
    int dst_inum = get_inode_of_file_path(dst_path);

    /* get source/target directory inode */
    struct fs_inode *din = &fs.inodes[srcdir_inum];

    /* find source directory entry */
    int s_blkno;
    struct fs_dirent s_de[DIRENTS_PER_BLK];
    int s_dirno = get_dir_entry_block(srcdir_inum, s_de, &s_blkno, src_leaf);
    if (s_dirno < 0) {
    	return -ENOENT;  // source does not exist
    }

    /* check target directory entry */
    int t_blkno;
    struct fs_dirent t_de[DIRENTS_PER_BLK];
    int t_dirno = get_dir_entry_block(dstdir_inum, t_de, &t_blkno, dst_leaf);
    // if target exists, check the type and unlink it
    if (t_dirno >= 0) {
    	if (fs.inodes[src_inum].mode != fs.inodes[dst_inum].mode) {
            return -EINVAL; // source and target not the same type
    	}
    	// if target is a directory
    	if (S_ISDIR(fs.inodes[dst_inum].mode)) {
    	    if (!is_dir_empty(dst_inum)) {
                return -EINVAL; // target directory is not empty
    	    }
    	}
    	// unlink the target
    	do_unlink(dst_inum, dstdir_inum, dst_leaf);
    }

    // link src inode to the target directory
    do_mklink(src_inum, dstdir_inum, dst_leaf);

    // unlink the src entry
    do_unlink(src_inum, srcdir_inum, src_leaf);

    return 0;
}

