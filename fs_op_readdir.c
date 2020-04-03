/*
 * fs_op_readdir.c
 *
 * description: fs_readdir function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <errno.h>
#include <fuse.h>

#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"

/**
 * readdir - get directory contents.
 *
 * For each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the directory path
 * @param ptr  filler buf pointer
 * @param filler filler function to call for each entry
 * @param offset the file offset -- unused
 * @param fi the fuse file information
 * @return 0 if successful, or -error number
 */
int fs_readdir(const char* path, void *ptr, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info* fi)
{
	// get inum of inode for path
    int inum;
    if (fi != NULL)
    	// get inode from fi-> placed by fs_opendir()
        inum = fi->fh;
    else {
    	// get inode for specified path
        inum = get_inode_of_file_path(path);

        // report error if does not exist
        if (inum < 0) {
            return inum;
        }
    }

    // cannot read if it is not a directory
    if (!S_ISDIR(fs.inodes[inum].mode)) {
        return -ENOTDIR;
    }

    // process each directory entry
    for (int blkindex = 0; ; blkindex++) {
    	// get block no of n-th directory block
        char buf[FS_BLOCK_SIZE];
        int blkno = get_file_blk(inum, blkindex, buf, 0);
        if (blkno == 0) {
        	break;
        } else if (blkno < 0) {
        	return -EIO;
        }

    	// call filler function for each entry
        struct fs_dirent* de = (void*)buf;
    	for (int i = 0; i < DIRENTS_PER_BLK; i++) {
    		if (de[i].valid) {
    			struct stat sb;
    			do_stat(de[i].inode, &sb);
    			filler(ptr, de[i].name, &sb, 0);
    		}
    	}
    }

    return 0;
}


