#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */
#define MAX_PATH_COUNT 20       /* Number of sub dir max limit in a path name */

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size, bool type_dir);
struct file *filesys_open (const char *name,bool type_dir, bool* warning);
bool filesys_remove (const char *name);

#endif /* filesys/filesys.h */
