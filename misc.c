/*
 * file:        misc.c
 *
 * description: various support functions for CS 5600/7600 file system
 *              startup argument parsing and checking, etc.
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019
 */

#define _XOPEN_SOURCE 500
#define _ATFILE_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>
#include <fuse.h>

#include "split.h"
#include "max.h"
#include "image.h"
#include "fsx600.h"		/* only for certain constants */

// should be defined in stdio.h but is not on macos
ssize_t getline(char** restrict linep, size_t* restrict linecapp,
         FILE* restrict stream);

// should be defined in string.h but is not on macos
char *strdup(const char* str);

/** All FUSE filesysstem functions accessed through operations structure. */
extern struct fuse_operations fs_ops;

/**  disk block device */
struct blkdev *disk;

struct data {
    char *image_name;
    int   cmd_mode;
} _data;

static void help(){
    printf("Arguments:\n");
    printf(" -cmdline : Enter an interactive REPL that provides a filesystem view into the image\n");
    printf(" -image <name.img> : Use the provided image file that contains the filesystem\n");
}

/*
 * See comments in /usr/include/fuse/fuse_opts.h for details of 
 * FUSE argument processing.
 * 
 *  usage: ./homework -image disk.img [-part #] directory
 *              disk.img  - name of the image file to mount
 *              directory - directory to mount it on
 */
static struct fuse_opt opts[] = {
    {"-image %s", offsetof(struct data, image_name), 0},
    {"-cmdline", offsetof(struct data, cmd_mode), 1},
    FUSE_OPT_END
};

/* Utility functions
 */

/**
 * strmode - translate a numeric mode into a string.
 * Note: simplified implementation of BSD function.
 *
 * @param mode the numeric mode
 * @param buf output buffer
 */
static void _strmode(int mode, char *buf)
{
    char *str = "rwxrwxrwx";
    if (S_ISDIR(mode)) *buf++ = 'd';
    else if (S_ISLNK(mode)) *buf++ = 'l';
    else *buf++ = '-';
    for (int mask = 0400; mask != 0; str++, mask = mask >> 1) {
    	*buf++ = (mask & mode) ? *str : '-';
    }
    *buf++ = ' ';  // last character will always be space
    *buf++ = 0;
}
#define strmode(mode,buf) _strmode(mode,buf)

/**
 * Returns filename component of path string.
 *
 * @param path the path string
 * @return the file name component
 */
static inline char* filename(char* path) {
	char* lastp = strrchr(path, '/');
	return (lastp == NULL) ? path : lastp+1;
}

/** current working directory */
static char cwd[MAXPATHLEN];

/**
 * Update cwd global from global paths[] array elements.
 *
 * @param array of path elements
 * @param depth number of elements
 */
static void update_cwd(char *paths[], int npaths)
{
    char *p = cwd;
    if (npaths == 0) {
    	strcpy(cwd,"/");
    } else {
    	for (int i = 0; i < npaths; i++) {
    		p += sprintf(p, "/%s", paths[i]);
    	}
    }
}

/**
 * Get current working directory.
 */
static const char *get_cwd(void)
{
    return cwd;
}

/**
 * Copy full path name of path to pathbuf. If path
 * starts with '/', copies path to pathbuf. Otherwise,
 * prepends cwd to path separated by '/'.
 *
 * @param path an absolute or relative path
 * @param pathbuf result path buffer must be MAXPATHLEN length
 * @return pointer to path buffer
 */
static char *full_path(const char *path, char *pathbuf) {
	if (*path == '/') {
		strcpy(pathbuf, path);
	} else {
		const char* cwd = get_cwd();
		if (strcmp(cwd,"/") == 0) cwd = "";
		sprintf(pathbuf, "%s/%s", cwd, path);
	}
	return pathbuf;
}

/**
 * Normalize path array to eliminate empty names,
 * and current (.) and parent (..) directory names.
 *
 * @param names array of name elements
 * @param nnames the number of elements
 * @return the new nnames value after normalization and
 *   the new path length
 */
static int normalize_dot_paths(char *names[], int nnames) {
	int dest = 0;
	for (int src = 0; src < nnames; src++) {
		if (strcmp(names[src],".") == 0) {
			// ignore current directory path element
			free(names[src]);  // "." no longer used
			names[src] = NULL;
		} else if (strcmp(names[src], "..") == 0) {
			if (dest > 0) {
				dest--;  // back up one directory element
				free(names[dest]); // names[dest] no longer used
				names[dest] = NULL;
			}
			free(names[src]);  // ".." no longer used
			names[src] = NULL;
		} else {  // add directory element
			if (dest < src) {
				names[dest] = names[src];
				names[src] = NULL;
			}
			dest++;
		}
	}
	return dest;
}


/**
 * Copy full normalized path name of path to pathbuf. If
 * path starts with '/', copies path to pathbuf. Otherwise,
 * prepends cwd to path separated by '/'.
 *
 * @param path an absolute or relative path
 * @param pathbuf result path buffer must be MAXPATHLEN length
 * @return pointer to path buffer
 */
static char *normalized_full_path(const char* path, char* pathbuf) {
	// make full path for path
	full_path(path, pathbuf);

	// split path into path names
    int pathlen = split(pathbuf, NULL, 0, "/");  // count path names
	if (pathlen == 0) {
		strcpy(pathbuf, "/");
	} else {
		// array of path names
		char* names[pathlen];
		split(pathbuf, names, pathlen, "/");

		pathlen = normalize_dot_paths(names, pathlen);
		if (pathlen == 0) {
			strcpy(pathbuf, "/");
		} else {
			// append path elements into pathbuf
			char *bufp = pathbuf;
			for (int i = 0; i < pathlen; i++) {
				bufp += sprintf(bufp, "/%s", names[i]);
			}
		}
		// free allocated tokens
		free_split_tokens(names, pathlen);
	}

    return pathbuf;
}

/**
 * Change directory.
 *
 * @param argv arg[0] is new directory
 */
static int do_cd1(char *argv[])
{
    struct stat sb;
    char pathbuf[MAXPATHLEN];
    normalized_full_path(argv[0], pathbuf);
    int retval = fs_ops.getattr(pathbuf, &sb);
    if (retval == 0) {
		if (S_ISDIR(sb.st_mode)) {
			int nnames = split(pathbuf, NULL, 0, "/");  // get count
			char **names = malloc(nnames * sizeof(char*));

			split(pathbuf, names, nnames, "/");
		    update_cwd(names, nnames);

			// free allocated tokens
			free_split_tokens(names, nnames);

		    free(names);
		    return 0;
		} else {
			return -ENOTDIR;
		}
    } else {
		return -ENOENT;
    }
}

/**
 * Change to root directory.
 */
static int do_cd0(char *argv[])
{
    char *args[] = {"/"};
	return do_cd1(args);
}

/**
 * Print current working directory
 */
static int do_pwd(char *argv[])
{
    printf("%s\n", get_cwd());
    return 0;
}

/** number of directory entries for ls */
static int ls_count = 0;

/** maximum line length of directory entries for ls */
static int ls_maxlen = 0;

/** temp file for directory entries for ls */
static FILE* ls_tmpfile = NULL;

static const char* ls_path = NULL;

/** Initialize ls variables */
static void init_ls(const char* path)
{
	ls_tmpfile = tmpfile();
	ls_path = path;
    ls_count = 0;
    ls_maxlen = 0;
}

/**
 * Filler callback for readdir() short list version.
 * Similar to ls -1. Form of function is specified
 * by Fuse readdir API.
 *
 * @param buf unused
 * @param name the file name
 * @param sb the stat block for file
 * @param off unused
 */
static int filler(void *buf, const char *name, const struct stat *sb, off_t off)
{
	// strip cwd from name
	const char* cwd = get_cwd();
	if (strstr(name, cwd) == name) {
		name += strlen(cwd)+(strcmp(cwd,"/") != 0);
	}

	// string up to tab used for sorting
    int len = fprintf(ls_tmpfile, "%s\t%s\n", name, name);
    ls_maxlen = max(ls_maxlen, len);
    ls_count++;
    return 0;
}

/**
 * Sort and print directory listings.
 */
static void print_ls(void)
{
	rewind(ls_tmpfile);

	// read lines into array of fixed-width lines
	char lsbuf[ls_count][ls_maxlen+1];
	char* line = NULL;
	size_t len;
	for (int i = 0; i < ls_count; i++) {
		getline(&line, &len, ls_tmpfile);
		strcpy(lsbuf[i], line);
	}

	// sort and print lines
    qsort(lsbuf, ls_count, ls_maxlen+1, (void*)strcmp);
    for (int i = 0; i < ls_count; i++) {
    	// skip sorting field up to tab char
    	char* p = strchr(lsbuf[i], '\t');
    	printf("%s", p+1);
    }

    // close and then reset to prevent problems
    fclose(ls_tmpfile);
    ls_tmpfile = NULL;
    ls_count = 0;
    ls_maxlen = 0;
}

/**
 * List directory relative to current working directory.
 *
 * @param argv argv[0] is a directory name
 */
int do_ls1(char *argv[])
{
    struct stat sb;
    char pathbuf[MAXPATHLEN];
    full_path(argv[0], pathbuf);

    // Appending "/." to path causes getattr to resolve
    // symlinks in the path except the leaf, and return
    // the stat buffer for the real directory.
    char pathdotbuf[MAXPATHLEN];
    sprintf(pathdotbuf, "%s/.", pathbuf);
    int retval = fs_ops.getattr(pathdotbuf, &sb);
    if (retval == 0) {  // directory
		struct fuse_file_info info;
		memset(&info, 0, sizeof(struct fuse_file_info));
		if ((retval = fs_ops.opendir(pathbuf, &info)) == 0) {
			init_ls(pathbuf);
			/* read directory information */
			retval = fs_ops.readdir(pathbuf, NULL, filler, 0, &info);
		}
		fs_ops.releasedir(pathbuf, &info);
	} else {
		// use the actual path to see if it is a file or symlink
		retval = fs_ops.getattr(pathbuf, &sb);
		if (retval == 0) {
			init_ls("/"); // pathbuf is full path so root relative
			/* list file information */
			retval = filler(NULL, pathbuf, &sb, 0);
		}
	}
    if (retval == 0) {
    	/* print file or directory information */
		print_ls();
    }

    return retval;
}

/**
 * List current working directory
 */
static int do_ls0(char *argv[])
{
	char *cwd = strdup(get_cwd());
	int status = do_ls1(&cwd);
	free(cwd);
	return status;
}

/**
 * Filler callback for readdir() long list version.
 * Similar to ls -ils. Form of function is specified
 * by Fuse readdir API.
 *
 *
 * @param buf unused
 * @param name the file name
 * @param sb the stat block for file
 * @param off unused
 */
/**
 * Callback adds directory entry listing to ls buffer.
 * Similar to ls -ils. Form of function is specified
 * by Fuse readdir API.
 */
static int dashl_filler(void *buf, const char *name, const struct stat *sb, off_t off)
{
	// strip cwd from name
	const char* cwd = get_cwd();
	if (strstr(name, cwd) == name) {
		name += strlen(cwd)+(strcmp(cwd,"/") != 0);
	}

    char mode[16], time[26], *lasts;
    strmode(sb->st_mode, mode);
	// string up to tab used for sorting
    int len = fprintf(ls_tmpfile, "%s\t%8lld %5lld %s %2d %4d %4d %8lld %s %s",
    		name, sb->st_ino, sb->st_blocks, mode,
			sb->st_nlink, sb->st_uid, sb->st_gid, sb->st_size,
            strtok_r(ctime_r(&sb->st_mtime,time),"\n",&lasts), name);
    if (S_ISLNK(sb->st_mode)) {
    	// add symlink info to listing if readlink() available
    	if (fs_ops.readlink != NULL) {
    		char fullpath[MAXPATHLEN];
			char sympath[MAXPATHLEN];
			sprintf(fullpath, "%s/%s", strcmp(ls_path,"/") != 0 ? ls_path : "", name);
			int status = fs_ops.readlink(fullpath, sympath, MAXPATHLEN);
			if (status == 0) {
				len += fprintf(ls_tmpfile, " -> %s", sympath);
			}
    	}
    }
    len += fprintf(ls_tmpfile, "\n");  // terminating newline


    ls_maxlen = max(ls_maxlen,len);
    ls_count++;
    return 0;
}


/**
 * Long list of specified directory.
 *
 * @path path of directory to list
 */
static int _lsdashl(const char *path)
{
    struct stat sb;
    char pathbuf[MAXPATHLEN];
    full_path(path, pathbuf);
    int retval = fs_ops.getattr(pathbuf, &sb);
    if (retval == 0) {
		if (S_ISDIR(sb.st_mode)) {
		    struct fuse_file_info info;
		    memset(&info, 0, sizeof(struct fuse_file_info));
			if ((retval = fs_ops.opendir(pathbuf, &info)) == 0) {
				init_ls(pathbuf);
				/* read directory information */
				retval = fs_ops.readdir(pathbuf, NULL, dashl_filler, 0, &info);
			}
			fs_ops.releasedir(pathbuf, &info);
		} else {
			init_ls("/"); // pathbuf is full path so root relative
			/* read file information */
			retval = dashl_filler(NULL, pathbuf, &sb, 0);
		}
		/* print file or directory information */
		print_ls();
    }
    return retval;
}

/**
 * Long list of specified directory
 * relative to current directory.
 *
 * @param argv argv[0] is directory name
 */
static int do_lsdashl1(char *argv[])
{
    char pathbuf[MAXPATHLEN];
    full_path(argv[0], pathbuf);
    return _lsdashl(pathbuf);
}

/**
 * Long list of current directory.
 *
 * @param argv unused
 */
static int do_lsdashl0(char *argv[])
{
	char *cwd = strdup(get_cwd());
    int status = do_lsdashl1(&cwd);
    free(cwd);
    return status;
}

/**
 * Modify the mode of a file or directory name specified
 * by argv[0] using the mode string specified by argv[1].
 * The file or directory name will be interpreted relative
 * to the current working directory.
 *
 * @param argv argv[0] is mode, argv[1] is a file or
 * directory name
 */
static int do_chmod(char *argv[])
{
    char path[MAXPATHLEN];
    int mode = strtol(argv[0], NULL, 8);
    full_path(argv[1], path);
    return fs_ops.chmod(path, mode);
}

/**
 * Create a hard link to a file.
 *
 * @param argv argv[0] is name, arg[1] is link name
 */
static int do_link(char *argv[])
{
	if (fs_ops.link == NULL) {
		// not implemented
		return -ENOTSUP;
	}

    char p1[MAXPATHLEN], p2[MAXPATHLEN];
    full_path(argv[0], p1);
    full_path(argv[1], p2);
    return fs_ops.link(p1, p2);
}

/**
 * Create a symbolic link to a file.
 *
 * @param argv argv[0] is name, arg[1] is link name
 */
static int do_symlink(char *argv[])
{
	if (fs_ops.symlink == NULL) {
		// not implemented
		return -ENOTSUP;
	}

    char p1[MAXPATHLEN], p2[MAXPATHLEN];
    full_path(argv[0], p1);
    full_path(argv[1], p2);
    return fs_ops.symlink(p1, p2);
}

/**
 * Read path from symlink
 *
 * @param argv arg[0] is file name; file system name
 *   relative to current directory
 */
static int do_readlink(char *argv[])
{
	if (fs_ops.readlink == NULL) {
		// not implemented
		return -ENOTSUP;
	}

	char path[MAXPATHLEN];
    full_path(argv[0], path);

    char sympath[MAXPATHLEN];
    int status = fs_ops.readlink(path, sympath, MAXPATHLEN);
    if (status == 0) {
    	fprintf(stdout, "%s\n", sympath);
    }
    return status;
}

/**
 * Rename directory.
 *
 * @param argv argv[0] is old name, arg[1] is new name
 *   relative to working directory
 */
static int do_rename(char *argv[])
{
    char p1[MAXPATHLEN], p2[MAXPATHLEN];
    full_path(argv[0], p1);
    full_path(argv[1], p2);
    return fs_ops.rename(p1, p2);
}

/**
 * Make directory.
 *
 * @param argv argv[0] is directory name
 */
static int do_mkdir(char *argv[])
{
    char path[MAXPATHLEN];
    full_path(argv[0], path);
    return fs_ops.mkdir(path, 0777);
}

/**
 * Remove a directory.
 *
 * @param argv argv[0] is directory name
 */
static int do_rmdir(char *argv[])
{
    char path[MAXPATHLEN];
    full_path(argv[0], path);
    return fs_ops.rmdir(path);
}

/**
 * Remove a file.
 *
 * @param argv argv[0] is file name
 */
static int do_rm(char *argv[])
{
    char path[MAXPATHLEN];
    full_path(argv[0], path);
    return fs_ops.unlink(path);
}

static int blksiz;		/* size of block buffer */
static char *blkbuf;	/* block buffer for coping files */

/**
 * Copy a file from localdir into filesystem
 *
 * @param argv arg[0] is local file, argv[1] is
 *   filesystem file name
 */
static int do_put(char *argv[])
{
    char *outside = argv[0], *inside = argv[1];
    char path[MAXPATHLEN];
    int len, fd, offset = 0, val;

    if ((fd = open(outside, O_RDONLY, 0)) < 0) {
    	return fd;
    }
    full_path(inside, path);
    if ((val = fs_ops.mknod(path, 0777, 0)) != 0) {
    	return val;
    }
    
    struct fuse_file_info info;
    memset(&info, 0, sizeof(struct fuse_file_info));
    if ((val = fs_ops.open(path, &info)) != 0) {
    	return val;
    }
    while ((len = read(fd, blkbuf, blksiz)) > 0) {
    	val = fs_ops.write(path, blkbuf, len, offset, &info);
    	if (val != len) {
    		break;
    	}
    	offset += len;
    }
    close(fd);
    fs_ops.release(path, &info);
    return (val >= 0) ? 0 : val;
}

/**
 * Copy a file from localdir into file system with
 * same name.
 *
 * @param argv arg[0] is file name; file system name
 *   relative to current directory
 */
static int do_put1(char *argv[])
{
    char *args2[] = {argv[0], filename(argv[0])};
    return do_put(args2);
}

/**
 * Creates regular file or set access and modification time.
 *
 * @param argv argv[0] is file name relative
 *   to current directory
 */
static int do_touch(char *argv[])
{
    char path[MAXPATHLEN];
    full_path(argv[0], path);
    // try creating new file
    int status = fs_ops.mknod(path, 0777, 0);
    if (status == -EEXIST) {
    	// if exists, modify its access/mod time to now
    	struct utimbuf ut;
    	ut.actime = ut.modtime = time(NULL);
    	status =fs_ops.utime(path, &ut);
    }
    return status;
}

/**
 * Copy a file from filesystem to localdir
 *
 * @param argv arg[0] is filesystem file name,
 *   argv[1] is local file
 */
static int do_get(char *argv[])
{
    char *inside = argv[0], *outside = argv[1];
    char path[MAXPATHLEN];
    int len, fd, offset = 0;

    if ((fd = open(outside, O_WRONLY|O_CREAT|O_TRUNC, 0777)) < 0) {
    	return fd;
    }
    full_path(inside, path);
    struct fuse_file_info info;
    memset(&info, 0, sizeof(struct fuse_file_info));

    int val = fs_ops.open(path, &info);
    if (val != 0) {
    	fs_ops.release(path, &info);
    	close(fd);
    	return val;
    }
    while (1) {
        len = fs_ops.read(path, blkbuf, blksiz, offset, &info);
		if (len >= 0) {
			len = write(fd, blkbuf, len);
			if (len <= 0) {
				break;
			}
			offset += len;
		}
    }
    close(fd);
    fs_ops.release(path, &info);
    return (len >= 0) ? 0 : len;
}

/**
 * Copy a file from filesystem to localdir with
 * same name.
 *
 * @param argv arg[0] is file name; fileystem name
 *   relative to current directory
 */
static int do_get1(char *argv[])
{
    char *args2[] = {argv[0], filename(argv[0])};
    return do_get(args2);
}

/**
 * Retrieve and print a file.
 *
 * @param argv argv[0] is file name
 */
static int do_show(char *argv[])
{
    char path[MAXPATHLEN];
    int len, offset = 0;

    full_path(argv[0], path);
    struct fuse_file_info info;
    memset(&info, 0, sizeof(struct fuse_file_info));
    int val;
    if ((val = fs_ops.open(path, &info)) != 0) {
    	return val;
    }
    while ((len = fs_ops.read(path, blkbuf, blksiz, offset, &info)) > 0) {
    	fwrite(blkbuf, len, 1, stdout);
    	offset += len;
    }
    fs_ops.release(path, &info);
    return (len >= 0) ? 0 : len;
}

/**
 * Print filesystem statistics
 *
 * @argv unused
 */
static int do_statfs(char *argv[])
{
    struct statvfs st;
    int retval = fs_ops.statfs("/", &st);
    if (retval == 0) {
    	printf("block size: %lu\n", st.f_bsize);
    	printf("no. blocks: %u\n", st.f_blocks);
    	printf("no. free blocks: %u\n", st.f_bfree);
    	printf("no. inodes: %u\n", st.f_files);
    	printf("no. free inodes: %u\n",  st.f_ffree);
    	printf("max name length: %lu\n", st.f_namemax);
    }
    return retval;
}

/**
 * Print files statistics
 *
 * @param argv argv[0] is file name relative
 *   to current directory
 */
static int do_stat(char *argv[])
{
    char path[MAXPATHLEN];
    full_path(argv[0], path);

    struct stat sb;
    int retval = fs_ops.getattr(path, &sb);
    if (retval == 0) {
        char mode[16], time[26], *lasts;
        strmode(sb.st_mode, mode);
        printf("%8lld %5lld %s %2d %4d %4d %8lld %s %s\n",
        		sb.st_ino, sb.st_blocks, mode,
    			sb.st_nlink, sb.st_uid, sb.st_gid, sb.st_size,
                strtok_r(ctime_r(&sb.st_mtime,time),"\n",&lasts), argv[0]);
        return 0;
    }
    return retval;
}

/**
 * Set read/write block size
 *
 * @param size read/write block size
 */
static void _blksiz(int size)
{
    blksiz = size;	// record new block size
    if (blkbuf != NULL) {	// free old block buffer
    	free(blkbuf);
    }
    blkbuf = malloc(blksiz);	// create new block buffer
    printf("read/write block size: %d\n", blksiz);
}

/**
 * Set read/write block size
 *
 * @param argv argv[0] is block size as string
 */
static int do_blksiz(char *argv[])
{
    _blksiz(atoi(argv[0]));
    return 0;
}

/**
 * Truncate file to specified length
 *
 * @param argv argv[0] is file name relative
 *   to current directory
 */
static int do_truncate(char *argv[])
{
    char path[MAXPATHLEN];
    full_path(argv[0], path);
	int len = atoi(argv[1]);
    return fs_ops.truncate(path, len);
}

/**
 * Truncate file to 0 length.
 *
 * @param argv argv[0] is file name relative
 *   to current directory
 */
static int do_truncate1(char *argv[])
{
    char *args2[] = {argv[0], "0"};
    return do_truncate(args2);
}

/**
 * Set access and modification time.
 *
 * @param argv argv[0] is file name relative
 *   to current directory
 */
static int do_utime(char *argv[])
{
    struct utimbuf ut;
    char path[MAXPATHLEN];
    full_path(argv[0], path);
    ut.actime = ut.modtime = time(NULL);	// set access time to now
    return fs_ops.utime(path, &ut);
}

/** cmd stack size */
#define CMDSTKMAX 10

/** cmd stack array */
static FILE* cmdstk[CMDSTKMAX];

/** index of top FILE* on cmd stack */
static int cmdstk_top = -1;

/**
 * Take further input from a command file
 *
 * @param argv argv[0] is a command file name.
 */
static int do_run(char *argv[]) {
	FILE* cmdfile = fopen(argv[0], "r");
	if (cmdfile == NULL) {
		printf("Cannot read command file %s\n", argv[0]);
		return -ENOENT;
	}
	if (cmdstk_top+1 >= CMDSTKMAX) {
		return -EMFILE;
	}
	cmdstk[++cmdstk_top] = cmdfile;
	return 0;
}

/** struct serves a dispatch table for commands */
static struct {
    char *name;
    int   nargs;
    int   (*f)(char *args[]);
    char  *help;
} cmds[] = {
	{"blksiz", 1, do_blksiz, "blksiz - set read/write block size"},
	{"cd", 0, do_cd0, "cd - change to root directory"},
    {"cd", 1, do_cd1, "cd <dir> - change to directory"},
    {"chmod", 2, do_chmod, "chmod <mode> <file> - change permissions"},
    {"get", 2, do_get, "get <inside> <outside> - retrieve a file from file system to local directory"},
    {"get", 1, do_get1, "get <name> - ditto, but keep the same name"},
    {"link", 2, do_link, "link <name> <linkname> - create a link to a file"},
    {"ls", 0, do_ls0, "ls - list files in current directory"},
    {"ls", 1, do_ls1, "ls <dir> - list specified directory"},
    {"ls-l", 0, do_lsdashl0, "ls-l - display detailed file listing"},
    {"ls-l", 1, do_lsdashl1, "ls-l <file> - display detailed file info"},
    {"mkdir", 1, do_mkdir, "mkdir <dir> - create directory"},
    {"put", 2, do_put, "put <outside> <inside> - copy a file from localdir into file system"},
    {"put", 1, do_put1, "put <name> - ditto, but keep the same name"},
	{"pwd", 0, do_pwd, "pwd - display current directory"},
    {"readlink", 1, do_readlink, "readlink <name> - read the symlink name"},
    {"rename", 2, do_rename, "rename <oldname> <newname> - rename file"},
    {"rm", 1, do_rm, "rm <file> - remove file"},
    {"rmdir", 1, do_rmdir, "rmdir <dir> - remove directory"},
    {"run", 1, do_run, "run <cmdfile> - run commands in command file"},
    {"show", 1, do_show, "show <file> - retrieve and print a file"},
    {"stat", 1, do_stat, "stat <file> - print file info"},
    {"statfs", 0, do_statfs, "statfs - print file system info"},
    {"symlink", 2, do_symlink, "symlink <name> <linkname> - create a symlink to a file"},
    {"touch", 1, do_touch, "touch <file> - create file or set modified time to current time"},
    {"truncate", 2, do_truncate, "truncate <file> <length> - truncate to length"},
    {"truncate", 1, do_truncate1, "truncate <file> - truncate to zero length"},
    {"utime", 1, do_utime, "utime <file> - set modified time to current time"},
    {0, 0, 0}
};

/**
 * Command loop for interactive command interpreter.
 */
static int cmdloop(void)
{
    char line[MAXPATHLEN];

    update_cwd(NULL, 0);
    cmdstk[++cmdstk_top] = stdin; // push stdin on cmd stack
    
    while (true) {
    	// prompt if input from a tty
		if (isatty(fileno(cmdstk[cmdstk_top]))) {
    		printf("cmd> "); fflush(stdout);
    	}

		if (fgets(line, sizeof(line), cmdstk[cmdstk_top]) == NULL) {
			if (cmdstk_top == 0) { // stack empty -- done
				break;
			}
			pclose(cmdstk[cmdstk_top--]); // pop stack
			continue;
		}

		if (line[0] == '#')	{/* unechoed comment line */
			continue;
		}

		// echo command if input not a tty
		if (!isatty(fileno(cmdstk[cmdstk_top]))) {
			printf("%s", line);
		}

		if (line[0] == '>') { /* echoed comment line */
			continue;
		}

		if (line[0] == '!') {  /* exec external program */
			// cannot execute if command stack full
			if (cmdstk_top+1 >= CMDSTKMAX) {
				errno = EMFILE;
				perror("exec");
				continue;
			}
			// execute command and capture its stdout
			FILE* fp = popen(line+1, "r");
			if (fp == NULL) {
				// error executing the command
				perror("exec");
				continue;
			}
			// push command output stream onto stack
			cmdstk[++cmdstk_top] = fp;
			continue;
		}

		// split input into command and args
		char *args[10];
		int i, nargs = split(line, args, 10, " \t\r\n");
	
		// continue if empty line
		if (nargs == 0) {
			continue;
		}

		// quit if command is "quit" or "exit"
		if ((strcmp(args[0], "quit") == 0) || (strcmp(args[0], "exit") == 0)) {
			// free tokens allocated by split
			free_split_tokens(args, nargs);
			break;
		}

		// provide help if command is "help" or "?"
		if ((strcmp(args[0], "help") == 0) || (strcmp(args[0], "?") == 0)) {
			for (i = 0; cmds[i].name != NULL; i++) {
				printf("%s\n", cmds[i].help);
			}
			// free tokens allocated by split
			free_split_tokens(args, nargs);
			continue;
		}

		// validate command and arguments
		for (i = 0; cmds[i].name != NULL; i++) {
			if ((strcmp(args[0], cmds[i].name) == 0) && (nargs == cmds[i].nargs+1)) {
				break;
			}
		}

		// if command not recognized or incorrect arg count
		if (cmds[i].name == NULL) {
			if (nargs > 0) {
				printf("bad command: %s\n", args[0]);
			}
			// free tokens allocated by split
			free_split_tokens(args, nargs);
			continue;
		}

		// process command
		int err = cmds[i].f(&args[1]);
		if (err != 0) {
			printf("error: %s\n", strerror(-err));
		}

		// free tokens allocated by split
		free_split_tokens(args, nargs);
    }
    return 0;
}


/**************/

/**
 * This function fixes up argv arguments when run under GDB
 * on Eclipse CDT. Eclipse adds single-quotes around all
 * arguments, which must be stripped out.
 *
 * @param argc the number of parameters
 * @param argv the parameter values
 */
static void fixup(int argc, char *argv[]) {
	for (int i = 0; i < argc; i++) {
		size_t len = strlen(argv[i]);
		if (argv[i][0] == '\'' && argv[i][len-1] == '\'') {
			argv[i][len-1] = '\0';
			argv[i]++;
		}
	}
}

int main(int argc, char **argv)
{
	fixup(argc, argv);

    /* Argument processing and checking
     */
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &_data, opts, NULL) == -1){
        help();
        exit(1);
    }

    if (_data.image_name == 0){
        fprintf(stderr, "You must provide an image\n");
        help();
        exit(1);
    }

    char *file = _data.image_name;
    if (strcmp(file+strlen(file)-4, ".img") != 0) {
        fprintf(stderr, "bad image file (must end in .img): %s\n", file);
        help();
        exit(1);
    }

    if ((disk = image_create(file)) == NULL) {
        fprintf(stderr, "cannot open image file '%s': %s\n", file, strerror(errno));
        help();
        exit(1);
    }

    if (_data.cmd_mode) {  /* process interactive commands */
        fs_ops.init(NULL);
        _blksiz(FS_BLOCK_SIZE);
        cmdloop();
        return 0;
    }

    /** pass control to fuse */
    return fuse_main(args.argc, args.argv, &fs_ops, NULL);
}


