/*
 * fs_path.h
 *
 * description: path utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef FS_UTIL_PATH_H_
#define FS_UTIL_PATH_H_

/** File path delimiter character */
extern const char* PATH_DELIM;

/* Return inode number for specified file or directory path.
 * Given "/a/b/c", returns the inode number for "c".
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @return inode of path node or error
 */
int get_inode_of_file_path(const char* path);

/**
 *  Return inode number of directory for specified file
 *  or directory path, and a leaf name that may not yet
 *  exist. Given "/a/b/c", returns inode number for "b"
 *  and the leaf "c".
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @param leaf pointer to space for FS_FILENAME_SIZE leaf name
 * @return inode of path node or -error
 */
int get_inode_of_file_path_dir(const char* path, char* leaf);




/**
 *  Return inode number of directory for specified file
 *  or directory path, and a leaf name that may not yet
 *  exist. Given "/a/b/c", returns inode number for "b"
 *  and the leaf "c".
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @param leaf pointer to space for FS_FILENAME_SIZE leaf name
 * @return inode of path node or -error
 */
int get_inode_of_file_path_dir_sym(char* path, char* leaf);


/* Return inode number for specified file or directory path.
 * Given "/a/b/c", returns the inode number for "c".
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @return inode of path node or error
 */
int get_inode_of_file_path_sym(char* path);

#endif /* FS_UTIL_PATH_H_ */
