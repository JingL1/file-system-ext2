/*
 * fs_ops.c
 *
 * description: fuse operations for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <fuse.h>

#include "fs_ops.h"

/**
 * Operations vector. Please don't rename it, as the
 * code in misc.c assumes it is named 'fs_ops'.
 */
struct fuse_operations fs_ops = {
    .chmod = fs_chmod,
    .getattr = fs_getattr,
    .init = fs_init,
    .mkdir = fs_mkdir,
    .mknod = fs_mknod,
    .open = fs_open,
    .opendir = fs_opendir,
    .read = fs_read,
    .readdir = fs_readdir,
    .release = fs_release,
    .releasedir = fs_releasedir,
    .rename = fs_rename,
    .rmdir = fs_rmdir,
    .statfs = fs_statfs,
    .truncate = fs_truncate,
    .unlink = fs_unlink,
    .utime = fs_utime,
    .write = fs_write,
    .symlink = fs_symlink,
    .link = fs_link,
    .readlink = fs_readlink,
};


