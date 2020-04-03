#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "fs_util_dir.h"
#include "fs_util_path.h"
#include "fs_util_meta.h"
#include "fs_util_vol.h"
#include "blkdev.h"


/**
 * link - create a hard link to a file.
 *
 * Errors:
 *   -ENOENT   - file does not exist
 *   -EISDIR   - component of original path is a directory
 *   -ENOSPC   - free inode not available
 *
 * @param path1 the file to be linked
 * @param path2 the new file
 * @return 0 if successful, or -error number
 */

int fs_link(const char* path1, const char* path2)
{
	// get inode number of specified path
    int inum = get_inode_of_file_path(path1);
    
    if(S_ISDIR(fs.inodes[inum].mode)){
        return -EISDIR;//cannot link a directory
    }

    // get inode and leaf for directory of specified path
    char leaf[FS_FILENAME_SIZE];
    get_inode_of_file_path_dir(path1, leaf);

    // get the new file's name and dir inum
    char newName[FS_FILENAME_SIZE];
    int target_dir_inum = get_inode_of_file_path_dir(path2, newName);

    // ensure that inode exists
    if (inum < 0) {
        return -ENOENT;
    }

    // inode of current dir
    struct fs_inode *din = &fs.inodes[target_dir_inum];

    // get directory entry set
    int blkno;
    char buf[FS_BLOCK_SIZE];
    get_dir_entry_block(target_dir_inum, buf, &blkno, newName);

	// find free directory entry
	int entno = get_dir_free_entry_block(target_dir_inum, buf, &blkno);
	if (entno < 0) {
		return -ENOSPC;	// no free directory entry
    }

	// set directory entry information
	struct fs_dirent *de = (void*)buf;
	set_dir_entry(&de[entno], inum, newName);

    // write updated directory block to disk
    disk->ops->write(disk, blkno, 1, buf);

    // increment size of directory by one fs_dirent
    din->size += sizeof(struct fs_dirent);

    // flush metadata updates
    flush_metadata();

    return 0;

}
