#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
//#define FILESYS_DEBUG_2
#define DIRECT_BLOCK_SIZE 121

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44



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
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  //if (pos < inode->data.length) {
    return inode->data.start + pos / BLOCK_SECTOR_SIZE;
  //}
  //else {
    //return -1;
  //}
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

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode); //Now we need to format the inode on disk
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length); //Takes the file length and determines how many sectors need to be allocated
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->type_dir = type_dir;
      disk_inode->parent = ROOT_DIR_SECTOR;
	  disk_inode->numDirect = 0; //To show no blocks have been written to

	  

	  for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) { //This is where we actually allocate the sectors
		if (sectors > 0) { //If there are still sectors to allocate
		  //Well, let's allocate!
		  free_map_allocate(1, &disk_inode->direct[i]); //Now we just allocate a sector and the direct table holds a pointer to the allocated sector
		  block_write(fs_device, disk_inode->direct[i], zeroes); //Now clean what is inside the allocated sector
		  sectors--; //We know a sector has been allocated
		  if (sectors == 0) { //If the sectors are all allocated
			block_write(fs_device, sector, disk_inode); //Update the inode that is now on disk
			success = true; //Then we allocated everything!
		  }
		}

	    if (success) { //If we have finished allocating
		  break; //Exit the for loop
	    }
	  }
	}
	free(disk_inode); //We're done with the inode on disk, so let's free it
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

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
		 for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) {
			free_map_release(inode->data.direct[i], 1); //Just deallocate all the direct blocks
		 }  
         free_map_release (inode->sector, 1);
         free_map_release (inode->data.start, bytes_to_sectors (inode->data.length)); 
        } 
	  else { //Save the state of the disk_inode to disk
		block_write(fs_device, inode->sector, &inode->data); //This writes the state of the latest copy of the disk_inode to disk
 	  }

    }

  free (inode);
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
  bool readable = false; //Checks if you can read from the requested sector
  block_sector_t sectorToReadFrom;

  /* Disk sector to read, starting byte offset within sector. */
  block_sector_t sector_idx = byte_to_sector (inode, offset);
  int sector_ofs = offset % BLOCK_SECTOR_SIZE;

  for (int j = 0; j < DIRECT_BLOCK_SIZE; j++) {
	if (inode->data.direct[j] == sector_idx) { //If we can find the sector to read from
		sectorToReadFrom = j;
		readable = true; //You can read from the sector
		break;
	}
  }

  if (readable == false) { //If you cannot read from the sector
	return 0; //No bytes were read
  }

 while (size > 0) 
    { 
	  int chunk_size = 512; //How many bytes you can read from a sector
	  for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) { //Now we read from the allocated blocks
		i = sectorToReadFrom; //So that you start reading from the correct sector
		if (size > chunk_size) { //If the number of bytes to be read is too many to read from a single sector
		  block_read(fs_device, inode->data.direct[i], chunk_size); //Read 512 bytes
		  size -= chunk_size; //So 512 bytes have already been read
		  bytes_read += chunk_size; //Update how many bytes were read
		}
		else { //We only have to read from a single sector
		  block_read(fs_device, inode->data.direct[i], buffer); //Read from the sector
		  size -= chunk_size; //Of course, update size to reflect that all the bytes have been read
		  bytes_read += chunk_size; //Update how many bytes were read
		  break;
		}
	  } //So now the direct blocks are filled up
 	}
	  return bytes_read;
	
/*	  
      /* Bytes left in inode, bytes left in sector, lesser of the two.
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector.
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer.
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer.
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance.
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read; */
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
  const uint8_t *buffer = buffer_; //This holds the stuff to write to the sector
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  //The stuff that was written after looking at sample code

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset); //Determines which sector to write to
      int sector_ofs = offset % BLOCK_SECTOR_SIZE; //Where within the sector to write to
	  int chunk_size = 512; //This is how many bytes can be written to the block at once

	  for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) { //Now we write to the sectors using the direct blocks
		if (inode->data.numDirect == 0) { //If no blocks have been written to
		  i = 0;
		}
		else { //You want to start writing at numDirect
		  i = inode->data.numDirect;
		}
		if (inode->data.numDirect == DIRECT_BLOCK_SIZE) { //If all the direct blocks have been allocated
		  return 0; //Then there is no space to write so return 0
	 	}

		if (size > chunk_size) { //If the number of bytes to be written is too many to write to a single sector
		  block_write(fs_device, inode->data.direct[i], buffer); //Pretty sure we have to modify the buffer, as that is what I am writing to the block, and it may be too large
		  size -= chunk_size; //So 512 bytes have already been written
		  bytes_written += chunk_size; //Update how many bytes were written
		  inode->data.numDirect++; //To show that a block has been allocated
		}

		else { //We only have to write to a single remaining sector
		  block_write(fs_device, inode->data.direct[inode->data.numDirect], buffer); //Write the remaining bytes
		  size -= chunk_size; //Of course, update size to reflect that all the bytes have been written
		  bytes_written += chunk_size; //Update how many bytes were written
		  inode->data.numDirect++; //To show that a block has been allocated
		}
		if (size <= 0) { //If we are done writing to blocks
		  break; //Get out of the loop
		}
	  } //So now the direct blocks are filled up
	}
	return bytes_written;
		  
	  /*
      /* Bytes left in inode, bytes left in sector, lesser of the two.
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector.
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk.
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer.
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros.
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance.
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;*/
    //}
  //free (bounce);
  //return bytes_written;
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
