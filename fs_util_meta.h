/*
 * fs_metadata.h
 *
 * description: metadata utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef FS_UTIL_META_H_
#define FS_UTIL_META_H_

/**
 * Flush dirty metadata blocks to disk.
 */
void flush_metadata(void);

/**
 * Gets a free block number from the free list.
 *
 * @return free block number or 0 if none available
 */
int get_free_blk(void);

/**
 * Return a block to the free list.
 *
 * @param  blkno the block number
 */
void return_blk(int blkno);

/**
 * Determines whether block with blkno is free.
 *
 * @param blkno the block number
 * @param 1 (true) or 0 (false)
 */
int is_free_blk(int blkno);


/**
 * Gets a free inode number from the free list.
 *
 * @return a free inode number or 0 if none available
 */
int get_free_inode(void);

/**
 * Return a inode to the free list.
 *
 * @param  inum the inode number
 */
void return_inode(int inum);

/**
 * Determines whether inode with inum is free.
 *
 * @param inum the inode number
 * @param 1 (true) or 0 (false)
 */
int is_free_inode(int inum);


/**
 * Mark a inode as dirty.
 *
 * @param inum the number of inode to mark dirty
 */
void mark_inode(int inum);


#endif /* FS_UTIL_META_H_ */
