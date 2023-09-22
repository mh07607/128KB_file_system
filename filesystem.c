#include <stdio.h>
#include <stdlib.h>


/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */


#define FILENAME_MAXLEN 8  // including the NULL char

/* 
 * inode 
 */
typedef struct inode {
  int  dir;  // boolean value. 1 if it's a directory.
  char name[FILENAME_MAXLEN];
  int  size;  // actual file/directory size in bytes.
  int  blockptrs [8];  // direct pointers to blocks containing file's content.
  int  used;  // boolean value. 1 if the entry is in use.
  int  rsvd;  // reserved for future use
} inode;

/* 
 * directory entry
 */

typedef struct dirent {
  char name[FILENAME_MAXLEN];
  int  namelen;  // length of entry name
  int  inode;  // this entry inode index
} dirent;

typedef struct pathNode {
  struct pathNode ** children;
  dirent * entry;
} pathNode;

typedef struct block {
  char data[1024];
} block;

typedef struct superblock {
  //can replace with a enum for free and used
  char free_block_list[128];
  inode inode_list[16];
  //total size = 128 + 16*56 = 1024 Bytes
} superblock;

/*
 * functions
 */
// create file
// copy file
// remove/delete file
// move a file
// list file info
// create directory
// remove a directory

void create_file(char * fileName, int size) {
  // find a free inode
  // find a free block
  // update inode
  // update block
  // update superblock
}

void create_directory(char * directoryName, int size){
  
}

/*
 * main
 */
int main (int argc, char* argv[]) {
  void * file_system = malloc(128*1024);  // 128KB
  superblock * super_block = (superblock *) file_system;
  block * blocks[127] = (block *) file_system;

  inode * root = (inode *) file_system;
  root->dir = 1;
  root->name[0] = '/';
  root->size = 0;
  root->used = 1;



  printf("%llu\n", sizeof(*root));
  free(file_system);
  // while not EOF
    // read command
    
    // parse command
    
    // call appropriate function

	return 0;
}
