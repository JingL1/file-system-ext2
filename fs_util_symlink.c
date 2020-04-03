#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "fs_util_path.h"
#include "fs_util_file.h"
#include "fsx600.h"
#include "fs_util_vol.h"


enum { MAX_CHAIN = 1024 };

int find_source(char* sympath, char *currpath, int current_chain) {
    if (current_chain >= MAX_CHAIN) {
        printf("Find loop in symlink");
        return -ELOOP;
    }

    // get inode number of the sympath;
    int inum = get_inode_of_file_path(sympath);
    // ensure that the inode exists
    if (inum < 0) {
        return -ENOENT;
    }

    // get the leaf and dir_inum
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_file_path_dir(sympath, leaf);
    // error getting directory inode
    if (dir_inum < 0) {
        return dir_inum;
    }

    if (current_chain == -1) {
        return inum;
    }

    struct fs_inode *in = &fs.inodes[inum];

    if (S_ISLNK(in->mode)) {
        char buf[in->size];
        do_read(inum, buf, in->size, 0);
        strncpy(sympath, buf, MAXPATHLEN);
        // symlink can be absolute or relative
        if (sympath[0] != '/') {
            // relative path
            size_t currlen = strlen(currpath);
            size_t leaflen = strlen(leaf);
            currpath[currlen - leaflen] = '\0';
            strncat(currpath, sympath, strlen(sympath));
            strncpy(sympath, currpath, MAXPATHLEN);
        }

        inum = find_source(sympath, currpath, current_chain + 1);
    }
    return inum;
}
