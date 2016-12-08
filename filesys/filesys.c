#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
//#define FILESYS_DEBUG 1

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
bool parse_file_path(char* command, char* argv[], char* parse_name);
bool
filesys_find_dir(const char* name,
                 struct inode** return_inode,
                 struct dir** return_dir ,
                 block_sector_t *parent_sector
                 ,char* parsed_name);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}
/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool type_dir) 
{
  #ifdef FILESYS_DEBUG
  printf("filesys_create %s\n", name);
  #endif 

    // Check if name is ""
  if(strcmp(name, "") == 0)
    return false;

  block_sector_t inode_sector = -1;
  block_sector_t parent_sector;
  struct dir *dir = NULL;
  struct inode *inode = NULL;
  char parsed_name[512];
  bool success = filesys_find_dir(name, &inode, &dir, &parent_sector, parsed_name);

  // Check if parsed name is bigger than allowed by pintos
  if(strlen(parsed_name) > 14)
    return false;

  // Check if the directory to create file/dir in exists
  if(dir->inode->removed)
    return false;


  #ifdef FILESYS_DEBUG
  printf("filesys find succ %d parsed name %s\n", success, parsed_name);
  #endif
  if(success) {
  // Add the file/dir to the specifieddirectory
  success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, type_dir, parent_sector));
  #ifdef FILESYS_DEBUG
  printf("filesyscreate: success1 inode sec %d: %d\n",inode_sector, success);
 #endif

  if(inode_sector == -1) // Exit with false if out ofmemory
    return false;

   success = dir_add (dir, parsed_name, inode_sector);
    #ifdef FILESYS_DEBUG
  printf("filesyscreate: success2: %d added to %p %p\n", success, dir, dir->inode);
  #endif

  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);

  if(dir!=NULL)
   dir_close (dir); // Close the directory in which we added the new file to (thus updating it on disk)
  //inode_close(inode);
  }
  #ifdef FILESYS_DEBUG

  printf("create succ %d\n", success);
  #endif
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file*
filesys_open (const char *name, bool type_dir, bool* warning)
{
    #ifdef FILESYS_DEBUG
  printf("filesysopen: name: %s\n", name);
  #endif

  // Check for "" file name
  if(strcmp(name,"") == 0)
    return NULL;
  struct dir *dir = NULL; //dir_open_root (); // Don't open root by default
  struct inode *inode = NULL;
  char parsed_name[100];

  // Parse the full path name
  bool success = filesys_find_dir(name, &inode, &dir, NULL, parsed_name);


    // Check if file name is over da limit
  if(strlen(parsed_name) > 14)
    return NULL;
  #ifdef FILESYS_DEBUG
  printf("Filesysopen: filesysfind succ %d| parsedname:%s\n", success, parsed_name);
  #endif
  if(success)
  {
    #ifdef FILESYS_DEBUG
    printf("open inode %p\n", inode);
    printf("open dir %p\n", dir);
    #endif

    // Condition is hit when user tries to open directory or file using open
    if(!type_dir)
    {
        bool found = false;
        // Handle Edge case where file name is "/"
        if(name[0] == 47 && name[1] == NULL) // '/' == ascii dec:47
        {
          found = true;
          //inode = inode_reopen(inode);
        }

        // Handle Edge case where file name is "."
        else if(name[0] == 46 && name[1] == NULL)
        {
          inode = dir->inode;
          //inode = inode_reopen(inode);
          found = true;
        }

        // Handle Edge case where file name is ".."
        else if(name[0] == 46 && name[1] == 46 && name[2] == NULL)
        {
          if(dir->inode->removed == false)
          {
          inode = inode_open(dir->inode->parent);
          found = true;
          dir_close(dir);
          }
        }
        else 
        {
          #ifdef FILESYS_DEBUG
          printf("filesysopen: dirlookup: dir %p dirinode %p openfname %s storage%p\n", dir,dir->inode, parsed_name, inode);
          #endif
          found = dir_lookup (dir, parsed_name,
              &inode);
          dir_close(dir);
        }

          #ifdef FILESYS_DEBUG
            printf("Check me %d", found);
            #endif


        if(found)
        {

          // Tell caller user is trying to open a directory using open
          // if the caller cares (syscal will care)
          if(inode->type_dir == true)
          {
            // Make sure the directory/file actually exists
            if(inode->removed)
            {
              inode_close(inode);
              return NULL;
            }

            if(warning)
              *warning = true;
            return (struct file*) dir_open(inode);
          }

          return file_open(inode);

      }
      else
      {
        return NULL;
      }
    }
    // Condition is hit when user changes directory (syscall chdir)
    else
    {
      bool found = false; bool no_reopen = false;
            // Handle Edge case where directory name is "/"
        if(name[0] == 47 && name[1] == NULL) // '/' == ascii dec:47
        {
          found = true;
          //inode = inode_reopen(inode);
        }

        // Handle Edge case where file name is "."
        else if(name[0] == 46 && name[1] == NULL)
        {
          inode = dir->inode;
          //inode = inode_reopen(inode);
          found = true;
        }
        // Handle Edge case where file name is ".."
        else if(name[0] == 46 && name[1] == 46 && name[2] == NULL)
        {
          if(dir->inode->removed == false)
          {
          inode = inode_open(dir->inode->parent);
          found = true;
          }
          dir_close(dir);

        }
        else
        {
          found = dir_lookup (dir, parsed_name,
              &inode);
          dir_close(dir);
        }
        if(found)
        {
          #ifdef FILESYS_DEBUG
          printf("Changing cd directory to inode %p %d\n", inode, inode->sector);
          #endif
            // Make sure the directory/file actually existsdir_removedir_remove
            if(inode->removed)
            {
              inode_close(inode);
              return NULL;
            }

        struct dir* dir = dir_open(inode);

        thread_current()->cd.cd_dir = dir;
        return (struct file*) dir;
        }
    }

  }
  return NULL;

}

/* Takes in a full path to a directory or file
   and then parses such that
   it returns the requested directory/file's inode
   in return_inode
   and returns the directory that the requested
   directory or file in return_dir
   and returns the directory/file's name only (no paths)
   in parsed_name */
bool
filesys_find_dir(const char* name,
                 struct inode** return_inode,
                 struct dir** return_dir ,
                 block_sector_t *parent_sector,
                 char* parsed_name)
{
    #ifdef FILESYS_DEBUG
  printf("Hello from filesys_find_dir\n");
  #endif
  struct thread* t = thread_current();
  // Make a copy of name
  int file_char_length = strlen(name) + 1;
  char name_cpy[file_char_length];
  strlcpy(name_cpy, name, file_char_length);

  // Parse the path
  char* argv[MAX_PATH_COUNT];
  parse_file_path(name_cpy, argv,parsed_name);

  // Parent sector
  block_sector_t return_parent_sector = -1;

  // Open the correct dir to create the file/dir in
      #ifdef FILESYS_DEBUG
  printf("filesysfind name %s\n", name);
  #endif
  struct dir* dir = NULL;
  if((bool) argv[1] == true)
    dir = dir_open_root();
  else
  {
        #ifdef FILESYS_DEBUG
    if(t->cd.cd_dir)
    printf("Using cd not root %p secotr %d\n", t->cd.cd_dir, t->cd.cd_dir->inode->sector);
    #endif
    dir = t->cd.cd_dir;
    if(dir !=NULL)
    {
      dir = dir_open(inode_reopen(dir->inode));
    }
  }
  if(dir == NULL)
  {
    dir = dir_open_root();
  }
  #ifdef FILESYS_DEBUG
  printf("using dir -> %p\n", dir);
  printf("filesys find: i = %d", argv[0]);
  #endif

  int i = 2;
  struct inode * inode = NULL;
  for(i = 2; i < argv[0] -1; i++)
  {
      #ifdef FILESYS_DEBUG
    printf("filesys find dir in %dfor %s\n", i, argv[i]);
      #endif 

    inode = NULL;

    // get parent if necessary
    if(strcmp(argv[i],"..") == 0)
    {
      struct dir* temp = dir;
      dir = dir_open_parent(dir);
      dir_close(temp);
    }
 
    // continue if dot, we know it's relative already dammit
    else if(strcmp(argv[i], ".") == 0)
    {
      continue;
    }

    else
    {
      bool found = dir_lookup (dir, argv[i],
            &inode);
      if(found)
      {
          #ifdef FILESYS_DEBUG
      printf(" and look it up\n");
      #endif
      dir_close(dir);
      dir = dir_open(inode);
      }

      else
        return false;
    }
    return_parent_sector = inode->sector;


  }
  #ifdef FILESYS_DEBUG
  printf("find inode%p\n find dir %p\n", inode,dir);
  #endif

  if(return_parent_sector == -1)
    return_parent_sector = dir->inode->sector;
  // Return the root directory's inode
  // in the edge case where the name is "/"inode
  if(name[0] == 47 && name[1] == NULL)
  {
    inode = dir->inode;
    dir = NULL; // recall that dir 
    // holds the parent directory
    // of the requested file/directory
    // since root has no parent dir,
    // dir is set to false
  }

  if(return_inode)
    *return_inode = inode;
  if(return_dir)
    *return_dir = dir;

  if(parent_sector)
    *parent_sector = return_parent_sector;


    return true;

}

/* Parses the path name, into an array of char
   which contains each sub dir name
   returns false if relative, true if absolute */
bool parse_file_path(char* command, char* argv[], char* parsed_name)
{
  #ifdef FILESYS_DEBUG
  printf("parse_file_path... %s\n", command);
  #endif

  char* save;
  bool absolute = false;
  if(command[0] == '/')
    absolute = true;


  int i = 2;
  argv[i] = strtok_r(command, "/", &save);
  while(argv[i] != NULL)
  {
    i++;
    argv[i] = strtok_r(NULL, "/", &save);
  }

  
  if(parsed_name && argv[i-1])
    strlcpy(parsed_name, argv[i-1], strlen(argv[i-1])+1);

  else
    if(parsed_name)
      parsed_name[0] = 0;


  int j = 2;
  while(j < (i))
  {
    if(strcmp(argv[j],".") == 0)
    {
      absolute = false;
      break;
    }
    j++;
  }

  /*add metadata*/
  argv[0] = (char*)i;
  argv[1] = (char*)absolute;
  #ifdef FILESYS_DEBUG
  for(int k = 2; k < (int)argv[0]; k++)
  {
    printf("%s\n", argv[k]);
  }
    printf("parse done %d\n", absolute);
    #endif

  return absolute; // return arg count
}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{

  // Check for "" file name
  if(strcmp(name,"") == 0)
    return NULL;
  struct dir *dir = NULL; //dir_open_root (); // Don't open root by default
  struct inode *inode = NULL;
  char parsed_name[100];


  bool success = filesys_find_dir(name, &inode, &dir, NULL, parsed_name);

  if(parsed_name[0] == NULL)
    return false;

  if(success)
  {
  // Check if file name is over da limit
  
  if(strlen(parsed_name) > 14)
    return NULL;
  success = dir != NULL && dir_remove (dir, parsed_name);
  dir_close (dir); 
  }


  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create(ROOT_DIR_SECTOR, 2))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
