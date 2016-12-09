#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
//#define INODE_DEBUG 2

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

bool inode_expand(struct inode_disk *inode, off_t length, bool create);

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}


static block_sector_t
byte_to_db_indirect_sector(const struct inode *inode, off_t length, off_t pos)
{
  //printf("Hello\n");
   if(pos < (DIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE + INDIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE + INDIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE*INDIRECT_BLOCK_SIZE))
   {
    struct indirect_block block;
    for(int i = 0; i < INDIRECT_BLOCK_SIZE; i++)
    {
      block.ind_ptrs[i] = 0;
    }
  //printf("Hello2\n");

    ASSERT(inode->data.db_indirect_ptr > 0);
    block_read(fs_device, inode->data.db_indirect_ptr, &block);
    int pos_db_indirect = pos - DIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE - INDIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE;

    int db_ind_index = pos_db_indirect / (INDIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE);
  //printf("Hell3.2 %d %d", db_ind_index, pos_db_indirect);
    int ind_index = ( pos_db_indirect % (INDIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE) ) / BLOCK_SECTOR_SIZE;
  //printf("Hello3 %d %d %d\n", ind_index, db_ind_index, block.ind_ptrs[db_ind_index]);
    struct indirect_block block2;

    for(int i = 0; i < INDIRECT_BLOCK_SIZE; i++)
    {
      block2.ind_ptrs[i] = 0;
    }
    block_read(fs_device, block.ind_ptrs[db_ind_index], &block2);
    
  //printf("Hello4 %d\n", block2.ind_ptrs[ind_index]);

    return block2.ind_ptrs[ind_index];
   }

   int return_sector = -1;
   ASSERT(return_sector == 0);
   return (block_sector_t) return_sector;
}

static block_sector_t
byte_to_indirect_sector(const struct inode *inode, off_t length, off_t pos)
{
   if(pos < (DIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE + INDIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE))
   {
    struct indirect_block block;
    for(int i = 0; i < INDIRECT_BLOCK_SIZE; i++)
    {
      block.ind_ptrs[i] = 0;
    }

    ASSERT(inode->data.indirect_ptr > 0);
    block_read(fs_device, inode->data.indirect_ptr, &block);
    int pos_indirect = pos - DIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE;
    return block.ind_ptrs[pos_indirect / BLOCK_SECTOR_SIZE];
   }


   return byte_to_db_indirect_sector(inode, length, pos);
}



/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if the byte offest POS
   refers to a indirect or dbl_indirect sector*/
static block_sector_t
byte_to_sector (const struct inode *inode, off_t length, off_t pos) 
{
  ASSERT (inode != NULL);
  //printf("pos %d\n", pos);
  if (pos <  DIRECT_BLOCK_SIZE*BLOCK_SECTOR_SIZE) {

    //makeprintf("inode->data.direct[] %d\n", inode->data.direct[pos / BLOCK_SECTOR_SIZE]);
    return inode->data.direct[pos / BLOCK_SECTOR_SIZE];
  }
  //printf("length %d pos %d\n", length, pos);
  
  return byte_to_indirect_sector(inode, length, pos);
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
inode_create (block_sector_t sector, off_t length, bool type_dir, block_sector_t parent_sector)
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
      disk_inode->parent = parent_sector;

    disk_inode->indirect_ptr = 0;
    disk_inode->db_indirect_ptr = 0;
    disk_inode->numDirect = 0; //To show no blocks have been written to, already setup since using local variable
    disk_inode->numIndirect = 0; // To show indirect have not been used at all, thus not setup
    disk_inode->numDbIndirect = 0; // To show dbl_indirect have not been used at all, thus not setup
    if (inode_expand(disk_inode, length, true)) { //If we allocated to the disk properly
    disk_inode->length = length;
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
        #ifdef INODE_DEBUG
        printf("Deleting inode %p sector %d\n", inode, inode->sector);
        #endif
          removed = true;
        
         for (int i = 0; i < inode->data.numDirect; i++) {
          free_map_release(inode->data.direct[i], 1); //Just deallocate all the direct blocks
         }
     
         struct indirect_block block;

         for(int j = 0; j < INDIRECT_BLOCK_SIZE; j++)
         {
          block.ind_ptrs[j] = 0;
         }
         if(inode->data.indirect_ptr > 0)
         {
           block_read(fs_device, inode->data.indirect_ptr, &block);
           for (int j = 0; j < inode->data.numIndirect; j++) {
            free_map_release(block.ind_ptrs[j], 1); //Just deallocate all the direct blocks
           }
         } 
         free_map_release (inode->sector, 1);
      }

     }
         


    

    // If not marked for deletion, save the state
    if(!removed){ //Save the state of the disk_inode to disk regardless, but only if it hasn't been deleted aka removed
    inode->data.parent = inode->parent;
    ASSERT(inode->length == inode->data.length);
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

int inode_actual_length(struct inode *inode) {
  int direct_sectors = inode->data.numDirect;
  int indirect_sectors = inode->data.numIndirect >= 0? inode->data.numIndirect : 0;
  int indbindirect_sectors = inode->data.numDbIndirect >= 0? inode->data.numDbIndirect : 0;
  int total_blocks_from_db_indirect = 0;


  // Caculate number of data blocks from double_indirect_block
  if(indbindirect_sectors > 0)
  {

    struct indirect_block block;
    for(int i = 0; i < INDIRECT_BLOCK_SIZE; i++)
    {
      block.ind_ptrs[i] = 0;
    }

    if(inode->data.db_indirect_ptr > 0)
    {

      // Read the double block to get the pointers
      block_read(fs_device, inode->data.db_indirect_ptr, &block);
      for(int i = 0; i < INDIRECT_BLOCK_SIZE; i++)
      {
        // check if double block pointer is valid i
        if(block.ind_ptrs[i] > 0)
        {

            struct indirect_block block_ind;
            for(int i = 0; i < INDIRECT_BLOCK_SIZE; i++)
              block_ind.ind_ptrs[i] = 0;

            block_read(fs_device, block.ind_ptrs[i], &block_ind);

            // calculate the number of sectors/datablocks
            // in this indirect_block
            for(int i = 0; i < INDIRECT_BLOCK_SIZE; i++)
            {
              // check if indirect pointer is valid 
              if(block_ind.ind_ptrs[i] > 0)
              {
                total_blocks_from_db_indirect = total_blocks_from_db_indirect + 1;
              }
            }
        }
      }
    } else
    {
      return -1;
    }
  }

  int inode_actual_size = direct_sectors*BLOCK_SECTOR_SIZE + indirect_sectors*BLOCK_SECTOR_SIZE + total_blocks_from_db_indirect*BLOCK_SECTOR_SIZE; // TODO, add db_indirect
  return inode_actual_size;
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

  #ifdef INODE_DEBUG
  printf("inode_read: inode->length %d size %d, offset %d numDirect %d numIndirect %d numDbIndrect %d\n", inode->length, size, offset,inode->data.numDirect, inode->data.numIndirect, inode->data.numDbIndirect);
  #endif
  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */ 
      block_sector_t sector_idx = byte_to_sector (inode, inode_actual_length(inode), offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_actual_length(inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;
  #ifdef INODE_DEBUG
      printf(" sector_idx %d\n sector_ofs %d\n inode_left %d\n sector_left %d\n min_left %d size %d\n", sector_idx, sector_ofs, inode_left, sector_left, min_left, size);
      #endif
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
// printf(" Inode read bytes read %d", bytes_read);
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
  int offset_copy = offset;

  if (inode->deny_write_cnt)
    return 0;



  int inode_actual_size = inode_actual_length(inode);
  // Update the length if we are writing past the file's size
  if(inode->data.length  < inode_actual_size && ( (offset + size) > inode->data.length && (offset + size)  <= inode_actual_size)  )
  {
    inode->length = offset_copy + size;
    #ifdef INODE_DEBUG
    printf("inode->length %d\n", inode->length);
    #endif
    inode->data.length = inode->length;
  }
  #ifdef INODE_DEBUG
  printf(" about to expand...  actua_size %d offest %d size %d inode->data.length %d inode->length %d\n",  inode_actual_size, offset, size, inode->data.length, inode->length);
  #endif

  if (offset + size > inode_actual_size) { //If you are writing to a point past the inode
    #ifdef INODE_DEBUG
  printf("Condition met to expand , offset %d size %d inode_length %d\n", offset, size, inode_length(inode));
  #endif
  inode_expand(&inode->data, offset + size - inode->data.length, false); //Now you must extend the inode that exists before writing to it and alos update the length of the inode
  inode->data.length += offset + size - inode->data.length;
  inode->length = inode->data.length;
  inode_actual_size = inode_actual_length(inode);
  #ifdef INODE_DEBUG
  printf("Done expanding %d\n", inode->length);
  #endif
  }
  #ifdef INODE_DEBUG
  printf("Now Writing... ");
  #endif
  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, inode_actual_size, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;
      #ifdef INODE_DEBUG
      printf("%d sector ofs offset %d", sector_ofs, offset);
      #endif
      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_actual_size - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;
  #ifdef INODE_DEBUG
      printf(" sector_idx %d\n sector_ofs %d\n inode_left %d\n sector_left %d\n min_left %d\n", sector_idx, sector_ofs, inode_left, sector_left, min_left);
      #endif
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
      #ifdef INODE_DEBUG
      printf("end while size %d\n", offset);
      #endif
    }
  free (bounce);

    #ifdef INODE_DEBUG
    printf("inode->length %d\n", inode->length);
    #endif

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


bool allocate_indirect(struct inode_disk* inode, 
  block_sector_t double_block_sector,
  int double_block_sector_index,
  int num_ind_sectors,
  bool new_double_block_time,
  struct indirect_block* db_block)
{


  // allocate a double block 
  if(new_double_block_time)
  {
    free_map_allocate(1, &db_block->ind_ptrs[inode->numDbIndirect]);
    inode->numDbIndirect = inode->numDbIndirect + 1;
    // update the inode_disk since numDbIndirect was increased
    block_write(fs_device, inode->db_indirect_ptr, db_block);
    double_block_sector = db_block->ind_ptrs[inode->numDbIndirect -1];
  }

  struct indirect_block ind_block; //This is for implementing indirect blocks
  for (int j = 0; j < INDIRECT_BLOCK_SIZE; j++) {
    ind_block.ind_ptrs[j] = 0; //This will clean the junk inside the array of pointers
  }

  // Now allocate the number of leftover sectors inside the double block
  if(new_double_block_time)
  for(int k = 0; k < num_ind_sectors; k++)
  {
    free_map_allocate(1, &ind_block.ind_ptrs[k]);
  }

  else {

      block_read(fs_device, double_block_sector, &ind_block);
    for(int i = 0; i < num_ind_sectors; i++)
    {
      free_map_allocate(1, &ind_block.ind_ptrs[i+double_block_sector_index]);
    }
  }
  // Write the new double block into filesystem
  if(num_ind_sectors > 0)
  block_write(fs_device, double_block_sector, &ind_block);

  bool success = true;

  return success;
}




bool inode_expand(struct inode_disk *inode, off_t length, bool create) { //This will be the function to expand the blocks needed in inode_create

  #ifdef INODE_DEBUG
  printf("Inode_Expand inode %p, length %d numDirect %d inode_length%d numIndirect %d numDbIndrect %d\n", inode, length, inode->numDirect, inode->length, inode->numIndirect, inode->numDbIndirect);
  #endif
  static char zeroes[BLOCK_SECTOR_SIZE]; //An array of zeroes to "clean" the sector data
  int sectors = bytes_to_sectors (length);// - bytes_to_sectors(inode->length); //This determines the length by which you want to expand the current inode
  int total_sectors = sectors; // save a copy of sectors
  bool success = false;

  int i = -1;
  int old_numDirect = inode->numDirect;
  for (i = inode->numDirect; i < DIRECT_BLOCK_SIZE; i++) { //This is where we actually allocate the sectors
  i = inode->numDirect; //Start at the next free block
  if (sectors > 0) { //If there are still sectors to allocate
   //Well, let's allocate!
    #ifdef INODE_DEBUG
    printf("inode expand: Direct expanding at %d\n", i);
    #endif
   free_map_allocate(1, &inode->direct[i]); //Now we just allocate a sector and the direct table holds a pointer to the allocated sector
   block_write(fs_device, inode->direct[i], zeroes); //Now clean what is inside the allocated sector
   sectors--; //We know a sector has been allocated
   inode->numDirect++; //Also increment the number of direct blocks allocated
    }
    if (sectors == 0) { //If the sectors are all allocated
      success = true; //Then we allocated everything!
     }
    if (success) { //If we did allocate everything
      //inode->length += length; // Length is updated by calling function
      break;
    }
  }
  #ifdef INODE_DEBUG
  printf("success %d, sectors, %d, inode->numDirect %d\n", success, sectors, inode->numDirect);
  #endif
  if (!success && sectors != 0) { //Just direct pointers were not enough

    struct indirect_block block; //This is for implementing indirect blocks
    for (int j = 0; j < INDIRECT_BLOCK_SIZE; j++) {
      block.ind_ptrs[j] = 0; //This will clean the junk inside the array of pointers
    }

    if (inode->indirect_ptr == 0) {
      free_map_allocate(1, &inode->indirect_ptr); //Go ahead and allocate to the indirect block
      //block_write(fs_device, inode->indirect_ptr, &block); //Let's also go ahead and write the initial state to disk
    }
    else { //Otherwise
      block_read(fs_device, inode->indirect_ptr, &block); //Otherwise read the indirect block into the filesystem
    }

    for (int j = inode->numIndirect; j < INDIRECT_BLOCK_SIZE; j++) {
      j = inode->numIndirect; //Start with the next free indirect block
      if (sectors > 0) { //Now we can allocate indirect blocks
      free_map_allocate(1, &block.ind_ptrs[j]); //So now we start allocating to the indirect block array
      block_write(fs_device, block.ind_ptrs[j], zeroes); //Now clean what is inside the allocated sector
      sectors--; //We know a sector has been allocated
      inode->numIndirect++; //Also increment the number of indirect blocks allocated
      }
      if (sectors == 0 || (j+1) == INDIRECT_BLOCK_SIZE) { //If all the sectors were allocated
      block_write(fs_device, inode->indirect_ptr, &block); //Now we write to disk
      
          if(sectors == 0)
          {
          success = true;
          //inode->length += length; // Length is updated by calling function
          }
      break; //Get out of this loop
      }
    }

    // double_indirect
    if(success == false)
    {
      ASSERT(sectors > 0);
    struct indirect_block db_block; //This is for implementing indirect blocks
    for (int j = 0; j < INDIRECT_BLOCK_SIZE; j++) {
      db_block.ind_ptrs[j] = 0; //This will clean the junk inside the array of pointers
    }
    #ifdef INODE_DEBUG
    printf("Double Indirect Expansion\n");
    #endif
      // Either create a new db_indirect_block or read from an existing one
      if(inode->db_indirect_ptr <= 0) {
        free_map_allocate(1, &inode->db_indirect_ptr);
      }
      else {
      block_read(fs_device, inode->db_indirect_ptr, &db_block); //Otherwise read the indirect block into the filesystem
      }

      // From the sectors needed to allocate, determine the
      // num of double blocks and any leftover
      // indirect blocks needed
      int num_db_sectors = sectors / INDIRECT_BLOCK_SIZE; 
      int num_ind_sectors = sectors % INDIRECT_BLOCK_SIZE;
      #ifdef INODE_DEBUG
      printf("num_db_sectors %d num_ind_sectors %d\n", num_db_sectors, num_ind_sectors);
      #endif
      if(num_db_sectors > (INDIRECT_BLOCK_SIZE - inode->numDbIndirect))
        return false;


      // Allocate the number of double blocks needed
      for(int k = 0; k < num_db_sectors; k++)
      {
        free_map_allocate(1, &db_block.ind_ptrs[inode->numDbIndirect]);
        struct indirect_block ind_block; //This is for implementing indirect blocks
        for (int j = 0; j < INDIRECT_BLOCK_SIZE; j++) {
          ind_block.ind_ptrs[j] = 0; //This will clean the junk inside the array of pointers
        }

        for(int k = 0; k < INDIRECT_BLOCK_SIZE; k++)
        {
          free_map_allocate(1, &ind_block.ind_ptrs[k]);
          inode->numDbIndirect = inode->numDbIndirect + 1;
        }

        block_write(fs_device, db_block.ind_ptrs[inode->numDbIndirect], &ind_block);
        inode->numDbIndirect = inode->numDbIndirect + 1;
      }

      // update the inode_disk if numDbIndirect was increased
      if(num_db_sectors > 0)
      {
        block_write(fs_device, inode->db_indirect_ptr, &db_block);
      }




      bool new_double_block_time = true;
      block_sector_t double_block_sector = 0;
      int double_block_sector_index = -1;
      // Figure out which double block to use, do we need to make a new one?
      if(num_db_sectors <= 0)
      for(int i = 0; i < inode->numDbIndirect; i++)
      {
        struct indirect_block ind_block; //This is for implementing indirect blocks
        for (int j = 0; j < INDIRECT_BLOCK_SIZE; j++) {
          ind_block.ind_ptrs[j] = 0; //This will clean the junk inside the array of pointers
        }
        block_read(fs_device, db_block.ind_ptrs[i], &ind_block);

        int p = 0;
        while(ind_block.ind_ptrs[p] > 0 && p<INDIRECT_BLOCK_SIZE)
          p++;



        if(p < INDIRECT_BLOCK_SIZE && ind_block.ind_ptrs[p] == 0)
        {

          if( (p + num_ind_sectors) > INDIRECT_BLOCK_SIZE)
          {
            success = allocate_indirect( inode,
              double_block_sector,
              double_block_sector_index,
              INDIRECT_BLOCK_SIZE - p,
              false,
              &db_block);

              num_ind_sectors -= (INDIRECT_BLOCK_SIZE-p);

              new_double_block_time = true;
            }

          
          double_block_sector = db_block.ind_ptrs[i];
          double_block_sector_index = p;
          new_double_block_time = false;
          break;
         }
       }
        

      
      #ifdef INODE_DEBUG
      printf("Double Block Sector %d, P %d last sector %d \n", double_block_sector, double_block_sector_index, db_block.ind_ptrs[0]);
      #endif



      success = allocate_indirect( inode, 
        double_block_sector,
        double_block_sector_index,
        num_ind_sectors,
        new_double_block_time,
        &db_block);
      }


      
  } // end if (!success && sectors > 0)

  return success;
}