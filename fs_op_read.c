/*
 * fs_read.c
 *
 * description: fs_read function for CS 5600 / 7600 file system
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
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"

/**
 * read - read data from an open file.
 *
 * Should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return bytes from offset to EOF
 *   - on error, return <0
 *
 * Errors:
 *   -ENOENT  - file does not exist
 *   -ENOTDIR - component of path not a directory
 *   -EISDIR  - file is a directory
 *   -EIO     - error reading block
 *
 * @param path the path to the file
 * @param buf the read buffer
 * @param len the number of bytes to read
 * @param offset to start reading at
 * @param fi fuse file info
 * @return number of bytes actually read if successful, or -error number
 */
int fs_read(const char* path, char* buf, size_t len, off_t offset,
		    struct fuse_file_info* fi)
{
    int inum;

    if (fi != NULL) {
    	// get inode stored in fi->fh by fs_open()
        inum = fi->fh;
    } else {
    	// get inode for specified path
        inum = get_inode_of_file_path(path);

        // report error if error
        if (inum < 0) {
            return inum;
        }
    }

    /* cannot read if it is directory */
    if (S_ISDIR(fs.inodes[inum].mode)) {
    	return -EISDIR;
    }

    // read bytes of inode
    int nread = do_read(inum, buf, len, offset);
    return nread;
}


