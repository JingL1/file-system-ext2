/*
 * fs_util_dir.c
 *
 * description: directory utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_meta.h"
#include "fs_util_vol.h"
#include "blkdev.h"

/*
 * Note on path translation errors:
 * In addition to method-specific errors listed below,
 * almost every method can return one of the following
 * errors if it fails to locate a file or directory
 * corresponding to a specified path.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - a component of path not a directory
 */

/**
 * Determines whether directory block is empty.
 *
 * * Errors
 *   -EIO  - error reading directory block
 *
 * @param de ptr to first entry in directory
 * @return 1 (true) if empty, 0 (false) if has entries
 *   or - error
 */
int is_dir_empty(int inum)
{
    // ensure that inode for inum is a directory
    if (!S_ISDIR(fs.inodes[inum].mode)) {
        return -ENOTDIR;
    }

    // read first directory block for inode
    int blkno = get_file_blkno(inum, 0, 0);
    // empty if not present
    if (blkno == 0) {
    	return 1;
    }

    // relies on size field being accurate
    return (fs.inodes[inum].size == 2 * sizeof(struct fs_dirent));
}

/**
 * Look up a single directory entry in a directory block.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *
 * @param de storage for a directory block
 * @param name the name of the entry
 * @return entry in directory block
 */
int get_dir_entry_in_block(struct fs_dirent* de, const char* name) {
    for (int entno = 0; entno < DIRENTS_PER_BLK; entno++) {
        if (de[entno].valid && (strcmp(de[entno].name, name) == 0)) {
            return entno;
        }
    }

    // entry not found
    return -ENOENT;
}

/**
 * Look up a free directory entry in a directory block.
 *
 * Errors
 *   -ENNOSPC  - free entry not found
 *
 * @param de storage for a directory block
 * @param name the name of the entry
 * @return entry in directory block
 */
int get_free_entry_in_block(struct fs_dirent* de) {
    for (int entno = 0; entno < DIRENTS_PER_BLK; entno++) {
        if (!de[entno].valid) {
            return entno; // return entry no in block
        }
    }

    // entry not found
    return -ENOSPC;
}

/**
 * Look up a single directory entry in a directory.
 *
 * Errors
 *   -EIO     - error reading block
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - intermediate component of path not a directory
 *
 * @param inum the inode number of a directory
 * @param block storage for a directory block
 * @param blkno block no for a directory block
 * @param name the name of the entry
 * @return entry in block returned through block, or -error
 */
int get_dir_entry_block(
                        int inum, void *block, int* blkno, const char* name)
{
    // ensure that inode for inum is a directory
    if (!S_ISDIR(fs.inodes[inum].mode)) {
        *blkno = 0;  // no block
        memset(block, 0, FS_BLOCK_SIZE);
        return -ENOTDIR;
    }
    
    // get  block of directory
    int dir_blkno, entry_no;
    for (int blkindex = 0; ;blkindex++){
        dir_blkno = get_file_blk(inum, blkindex, block, 0);//no extend
        if (dir_blkno <= 0) {
            *blkno = 0;  // no block
            memset(block, 0, FS_BLOCK_SIZE);
            return (dir_blkno == 0) ? -ENOENT : -EIO;
        }
        entry_no = get_dir_entry_in_block(block, name);
        if (entry_no >= 0){
            break;
        }
    }
    
    if (entry_no < 0) {
        *blkno = 0;  // no block
        memset(block, 0, FS_BLOCK_SIZE);
    } else {
        *blkno = dir_blkno;
    }
    return entry_no;
}


/**
 * Find block with free entry in directory.
 *
 * Errors
 *   -EIO     - error reading block
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - intermediate component of path not a directory
 *   -ENOSPC  - cannot allocate space for free entry
 *
 * @param inum the inode number of a directory
 * @param block storage for a directory block
 * @param blkno block no returned for a directory block
 * @return entry in block returned through block, or -error
 */
int get_dir_free_entry_block(int inum, void* block, int* blkno)
{
    int dir_blkno, entry_no; // extend
    //search through all blocks until get one free block
    for (int blkindex = 0; ;blkindex++){
        //set block
        dir_blkno = get_file_blk(inum, blkindex, block, 1);
        if (dir_blkno <= 0) {
            memset(block, 0, FS_BLOCK_SIZE);
            return (dir_blkno == 0) ? -ENOSPC : -EIO;
        }
        // find free entry in block
        entry_no = get_free_entry_in_block(block);
        if (entry_no >= 0 ){
            break;
        }
    }
    // return block no and entry no
    *blkno = dir_blkno;
    return entry_no;
}

/**
 * Look up a single directory entry in a directory.
 *
 * Errors
 *   -EIO     - error reading block
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - intermediate component of path not a directory
 *
 * @param inum the inode number of a directory
 * @param name the name of the entry
 * @return inode number of the entry or -error
 */
int get_dir_entry_inode(int inum, const char* name)
{
    char buf[FS_BLOCK_SIZE];

    // get block and entry number of name in directory
    int blkno;
    int entno = get_dir_entry_block(inum, buf, &blkno, name);

    // return inode of entry if found or error returned
    struct fs_dirent* de =(void*)buf;
    return (entno < 0) ? entno : de[entno].inode;
}

/**
 * Sets directory entry with inum and name.
 *
 * @param de the directory entry
 * @param inum the inode number
 * @param the entry name
 */
void set_dir_entry(struct fs_dirent* de, int inum, const char* name) {
    // fill in entry info for new entry in directory
    de->valid = 1;
    de->isDir = S_ISDIR(fs.inodes[inum].mode);  // true if inode mode is directory
    // truncates leaf at FS_FILENAME_SIZE-1, then '\0'
    strncpy(de->name, name, FS_FILENAME_SIZE-1);

    // add inode to directory
    de->inode = inum;
    fs.inodes[inum].nlink++;  // increase reference count if inode for this entry
    mark_inode(inum);
}
