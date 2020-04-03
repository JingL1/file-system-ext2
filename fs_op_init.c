/*
 * fs_op_init.c
 *
 * description: init function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <fuse.h>

#include "fs_util_vol.h"
#include "blkdev.h"

/** Instance of ex2 fs structure */
struct ext2_fs fs;

/**
 * init - this is called once by the FUSE framework at startup.
 *
 * This is a good place to read in the super-block and set up any
 * global variables you need. You don't need to worry about the
 * argument or the return value.
 *
 * @param conn fuse connection information - unused
 * @return unused - returns NULL
 */
void* fs_init(struct fuse_conn_info* conn)
{
	// read the superblock
    struct fs_super sb;
    if (disk->ops->read(disk, 0, 1, &sb) < 0) {
        exit(1);
    }

    // record root inode
    fs.root_inode = sb.root_inode;

    /* The inode map and block map are written directly to the disk after the superblock */

    // read inode map
    fs.inode_map_base = 1;
    fs.inode_map = malloc(sb.inode_map_sz * FS_BLOCK_SIZE);
    if (disk->ops->read(disk, fs.inode_map_base, sb.inode_map_sz, fs.inode_map) < 0) {
        exit(1);
    }

    // read block map
    fs.block_map_base = fs.inode_map_base + sb.inode_map_sz;
    fs.block_map = malloc(sb.block_map_sz * FS_BLOCK_SIZE);
    if (disk->ops->read(disk, fs.block_map_base, sb.block_map_sz, fs.block_map) < 0) {
        exit(1);
    }

    /* The inode data is written to the next set of blocks */
    fs.inode_base = fs.block_map_base + sb.block_map_sz;
    fs.n_inodes = sb.inode_region_sz * INODES_PER_BLK;
    fs.inodes = malloc(sb.inode_region_sz * FS_BLOCK_SIZE);
    if (disk->ops->read(disk, fs.inode_base, sb.inode_region_sz, fs.inodes) < 0) {
        exit(1);
    }

    // number of metadata blocks
    fs.n_meta = fs.inode_base + sb.inode_region_sz;

    // number of blocks on device
    fs.n_blocks = sb.num_blocks;

    // allocate dirty metadata blocks
    fs.dirty = calloc(fs.n_meta, sizeof(void*));  // ptrs to dirty metadata blks

    return NULL;
}

