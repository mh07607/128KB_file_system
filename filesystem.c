#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FILENAME_MAXLEN 8  // including the NULL char
#define SUPERBLOCK_SIZE sizeof(superblock)
#define BLOCK_SIZE sizeof(block)
#define INODE_SIZE sizeof(inode)

typedef struct block {
  char data[1024];
} block;
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
  dirent direntEntry;
  struct pathNode * child;
  struct pathNode * sibling;
} pathNode;

typedef struct superblock {
  //'0' for free, '1' for used; can replace with a enum for free and used
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

void create_file(char * fileName, int size, superblock * super_block, block * blocks[127], pathNode * root) {
  // assuming size is in bytes
  // find a free inode
  // find a free block
  // update inode
  // update block
  // update superblock
  // inode->dirent->pathNode
  int numberOfBlocks = ceil((double)size/sizeof(block));
  int newFileBlockPtrs[numberOfBlocks];

  for(int i=0; i<127; i++){
    for(int j=0; j < numberOfBlocks; j++){
      if(super_block->free_block_list[i] == '0'){
        super_block->free_block_list[i] = '1';

        newFileBlockPtrs[j] = i;
        break;
      }
    }
  }

  int inodeNumber;
  for(int i=0; i<16; i++){
    if(super_block->inode_list[i].used == 0){
      super_block->inode_list[i].used = 1;
      super_block->inode_list[i].dir = 0;
      strcpy(super_block->inode_list[i].name, fileName);
      super_block->inode_list[i].size = size;
      for(int j=0; j<numberOfBlocks; j++){
        super_block->inode_list[i].blockptrs[j] = newFileBlockPtrs[j];
      }      
      inodeNumber = i;
      break;
    }
  }

  //create pathNode
  root->child = (pathNode *) malloc(sizeof(pathNode));
  root->child->direntEntry.inode = inodeNumber;
  root->child->direntEntry.namelen = strlen(fileName);
  strcpy(root->child->direntEntry.name, fileName);
  root->child->child = NULL;
  root->child->sibling = NULL;
}

void create_directory(char * directoryName, int size, superblock * super_block) {
  // find a free inode
  // find a free block
  // update inode
  // update block
  // update superblock

}

void initializeRoot(){

}
/*
 * main
 */
int main (int argc, char* argv[]) {
  // Initialize file system
  void * file_system = malloc(128*1024);  // 128KB
  // Initialize superblock
  superblock * super_block = (superblock *) file_system;
  super_block->free_block_list[0] = '1';
  for(int i=1; i<128; i++){
    super_block->free_block_list[i] = '0';
  }
  // Initialize blocks array
  block * blocks[127];
  for (int i = 0; i < 127; i++) {
      blocks[i] = (block*)((char*)file_system + SUPERBLOCK_SIZE + i * BLOCK_SIZE);
  }
  //root directory
  inode * root = (inode *) file_system;
  super_block->inode_list[0] = *root;
  root->dir = 1;
  strcpy(root->name, "/");
  root->size = 0;
  root->used = 1;
  root->blockptrs[0]=0;
  pathNode * rootPathNode = (pathNode *) malloc(sizeof(pathNode));
  rootPathNode->direntEntry.inode = 0;
  rootPathNode->direntEntry.namelen = 1;
  strcpy(rootPathNode->direntEntry.name, "/");
  rootPathNode->child = NULL;
  rootPathNode->sibling = NULL;

  printf("%s\n", root->name);
  create_file("test.txt", 1024, super_block, blocks, rootPathNode);
  printf("%s\n", rootPathNode->child->direntEntry.name);
  free(file_system);
  // while not EOF
    // read command
    
    // parse command
    
    // call appropriate function

	return 0;
}
