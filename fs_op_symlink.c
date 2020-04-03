/**
 * link - create a symbolic link to a file.
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_meta.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"

int fs_symlink(const char* path1, const char* path2)
{
	
    char leaf2[FS_FILENAME_SIZE];
    int dir_inum2 = get_inode_of_file_path_dir(path2, leaf2);

    // ensure that inode exists
    if (dir_inum2 < 0) {
        return dir_inum2;
    }
    
    int sym_inum = do_mkentry(dir_inum2, leaf2, 0777, S_IFLNK);
    do_write(sym_inum, path1, strlen(path1),0);
    
    return 0;

}
