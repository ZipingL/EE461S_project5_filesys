#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include <list.h>

#define DIRECT_BLOCK_SIZE 118
#define INDIRECT_BLOCK_SIZE 128
#define DBINDIRECT_BLOCK_SIZE 128


struct bitmap;

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    block_sector_t parent;
    bool type_dir;
    unsigned magic;                     /* Magic number. */
	uint32_t numDirect;					/* Number of allocated direct blocks */
	uint32_t numIndirect;				/* Number of allocated indirect blocks */
  uint32_t numDbIndirect;
	block_sector_t direct[118];			/* Holds pointers to free sectors */
	block_sector_t indirect_ptr;		/* Holds a pointer to a sector that will point to free sectors */
	block_sector_t db_indirect_ptr;	/* Points to a sector that points to a sector that points to free blocks (?) */
    //uint32_t unused[120];             /* Not used. If you add a field, subtract one from the array size */
  };

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    block_sector_t parent;
    bool type_dir;
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
	off_t length;						/* The same as the length in inode_disk, gets updated in inode_write and inode_create */
    struct inode_disk data;             /* Inode content. */
  };

struct indirect_block {
	block_sector_t ind_ptrs[128];
};


void inode_init (void);
bool inode_create (block_sector_t, off_t, bool type_dir);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

#endif /* filesys/inode.h */
