/*
 * fs_op_truncate.c
 *
 * description: fs_truncate function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "fs_util_file.h"
#include "fs_util_meta.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"

/**
 * truncate - truncate file to exactly 'len' bytes.
 *
 * Errors:
 *   ENOENT  - file does not exist
 *   ENOTDIR - component of path not a directory
 *   EINVAL  - length invalid (only supports 0)
 *   EISDIR	 - path is a directory (only files)
 *
 * @param path the file path
 * @param len the length
 * @return 0 if successful, or -error number
 */
int fs_truncate(const char* path, off_t len)
{
    /* you can cheat by only implementing this for the case of len==0,
     * and an error otherwise.
     */

	// get inode for specified path
    int inum = get_inode_of_file_path(path);

    // ensure that inode exists
    if (inum < 0) {
        return inum;
    }

    /* cannot truncate if it is directory */
    if (S_ISDIR(fs.inodes[inum].mode)) {
        return -EISDIR;
    }

    // truncate file
    int val = do_truncate(inum, len);
    if (val >= 0) {
        // mark inode dirty and flush metadata blocks
        mark_inode(inum);
        flush_metadata();
    }

    return val;
}
