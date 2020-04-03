/*
 * fs_op_statfs.c
 *
 * description: fs_op_statfs function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <sys/statvfs.h>
#include <string.h>

#include "fs_util_meta.h"
#include "fs_util_vol.h"

/**
 * statfs - get file system statistics.
 * See 'man 2 statfs' for description of 'struct statvfs'.
 *
 * Errors
 *   none -  Needs to work
 *
 * @param path the path to the file
 * @param st the statvfs struct
 * @return 0 if successful
 */
int fs_statfs(const char* path, struct statvfs* st)
{
    /* Return the following fields (set others to zero):
     *   f_bsize:	fundamental file system block size
     *   f_blocks	total blocks in file system
     *   f_bfree	free blocks in file system
     *   f_bavail	free blocks available to non-superuser
     *   f_files	total file nodes in file system
     *   f_ffiles	total free file nodes in file system
     *   f_favail	total free file nodes available to non-superuser
     *   f_namelen	maximum length of file name
     */
	memset(st, 0, sizeof(statvfs));

	// compute number of free blocks
	int n_blocks_free = 0;
	for (int i = 0; i < fs.n_blocks; i++) {
		if (is_free_blk(i)) n_blocks_free++;
	}

	// compute number of free inodes
	int n_inodes_free = 0;
	for (int i = 0; i < fs.n_inodes; i++) {
		if (is_free_inode(i)) {
			n_inodes_free++;
		}
	}

	st->f_bsize = FS_BLOCK_SIZE;
    st->f_blocks = fs.n_blocks;
    st->f_bfree = n_blocks_free;
    st->f_bavail = st->f_bfree;
    st->f_files = fs.n_inodes;
    st->f_ffree = n_inodes_free;
    st->f_favail = st->f_ffree;
    st->f_namemax = FS_FILENAME_SIZE-1;

    return 0;
}

