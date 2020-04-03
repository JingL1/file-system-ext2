/*
 * file:        image.h
 *
* description: block device creation function for CS 7600 / CS 5600 file system
  *
 * Peter Desnoyers, Northeastern Computer Science, 2015
 * Philip Gust, Northeastern Computer Science, 2019
 */

#ifndef IMAGE_H_
#define IMAGE_H_

#include "blkdev.h"

/**
 * Create an image block device reading from a specified image file.
 *
 * @param path the path to the image file
 * @return the block device or NULL if cannot open or read image file
 */
extern struct blkdev *image_create(char *path);


#endif /* IMAGE_H_ */
