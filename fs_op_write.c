/*
 * fs_op_write.c
 *
 * description: fs_write function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <errno.h>
#include <fuse.h>
#include <string.h>

#include "fs_util_file.h"
#include "fs_util_meta.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"


/**
 *  write - write data to a file
 *
 * It should return exactly the number of bytes requested, except on
 * error.
 *
 * Errors:
 *   -ENOENT  - file does not exist
 *   -ENOTDIR - component of path not a directory
 *   -EISDIR  - file is a directory
 *   -EINVAL  - if 'offset' is greater than current file length.
 *  			(POSIX semantics support the creation of files with
 *  			"holes" in them, but we don't)
 *
 * @param path the file path
 * @param buf the buffer to write
 * @param len the number of bytes to write
 * @param offset the offset to starting writing at
 * @param fi the Fuse file info for writing
 * @return number of bytes actually written if successful, or -error number
 */
int fs_write(const char* path, const char* buf, size_t len,
		     off_t offset, struct fuse_file_info* fi)
{
    int inum;
    if (fi != NULL) {
    	// get inode stored in fi->fh by fs_open
        inum = fi->fh;
    } else {
    	// get inode for specified path
        inum = get_inode_of_file_path(path);

        // return error code if error
        if (inum < 0) {
            return inum;
        }
    }
    /* cannot write if it is directory */
    if (S_ISDIR(fs.inodes[inum].mode)) {
        return -EISDIR;
    }

    int nwritten = do_write(inum, buf, len, offset);
    return nwritten;
}

