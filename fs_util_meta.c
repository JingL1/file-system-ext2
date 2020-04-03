/*
 * fs_util_meta.c
 *
 * description: metadata utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>

#include "fs_util_meta.h"
#include "fs_util_vol.h"
#include "blkdev.h"

/**
 * Flush dirty metadata blocks to disk.
 */
void flush_metadata(void)
{
    for (int i = 0; i < fs.n_meta; i++) {
        if (fs.dirty[i] != NULL) {
            disk->ops->write(disk, i, 1, fs.dirty[i]);
            fs.dirty[i] = NULL;
        }
    }
}

/**
 * Gets a free block number from the free list.
 *
 * @return free block number or 0 if none available
 */
int get_free_blk(void)
{
    for (int i = 0; i < fs.n_blocks; i++) {
        if (!FD_ISSET(i, fs.block_map)) {
        	// mark block allocated
            FD_SET(i, fs.block_map);

            // mark block map block dirty
            int n = i / BITS_PER_BLK;
            fs.dirty[fs.block_map_base + n] = (void*)fs.block_map + n*FS_BLOCK_SIZE;
            return i;
        }
    }
    return 0;
}

/**
 * Return a block to the free list.
 *
 * @param  blkno the block number
 */
void return_blk(int blkno)
{
	// mark block free
    FD_CLR(blkno, fs.block_map);

    // mark block map block dirty
    int n = blkno / BITS_PER_BLK;
    fs.dirty[fs.block_map_base + n] = (void*)fs.block_map + n*FS_BLOCK_SIZE;
}

/**
 * Determines whether block with blkno is free.
 *
 * @param blkno the block number
 * @param 1 (true) or 0 (false)
 */
int is_free_blk(int blkno) {
	return (FD_ISSET(blkno, fs.block_map) == 0);

}

/**
 * Determines whether inode with inum is free.
 *
 * @param inum the inode number
 * @param 1 (true) or 0 (false)
 */
int is_free_inode(int inum) {
	return (FD_ISSET(inum, fs.inode_map) == 0);
}

/**
 * Gets a free inode number from the free list.
 *
 * @return a free inode number or 0 if none available
 */
int get_free_inode(void)
{
    for (int i = 0; i < fs.n_inodes; i++) {
        if (!FD_ISSET(i, fs.inode_map)) {
        	// mark inode allocated
            FD_SET(i, fs.inode_map);

            // mark inode map block dirty
            int n = i / BITS_PER_BLK;
            fs.dirty[fs.inode_map_base + n] = (void*)fs.inode_map + n*FS_BLOCK_SIZE;
            return i;
        }
    }
    return 0;
}

/**
 * Return a inode to the free list.
 *
 * @param  inum the inode number
 */
void return_inode(int inum)
{
	// mark inode free
    FD_CLR(inum, fs.inode_map);

    // mark inode map block dirty
    int n = inum / BITS_PER_BLK;
    fs.dirty[fs.inode_map_base + n] = (void*)fs.inode_map + n*FS_BLOCK_SIZE;
}

/**
 * Mark a inode as dirty.
 *
 * @param inum the number of inode to mark dirty
 */
void mark_inode(int inum)
{
	// mark inode block dirty
    long n = inum / INODES_PER_BLK;
    fs.dirty[fs.inode_base + n] = (void*)fs.inodes + n*FS_BLOCK_SIZE;
}

