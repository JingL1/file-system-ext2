/**
 * Read path from symlink
 *
 * Errors:
 *   -ENOENT   - file does not exist
 *   -EISDIR   - component of original path is a directory
 *   -ENOSPC   - free inode not available
 *
 * @param path1 the file to be linked
 * @param path2 the symlink name
 * @param how long to be read
 *
 * @return 0 if successful, or -error number
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_meta.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"

enum { MAX_PATH = 4096, MAX_CHAIN = 1024 };

int fs_readlink(const char* path, char* sympath, size_t len)
{

    char path_var[MAX_PATH];
    strncpy(path_var, path, MAX_PATH);
    
    // get inode number of specified path
    int inum = get_inode_of_file_path_sym(path);

    // ensure that inode exists
    if (inum < 0) {
        return -ENOENT;
    }

    // thanks to our design, the sympath here is also a full_path
    do_read(inum, sympath, len, 0);
    int namelen = fs.inodes[inum].size;
    sympath[namelen] = '\0';

    // find non-symlink file recursively
    //find_source(sympath, 0);

    return 0;
}

