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

int parseFileName(const char *path, char ***parts) {
    char *token;
    char *copy = strdup(path); // Make a copy to avoid modifying the original string
    char **result = NULL;
    int i = 0;

    token = strtok(copy, "/");
    while (token != NULL) {
        result = realloc(result, (i + 1) * sizeof(char *));
        result[i] = strdup(token);
        i++;
        token = strtok(NULL, "/");
    }

    free(copy);

    *parts = result;
    return i;
}

void create_file(char * filePath, int size, superblock * super_block, block * blocks[127], pathNode * root) {
  // assuming size is in bytes
  // find a free inode
  // find a free block
  // update inode
  // update block
  // update superblock
  // inode->dirent->pathNode

  int numberOfBlocks = ceil((double)size/sizeof(block));
  if(numberOfBlocks > 8) {
    printf("File size too large.\n");
    return;
  }
  int newFileBlockPtrs[numberOfBlocks];

  //splitting path into an array of strings
  char** path;
  int count = parseFileName(filePath, &path);
  char* fileName = path[count-1];

  //checking if path exists
  pathNode * temp = root;
  for(int i=0; i<count-1; i++){
    //printf("%s\n", temp->direntEntry.name);
    if(temp==NULL){
      printf("Incorrect file path. (temp==NULL)\n");
      return;
    }
    if(temp->child==NULL){
      printf("Incorrect file path. (temp->child==NULL)\n");
      return;
    }
    int pathfound = 0;
    temp=temp->child;
    if(temp->direntEntry.name==path[i]){
      pathfound=1;
    }
    else {
      while(temp->sibling != NULL){
        temp = temp->sibling;
        if(strcmp(temp->direntEntry.name, path[i])==0){
          pathfound=1;
          break;
        }
      }
    }
    if(pathfound==0){
      printf("Incorrect file path. (pathfound==0)\n");
      return;
    }
  }
  //checking if path doesn't already exist
  if(temp->child!=NULL){
    if(strcmp(temp->child->direntEntry.name, fileName)==0){
      printf("File already exists.\n");
      return;
    } 
    else {
      pathNode * temp2 = temp->child;
      while(temp2->sibling != NULL){
        temp2 = temp2->sibling;
        if(strcmp(temp2->direntEntry.name, fileName)==0){
          printf("File already exists.\n");
          return;
        }
      }
      //free(temp2);
    }
  }
  // while(temp->sibling != NULL){
  //   temp = temp->sibling;
  //   if(strcmp(temp->direntEntry.name, fileName)==0){
  //     printf("File already exists.\n");
  //     return;
  //   }
  // }

  //find free blocks and storing them in newFileBlockPtrs, if not enough free blocks, return
  for(int i=0; i<numberOfBlocks; i++){
    for(int j=1; j<128; j++){
      if(super_block->free_block_list[j]=='0'){        
        newFileBlockPtrs[i]=j;
        break;
      }
      if(j==127){
        printf("Not enough free space.\n");
        return;
      }
    }
  }

  int inodeNumber = -1;
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

  if(inodeNumber==-1){
    printf("Not enough free space.\n");
    return;
  }

  //As no errors have been found, now we will start updating our data structures

  for(int i=0; i<numberOfBlocks; i++){
    //update free block list
    super_block->free_block_list[newFileBlockPtrs[i]]='1';    
    //insert data into block
    for(int j=0; j<1024; j++){
      //cycling through the alphabet by incrementing ascii values
      blocks[newFileBlockPtrs[i]]->data[j] = 'a'+(j)%26;
    }    
  }

  //create pathNode
  pathNode * node = (pathNode *) malloc(sizeof(pathNode));
  node->direntEntry.inode = inodeNumber;
  node->direntEntry.namelen = strlen(fileName);
  strcpy(node->direntEntry.name, fileName);
  node->child = NULL;
  node->sibling = NULL;

  //add node to path
  //printf("%s\n", temp->direntEntry.name);
  if(temp->child==NULL){
    temp->child = node;
  } else {
    temp=temp->child;
    while(temp->sibling != NULL){
      temp = temp->sibling;
    }
    temp->sibling=node;
  }
}

void create_directory(char * directoryPath, superblock * super_block, pathNode * root) {
  // find a free inode
  // find a free block
  // update inode
  // update block
  // update superblock
  //splitting path into an array of strings
  char** path;
  int count = parseFileName(directoryPath, &path);
  char* fileName = path[count-1];

  //checking if path exists
  pathNode * temp = root;
  for(int i=0; i<count-1; i++){
    if(temp==NULL){
      printf("Incorrect file path.\n");
      return;
    }
    if(temp->child==NULL){
      printf("Incorrect file path.\n");
      return;
    }
    int pathfound = 0;
    temp=temp->child;
    if(temp->direntEntry.name==path[i]){
      pathfound=1;
    }
    else {
      while(temp->sibling != NULL){
        temp = temp->sibling;
        if(strcmp(temp->direntEntry.name, path[i])==0){
          pathfound=1;
          break;
        }
      }
    }
    if(pathfound==0){
      printf("Incorrect file path.\n");
      return;
    }
  }
  //checking if path doesn't already exist
  if(temp->child!=NULL){
    if(strcmp(temp->child->direntEntry.name, fileName)==0){
      printf("Directory already exists.\n");
      return;
    }
    else {
      pathNode * temp2 = temp->child;
      while(temp2->sibling != NULL){
        temp2 = temp2->sibling;
        if(strcmp(temp2->direntEntry.name, fileName)==0){
          printf("Directory already exists.\n");
          return;
        }
      }
    }
  }
  

  int inodeNumber = -1;
  for(int i=0; i<16; i++){
    if(super_block->inode_list[i].used == 0){
      super_block->inode_list[i].used = 1;
      super_block->inode_list[i].dir = 1;
      strcpy(super_block->inode_list[i].name, fileName);
      super_block->inode_list[i].size = 0;   
      inodeNumber = i;
      break;
    }
  }

  if(inodeNumber==-1){
    printf("Not enough free space.\n");
    return;
  }

  //As no errors have been found, now we will start updating our data structures

  //create pathNode
  pathNode * node = (pathNode *) malloc(sizeof(pathNode));
  node->direntEntry.inode = inodeNumber;
  node->direntEntry.namelen = strlen(fileName);
  strcpy(node->direntEntry.name, fileName);
  node->child = NULL;
  node->sibling = NULL;

  //add node to path
  if(temp->child==NULL){
    temp->child = node;
  } else {
    temp=temp->child;
    while(temp->sibling != NULL){
      temp = temp->sibling;
    }
    temp->sibling=node;
  }
}


void recursiveListAllFiles(superblock *super_block, pathNode *node, const char *basePath) {
    if (node == NULL) {
        return;
    }
    
    char path[256]; // Assuming a reasonable maximum path length

    // Copy the basePath to the path buffer
    strcpy(path, basePath);

    // Append the current node's name to the path
    if (strcmp(path, "/") != 0) {
        strcat(path, "/");
    }
    strcat(path, node->direntEntry.name);

    printf("%s, %d\n", path, super_block->inode_list[node->direntEntry.inode].size);

    // Recursively process child nodes
    recursiveListAllFiles(super_block, node->child, path);

    // Traverse to the next sibling
    recursiveListAllFiles(super_block, node->sibling, basePath);
}

void listAllFiles(superblock *super_block, pathNode *rootNode) {
    printf("%s, %d\n", 
        rootNode->direntEntry.name, 
        super_block->inode_list[rootNode->direntEntry.inode].size);
    
    // Start with an empty base path
    char basePath[] = "";
    recursiveListAllFiles(super_block, rootNode->child, basePath);
}

void initializeRoot(){

}

void saveFileSystem(){

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
  //root inode
  super_block->inode_list->dir = 1;
  strcpy(super_block->inode_list->name, "/");
  super_block->inode_list->size = 0;
  super_block->inode_list->used = 1;
  super_block->inode_list->blockptrs[0]=0;
  //root pathNode
  pathNode * rootNode = (pathNode *) malloc(sizeof(pathNode));
  rootNode->direntEntry.inode = 0;
  rootNode->direntEntry.namelen = 1;
  strcpy(rootNode->direntEntry.name, "/");
  rootNode->child = NULL;
  rootNode->sibling = NULL;

  //testing
  create_file("/test.txt", 1024, super_block, blocks, rootNode);
  create_file("/nest.txt", 1024, super_block, blocks, rootNode);
  create_directory("/frost", super_block, rootNode);
  create_directory("/fire", super_block, rootNode);
  //create_file("/crost/foo.txt", 1024, super_block, blocks, rootNode);
  create_file("/frost/best.txt", 1024, super_block, blocks, rootNode);
  create_file("/frost/yest.txt", 8000, super_block, blocks, rootNode);
  create_directory("/frost/yosh", super_block, rootNode);
  create_file("/frost/yosh/mest.txt", 1024, super_block, blocks, rootNode);
  create_file("/frost/yosh/mest.txt", 1024, super_block, blocks, rootNode);
  
  listAllFiles(super_block, rootNode);
  //listAllDirectories(super_block, rootNode);

  // while not EOF
    // read command
    
    // parse command
    
    // call appropriate function

  free(file_system);

  

	return 0;
}
