/*
 * fs_op_mkdir.c
 *
 * description: fs_mkdir function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <fuse.h>

#include "fs_util_file.h"
#include "fsx600.h"

/**
 *  mkdir - create a directory with the given mode. Behavior
 *  undefined when mode bits other than the low 9 bits are used.
 *
 * Errors
 *   -ENOTDIR  - component of path not a directory
 *   -EEXIST   - directory already exists
 *   -ENOSPC   - free inode not available
 *   -ENOSPC   - directory full
 *
 * @param path path to file
 * @param mode the mode for the new directory
 * @return 0 if successful, or -error number
 */
int fs_mkdir(const char* path, mode_t mode)
{
	/* get directory inode */
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_file_path_dir(path,leaf);
    
    //error getting directory inode
    if(dir_inum < 0){
        return dir_inum;
    }
    
    // make regular entry
    int inum = do_mkentry(dir_inum, leaf, mode, S_IFDIR);
    //make . hard link
    do_mklink(inum, inum, ".");
    //make .. hard link
    do_mklink(dir_inum, inum, "..");
    
    return (inum < 0) ? inum : 0;
}


