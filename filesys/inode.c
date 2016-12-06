#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
//#define FILESYS_DEBUG 2
#define DIRECT_BLOCK_SIZE 119
#define INDIRECT_BLOCK_SIZE 128

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

bool inode_expand(struct inode_disk *inode, off_t length);
off_t inode_extension(struct inode *inode, off_t length);

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}


/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t length, off_t pos) 
{
  ASSERT (inode != NULL);
  if (pos < length) {
	if (pos < BLOCK_SECTOR_SIZE * DIRECT_BLOCK_SIZE) {
	  return inode->data.direct[pos / BLOCK_SECTOR_SIZE];
	}
  }
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool type_dir)
{
  //The sectors all need to be allocated here. Always allocate enough for the file size of length n
  struct inode_disk *disk_inode = NULL;
  bool success = false;
  static char zeroes[BLOCK_SECTOR_SIZE]; //This is a block of purely zeroes so we can "clean" the data inside the sectors we allocate

  ASSERT (length >= 0);
  //printf("%d\n", sizeof *disk_inode);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode); //Now we need to format the inode on disk
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length); //Takes the file length and determines how many sectors need to be allocated
      disk_inode->length = 0; // Set the length to 0, as well will "expand it" with actual given length
      disk_inode->magic = INODE_MAGIC; //hello
      disk_inode->type_dir = type_dir;
      disk_inode->parent = ROOT_DIR_SECTOR;
	  disk_inode->numDirect = 0; //To show no blocks have been written to
	  if (inode_expand(disk_inode, length)) { //If we allocated to the disk properly
		block_write(fs_device, sector, disk_inode); //Update the inode that is now on disk
		success = true;
	  }
	free(disk_inode); //We're done with the inode on disk, so let's free it
    }
  return success; //Whether allocation was successful or not, we need to return whether we were successful or not

	

	  /*
	  for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) { //This helps us determine if a block has been allocated or not
		disk_inode->direct[i] = -1;
	  }

      for (int i = 0; i < sectors; i++) { //You know how many sectors need to be allocated 
        { for (int j = 0; j < DIRECT_BLOCK_SIZE; j++) {
			 if (disk_inode->direct[j] == -1) { //Check that we are writing to a free entry
			 	free_map_allocate(1, &disk_inode->direct[j]); //Now we just allocate a sector and the direct table holds a pointer to the allocated sector
				length = length - 512; //Every allocation means we have taken care of 512 of the bytes that need to be written to
			 	if (length <= 0) {
			  	 break; //If there is no need to allocate more space, just stop allocating
			 	}
			 }
		  }
	   } //Now the direct block table is filled
		  if (length > 0) {
			success = false; //We were not able to allocate enough
		  }
		  else { //Allocation was successful
			success = true;
		  }
		  block_write(fs_device, sector, disk_inode); //And now we update the copy of the inode disk
		  */
		  //Will end up doing something similar for the indirect and double indirect pointers
		  /*
          block_write (fs_device, sector, disk_inode);
          if (sectors > 0) 
            {
              static char zeros[BLOCK_SECTOR_SIZE];
              size_t i;
              
              for (i = 0; i < sectors; i++) 
                block_write (fs_device, disk_inode->start + i, zeros);
            }
          success = true;
        } 
      free (disk_inode);
  }

  return success; */
}

bool inode_edit_parent(block_sector_t parent_sector,
           struct inode* child)
{
  if (child == NULL)
      return false;
  child->parent = parent_sector;
  return true;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  inode->length = inode->data.length; //The inode needs to know how long the corresponding data is
  inode->parent = inode->data.parent;
  inode->type_dir = inode->data.type_dir;
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  bool list_remove_inode = false;
  bool removed = false;
  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove_inode = true;
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          bool removed = true;
		 for (int i = 0; i < inode->data.numDirect; i++) {
			free_map_release(inode->data.direct[i], 1); //Just deallocate all the direct blocks
		 }
		 /*for (int j = 0; j < inode->data.numDirect; j++) {
			free_map_release(inode->data.indirect_ptr[j], 1); //Just deallocate all the direct blocks
		 } */  
         free_map_release (inode->sector, 1);
         free_map_release (inode->data.start, bytes_to_sectors (inode->data.length)); 
        } 

    }

    // Not going to rmeove from inode list if last opener did not close
    if(!removed){ //Save the state of the disk_inode to disk regardless, but only if it hasn't been deleted aka removed
    inode->data.parent = inode->parent;
    block_write(fs_device, inode->sector, &inode->data); //This writes the state of the latest copy of the disk_inode to disk
    }

    // Remove the inode from the inode list if
    // the last opener has closed it
    // then free the inode
    if(list_remove_inode == true)
    {
      list_remove (&inode->elem);
      free(inode);
    }

}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  if (offset + size > inode->data.length) { //If you are reading past the end of the inode
	return 0; //Return a 0 as no bytes could be read
  }

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, inode->length, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;

}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  if (offset + size > inode->data.length) { //If you are writing to a point past the inode
	inode->length = inode_expand(&inode->data, offset + size - inode_length(inode)); //Now you must extend the inode that exists before writing to it and alos update the length of the inode
	//printf("%d\n", inode->length);
  }

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, inode_length(inode), offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;

}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

bool inode_expand(struct inode_disk *inode, off_t length) { //This will be the function to expand the blocks needed in inode_create

  static char zeroes[BLOCK_SECTOR_SIZE]; //An array of zeroes to "clean" the sector data
  int sectors = bytes_to_sectors (length);// - bytes_to_sectors(inode->length); //This determines the length by which you want to expand the current inode
  int total_sectors = sectors; // save a copy of sectors
  bool success = false;
  struct indirect_block block; //This is for implementing indirect blocks

  int i = -1;
  int old_numDirect = inode->numDirect;
  for (i = inode->numDirect; i < DIRECT_BLOCK_SIZE; i++) { //This is where we actually allocate the sectors
	i = inode->numDirect; //Start at the next free block
	if (sectors > 0) { //If there are still sectors to allocate
	 //Well, let's allocate!
	 free_map_allocate(1, &inode->direct[i]); //Now we just allocate a sector and the direct table holds a pointer to the allocated sector
	 block_write(fs_device, inode->direct[i], zeroes); //Now clean what is inside the allocated sector
	 sectors--; //We know a sector has been allocated
	 inode->numDirect++; //Also increment the number of direct blocks allocated
	}
    if (sectors == 0) { //If the sectors are all allocated
      success = true; //Then we allocated everything!
     }
    if (success) { //If we did allocate everything
      inode->length += length; // Update the inode_disk length since we wrote new data!
      break;
    }
  }

  if (!success && sectors != 0) { //Just direct pointers were not enough
	success = false; //Set success back to false
	inode->indirect_ptr = block.ind_ptrs; //Now we point to the block with 128 pointers

	for (int j = 0; j < INDIRECT_BLOCK_SIZE; j++) {
	  block.ind_ptrs[j] = 0; //This will clean the junk inside the array of pointers
	}

	if (inode->numIndirect == 0) {
	  free_map_allocate(1, &inode->indirect_ptr); //Go ahead and allocate to the indirect block
	  block_write(fs_device, inode->indirect_ptr, &block); //Let's also go ahead and write the initial state to disk
	}
	else { //Otherwise
	  block_read(fs_device, inode->indirect_ptr, &block); //Otherwise read the indirect block into the filesystem
	}

	for (int j = 0; j < INDIRECT_BLOCK_SIZE; j++) {
	  j = inode->numIndirect; //Start with the next free indirect block
	  if (sectors > 0) { //Now we can allocate indirect blocks
		free_map_allocate(1, &block.ind_ptrs[j]); //So now we start allocating to the indirect block array
		block_write(fs_device, block.ind_ptrs[j], zeroes); //Now clean what is inside the allocated sector
	 	sectors--; //We know a sector has been allocated
	 	inode->numIndirect++; //Also increment the number of indirect blocks allocated
	  }
	  if (sectors == 0) { //If all the sectors were allocated
		success = true;
		block_write(fs_device, inode->indirect_ptr, &block); //Now we write to disk
		inode->length += length; //Now we can update the length
	 	break; //Get out of this loop
	  }
	}
  }

  return success;
}
