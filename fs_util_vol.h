/*
 * fs_util_vol.h
 *
 * description: volume definition for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef FS_UTIL_VOL_H_
#define FS_UTIL_VOL_H_

#include <sys/select.h>

#include "fsx600.h"

/**
 * disk access - the global variable 'disk' points to a blkdev
 * structure which has been initialized to access the image file.
 *
 * NOTE - blkdev access is in terms of BLOCK_SIZE byte blocks
 */
extern struct blkdev *disk;


/** information about ext2 fs volume */
struct ext2_fs {
	/** number of metadata blocks */
	int n_meta;

	/** blkno of first inode map block */
	int inode_map_base;

	/** pointer to inode bitmap to determine free inodes */
	fd_set *inode_map;

	/** number of inodes from superblock */
	int n_inodes;

	/** blkno of first inode block */
	int inode_base;

	/** pointer to inode blocks */
	struct fs_inode *inodes;

	/** number of root inode from superblock */
	int root_inode;

	/** blkno of first data block */
	int block_map_base;

	/** pointer to block bitmap to determine free blocks */
	fd_set *block_map;

	/** number of available blocks from superblock */
	int n_blocks;

	/** array of dirty metadata blocks to write */
	void **dirty;
};

/** Instance of ex2 fs structure */
extern struct ext2_fs fs;



#endif /* FS_UTIL_VOL_H_ */
