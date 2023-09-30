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

typedef struct Node {
  dirent direntEntry;
  struct Node * child;
  struct Node * sibling;
  struct Node * parent;
} Node;

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
        //allocating more space in result for each new word found in command
        result = realloc(result, (i + 1) * sizeof(char *));
        result[i] = strdup(token);
        i++;
        token = strtok(NULL, "/");
    }

    free(copy);

    *parts = result;
    return i;
}

void free_paths(char **path, int length) {  
  for(int i = 0; i < length; i++){
    free(path[i]);
  }
  free(path);
}

int traverseUntilParent(Node ** temp, int count, char ** path, superblock * superblock){
  for(int i=0; i<count-1; i++){
    //printf("%s\n", temp->direntEntry.name);
    if(*temp==NULL){
      printf("Incorrect file path. (temp==NULL)\n");
      return -1;
    }
    if((*temp)->child==NULL){
      printf("Incorrect file path. (temp->child==NULL)\n");
      return -1;
    }
    int pathfound = 0;
    (*temp)=(*temp)->child;
    if(strcmp((*temp)->direntEntry.name, path[i])==0){
      pathfound=1;
    }
    else {
      while((*temp)->sibling != NULL){        
        (*temp) = (*temp)->sibling;      
        if(strcmp((*temp)->direntEntry.name, path[i])==0){          
          pathfound=1;          
          break;
        }
      }
    }
    if(pathfound==0){
      printf("Incorrect file path. (pathfound==0)\n");
      return -1;
    }
  }
  if(superblock->inode_list[(*temp)->direntEntry.inode].dir==0){
    printf("Incorrect file path. A file cannot contain other files/directories.\n");
    return -1;
  }
  return 0;
}

void create_file(char * filePath, int size, superblock * super_block, block * blocks[127], Node * root) {
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
  Node * temp = root;
  if(traverseUntilParent(&temp, count, path, super_block)==-1){
    return;
  }

  Node * parent = temp;
  //checking if path doesn't already exist
  if(temp->child!=NULL){
    if(strcmp(temp->child->direntEntry.name, fileName)==0){
      printf("File already exists.\n");
      return;
    } 
    else {
      Node * temp2 = temp->child;
      while(temp2->sibling != NULL){
        temp2 = temp2->sibling;
        if(strcmp(temp2->direntEntry.name, fileName)==0){
          printf("File already exists.\n");
          return;
        }
      }
    }
  }

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

  //set unused block pointers to NULL
  for(int i=numberOfBlocks; i<8; i++){
    super_block->inode_list[inodeNumber].blockptrs[i] = -1;
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

  //create Node
  Node * node = (Node *) malloc(sizeof(Node));
  node->direntEntry.inode = inodeNumber;
  node->direntEntry.namelen = strlen(fileName);
  strcpy(node->direntEntry.name, fileName);
  node->child = NULL;
  node->sibling = NULL;

  //add node to path
  //printf("%s\n", temp->direntEntry.name);
  if(temp->child==NULL){
    temp->child = node;
    temp->child->parent = parent;
  } else {
    temp=temp->child;
    while(temp->sibling != NULL){
      temp = temp->sibling;
    }
    temp->sibling=node;
    temp->sibling->parent = parent;
  }

  //propagate size changes up the tree
  while(parent!=NULL){
    super_block->inode_list[parent->direntEntry.inode].size = super_block->inode_list[parent->direntEntry.inode].size + size;
    parent = parent->parent;
  }

  //freeing memory
  free_paths(path, count);
}

void delete_file(char * filePath, superblock * super_block, Node * root) {
  //splitting path into an array of strings
  char** path;
  int count = parseFileName(filePath, &path);
  char* fileName = path[count-1];

  Node * temp = root;
  if(traverseUntilParent(&temp, count, path, super_block)==-1){
    return;
  }
  Node * parent = temp;
  int fileFound = 0;
  int fileSize = -1;
  //checking if file exists
  if(temp->child!=NULL){
    if(strcmp(temp->child->direntEntry.name, fileName)==0){
      if(super_block->inode_list[temp->child->direntEntry.inode].dir==1){
        printf("Not a file.\n");
        return;
      }
      Node * node = temp->child;
      fileSize = super_block->inode_list[node->direntEntry.inode].size;
      super_block->inode_list[node->direntEntry.inode].used = 0;
      for(int i=0; i<8; i++){
      if(super_block->inode_list[node->direntEntry.inode].blockptrs[i]!=-1){
              super_block->free_block_list[super_block->inode_list[node->direntEntry.inode].blockptrs[i]]='0';
      }
      }
      temp->child = node->sibling;
      free(node);
      fileFound = 1;
    } 
    else {
      temp = temp->child;
      while(temp->sibling != NULL){
        if(strcmp(temp->sibling->direntEntry.name, fileName)==0){
          if(super_block->inode_list[temp->sibling->direntEntry.inode].dir==1){
            printf("Not a file.\n");
            return;
          }
          Node * node = temp->sibling;
          fileSize = super_block->inode_list[node->direntEntry.inode].size;
          super_block->inode_list[node->direntEntry.inode].used = 0;
          for(int i=0; i<8; i++){
            if(super_block->inode_list[node->direntEntry.inode].blockptrs[i]!=-1){
              super_block->free_block_list[super_block->inode_list[node->direntEntry.inode].blockptrs[i]]='0';
            }
          }
          temp->sibling = node->sibling;
          free(node);          
          fileFound = 1;
          break;
        }
        temp = temp->sibling;
      }
    }
  }

  if(fileFound==0){
    printf("File not found.\n");
    return;
  }

  //super_block->inode_list[parent->direntEntry.inode].size = super_block->inode_list[parent->direntEntry.inode].size - fileSize;
  //propagate size changes up the tree
  while(parent!=NULL){
    super_block->inode_list[parent->direntEntry.inode].size = super_block->inode_list[parent->direntEntry.inode].size - fileSize;
    parent = parent->parent;
  }

  //freeing memory
  free_paths(path, count);
}

void create_directory(char * directoryPath, superblock * super_block, Node * root) {
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
  Node * temp = root;
  if(traverseUntilParent(&temp, count, path, super_block)==-1){
    return;
  }
  //checking if path doesn't already exist
  if(temp->child!=NULL){
    if(strcmp(temp->child->direntEntry.name, fileName)==0){
      printf("Directory already exists.\n");
      return;
    }
    else {
      Node * temp2 = temp->child;
      while(temp2->sibling != NULL){
        temp2 = temp2->sibling;
        if(strcmp(temp2->direntEntry.name, fileName)==0){
          printf("Directory already exists.\n");
          return;
        }
      }
    }
  }
  
  Node * parent = temp;

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

  //create Node
  Node * node = (Node *) malloc(sizeof(Node));
  node->direntEntry.inode = inodeNumber;
  node->direntEntry.namelen = strlen(fileName);
  strcpy(node->direntEntry.name, fileName);
  node->child = NULL;
  node->sibling = NULL;

  //add node to path
  if(temp->child==NULL){
    temp->child = node;
    temp->child->parent = parent;
  } else {
    temp=temp->child;
    while(temp->sibling != NULL){
      temp = temp->sibling;
    }
    temp->sibling=node;
    temp->sibling->parent = parent;
  }

  //freeing memory
  free_paths(path, count);
}

void delete_directory_recursively(superblock *super_block, Node *node) {
  if (node == NULL) {
      return;
  }
    
  Node * child = node->child;
  Node * sibling = node->sibling;
  super_block->inode_list[node->direntEntry.inode].used = 0;
  if(super_block->inode_list[node->direntEntry.inode].dir == 0){
    for(int i=0; i<8; i++){
      if(super_block->inode_list[node->direntEntry.inode].blockptrs[i]!=-1){
        super_block->free_block_list[super_block->inode_list[node->direntEntry.inode].blockptrs[i]]='0';
      }
    }
  }
  free(node);
  // Recursively delete child nodes
  delete_directory_recursively(super_block, child);
  // Traverse to the next sibling
  delete_directory_recursively(super_block, sibling);
}

void delete_directory(char * directoryPath, superblock * super_block, Node * root) {
  //splitting path into an array of strings
  char** path;
  int count = parseFileName(directoryPath, &path);
  char* fileName = path[count-1];

  Node * temp = root;
  if(traverseUntilParent(&temp, count, path, super_block)==-1){
    return;
  }
  Node * parent = temp;
  int fileFound = 0;
  int fileSize = -1;
  //checking if file exists
  if(temp->child!=NULL){
    if(strcmp(temp->child->direntEntry.name, fileName)==0){
      if(super_block->inode_list[temp->child->direntEntry.inode].dir==0){
        printf("Not a directory.\n");
        return;
      }
      Node * node = temp->child;
      fileSize = super_block->inode_list[node->direntEntry.inode].size;
      super_block->inode_list[node->direntEntry.inode].used = 0;      
      temp->child = node->sibling;
      if(node->child!=NULL){
        delete_directory_recursively(super_block, node->child);
      }
      free(node);
      fileFound = 1;
    } 
    else {
      temp = temp->child;
      while(temp->sibling != NULL){
        if(strcmp(temp->sibling->direntEntry.name, fileName)==0){
          if(super_block->inode_list[temp->sibling->direntEntry.inode].dir==0){
            printf("Not a directory.\n");
            return;
          }
          Node * node = temp->sibling;
          temp->sibling = node->sibling;
          fileSize = super_block->inode_list[node->direntEntry.inode].size;
          super_block->inode_list[node->direntEntry.inode].used = 0;
          for(int i=0; i<8; i++){
            if(super_block->inode_list[node->direntEntry.inode].blockptrs[i]!=-1){
              super_block->free_block_list[super_block->inode_list[node->direntEntry.inode].blockptrs[i]]='0';
            }
          }          
          if(node->child!=NULL){
            delete_directory_recursively(super_block, node->child);
          }
          free(node);
          fileFound = 1;
          break;
        }
        temp = temp->sibling;
      }
    }
  }

  if(fileFound==0){
    printf("File not found.\n");
    return;
  }

  //super_block->inode_list[parent->direntEntry.inode].size = super_block->inode_list[parent->direntEntry.inode].size - fileSize;
  //propagate size changes up the tree
  while(parent!=NULL){
    super_block->inode_list[parent->direntEntry.inode].size = super_block->inode_list[parent->direntEntry.inode].size - fileSize;
    parent = parent->parent;
  }

  //freeing memory
  free_paths(path, count);
}

void recursively_list_all_files(superblock *super_block, Node *node, const char *basePath) {
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
    recursively_list_all_files(super_block, node->child, path);

    // Traverse to the next sibling
    recursively_list_all_files(super_block, node->sibling, basePath);
}

void list_all_files(superblock *super_block, Node *rootNode) {
    printf("%s, %d\n", 
        rootNode->direntEntry.name, 
        super_block->inode_list[rootNode->direntEntry.inode].size);
    
    // Start with an empty base path
    char basePath[] = "";
    recursively_list_all_files(super_block, rootNode->child, basePath);
    printf("\n");
}

void recursively_save_all_files(superblock *super_block, Node *node, const char *basePath, FILE * fstream) {
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
    fprintf(fstream, "%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
    super_block->inode_list[node->direntEntry.inode].dir,
    path, 
    super_block->inode_list[node->direntEntry.inode].size,
    super_block->inode_list[node->direntEntry.inode].blockptrs[0],
    super_block->inode_list[node->direntEntry.inode].blockptrs[1],
    super_block->inode_list[node->direntEntry.inode].blockptrs[2],
    super_block->inode_list[node->direntEntry.inode].blockptrs[3],
    super_block->inode_list[node->direntEntry.inode].blockptrs[4],
    super_block->inode_list[node->direntEntry.inode].blockptrs[5],
    super_block->inode_list[node->direntEntry.inode].blockptrs[6],
    super_block->inode_list[node->direntEntry.inode].blockptrs[7]);
    

    // Recursively process child nodes
    recursively_save_all_files(super_block, node->child, path, fstream);
    // Traverse to the next sibling
    recursively_save_all_files(super_block, node->sibling, basePath, fstream);
}

void save_all_files(superblock *super_block, Node *rootNode, FILE * fstream) {
    // fprintf(fstream, "%s, %d\n", 
    //     rootNode->direntEntry.name, 
    //     super_block->inode_list[rootNode->direntEntry.inode].size);
    
    // Start with an empty base path
    char basePath[] = "";

    recursively_save_all_files(super_block, rootNode->child, basePath, fstream);
    fprintf(fstream, "\n");
}

void copy_file(char * sourcePath, char * destinationPath, superblock * super_block, block * blocks[127], Node * root){
  //splitting paths into an arrays of strings
  char** srcPathArr;
  int count = parseFileName(sourcePath, &srcPathArr);
  char* srcFileName = srcPathArr[count-1];
  
  char** destPathArr;
  int count2 = parseFileName(destinationPath, &destPathArr);
  char* destFileName = destPathArr[count2-1];

  Node * temp1 = root;
  Node * temp2 = root;
  if(traverseUntilParent(&temp1, count, srcPathArr, super_block)==-1){
    return;
  }
  if(traverseUntilParent(&temp2, count2, destPathArr, super_block)==-1){
    return;
  }
  //parent of srcPath is not required
  //Node * parent1 = temp1;
  Node * parent2 = temp2;

  int srcFileFound = 0;
  if(temp1->child!=NULL){
    if(strcmp(temp1->child->direntEntry.name, srcFileName)==0){
      temp1=temp1->child;
      srcFileFound=1;
    }
    else {
      temp1 = temp1->child;
      while(temp1->sibling != NULL){
        temp1 = temp1->sibling;
        if(strcmp(temp1->direntEntry.name, srcFileName)==0){        
          srcFileFound=1;
          break;
        }
      }
    }
  }
  if(srcFileFound==0){
    printf("Source file not found.\n");
    return;
  }
  if(super_block->inode_list[temp1->direntEntry.inode].dir==1){
    printf("Cannot handle directories.\n");
    return;
  }

  int destFileFound = 0;
  int previousDestSize;
  if(temp2->child!=NULL){
    if(strcmp(temp2->child->direntEntry.name, destFileName)==0){
      temp2=temp2->child;
      destFileFound=1;
    }
    else {
      temp2 = temp2->child;
      while(temp2->sibling != NULL){
        temp2 = temp2->sibling;
        if(strcmp(temp2->direntEntry.name, destFileName)==0){
          destFileFound=1;
          break;
        }
      }
    }
  }

  int inodeNumber = -1;

  if(destFileFound==0){
    temp2 = (Node *) malloc(sizeof(Node));
    if(parent2->child!=NULL){
      Node * sibling = parent2->child;
      while (sibling->sibling!=NULL)
      {
        sibling = sibling->sibling;
      }
      sibling->sibling = temp2;            
    } else {
      parent2->child = temp2;
    }
  } else {
    inodeNumber = temp2->direntEntry.inode;
  }
  int numberOfBlocks = ceil((double)super_block->inode_list[temp1->direntEntry.inode].size/sizeof(block));
  //printf("size in beginning: %d \n", super_block->inode_list[temp1->direntEntry.inode].size);
  int newFileBlockPtrs[numberOfBlocks];
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

  if(inodeNumber==-1){
    for(int i=0; i<16; i++){
      if(super_block->inode_list[i].used == 0){
        super_block->inode_list[i].used = 1;
        super_block->inode_list[i].dir = 0;
        strcpy(super_block->inode_list[i].name, destFileName);
        super_block->inode_list[i].size = super_block->inode_list[temp1->direntEntry.inode].size;
        for(int j=0; j<numberOfBlocks; j++){
          super_block->inode_list[i].blockptrs[j] = newFileBlockPtrs[j];
        }      
        inodeNumber = i;
        break;
      }
    }
  }
  for(int i=numberOfBlocks; i<8; i++){
    super_block->inode_list[inodeNumber].blockptrs[i] = -1;
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
      //copying data from temp1 into temp2 i.e. source to destination
      blocks[newFileBlockPtrs[i]]->data[j] = blocks[super_block->inode_list[temp1->direntEntry.inode].blockptrs[i]]->data[j];
    }    
  }
  //int destFileSize = super_block->inode_list[temp2->direntEntry.inode].size;
  Node * copiedNode = temp2;
  copiedNode->direntEntry.inode = inodeNumber;
  copiedNode->direntEntry.namelen = strlen(destFileName);
  strcpy(copiedNode->direntEntry.name, destFileName);
  copiedNode->child = NULL;
  copiedNode->parent = parent2;
  if(destFileFound==0){
    copiedNode->sibling = NULL;
  }
  int destFileSize = super_block->inode_list[temp1->direntEntry.inode].size;
  super_block->inode_list[copiedNode->direntEntry.inode].size = destFileSize;
  while(parent2!=NULL){
    super_block->inode_list[parent2->direntEntry.inode].size = super_block->inode_list[parent2->direntEntry.inode].size + destFileSize;
    parent2 = parent2->parent;
  }

  //freeing memory
  free_paths(srcPathArr, count);
  free_paths(destPathArr, count2);
}

void move_file(char * sourcePath, char * destinationPath, superblock * super_block, block * blocks[127], Node * root){
  //splitting paths into an arrays of strings
  char** srcPathArr;
  int count = parseFileName(sourcePath, &srcPathArr);
  char* srcFileName = srcPathArr[count-1];
  
  char** destPathArr;
  int count2 = parseFileName(destinationPath, &destPathArr);
  char* destFileName = destPathArr[count2-1];

  Node * temp1 = root;
  Node * temp2 = root;
  if(traverseUntilParent(&temp1, count, srcPathArr, super_block)==-1){
    return;
  }
  if(traverseUntilParent(&temp2, count2, destPathArr, super_block)==-1){
    return;
  }

  Node * parent1 = temp1;
  Node * parent2 = temp2;

  Node * srcNode = NULL;
  int srcFileFound = 0;
  if(temp1->child!=NULL){
    if(strcmp(temp1->child->direntEntry.name, srcFileName)==0){
      srcNode=temp1->child;
      if(super_block->inode_list[srcNode->direntEntry.inode].dir==1){
        printf("Cannot handle directories.\n");
        return;
      }
      temp1->child=srcNode->sibling;
      srcFileFound=1;
    }
    else {
      temp1 = temp1->child;
      while(temp1->sibling != NULL){        
        if(strcmp(temp1->sibling->direntEntry.name, srcFileName)==0){      
          srcNode=temp1->sibling;
          if(super_block->inode_list[srcNode->direntEntry.inode].dir==1){
            printf("Cannot handle directories.\n");
            return;
          }
          temp1->sibling=srcNode->sibling;
          srcFileFound=1;
          break;
        }
        temp1 = temp1->sibling;
      }
    }
  }
  if(srcFileFound==0){
    printf("Source file not found.\n");
    return;
  }

  //Node * destFile = NULL;
  int destFileFound = 0;
  int previousDestSize;
  if(temp2->child!=NULL){
    if(strcmp(temp2->child->direntEntry.name, destFileName)==0){
      Node * destNode = temp2->child;
      previousDestSize = super_block->inode_list[destNode->direntEntry.inode].size;
      temp2->child = srcNode;
      srcNode->sibling = destNode->sibling;
      //delete destNode
      super_block->inode_list[destNode->direntEntry.inode].used = 0;
      for(int i=0; i<8; i++){
        if(super_block->inode_list[destNode->direntEntry.inode].blockptrs[i]!=-1){
          super_block->free_block_list[super_block->inode_list[destNode->direntEntry.inode].blockptrs[i]]='0';
        }
      }      
      free(destNode);
      destFileFound=1;
    }
    else {
      temp2 = temp2->child;
      while(temp2->sibling != NULL){        
        if(strcmp(temp2->sibling->direntEntry.name, destFileName)==0){
          Node * destNode = temp2->sibling;
          previousDestSize = super_block->inode_list[destNode->direntEntry.inode].size;
          temp2->sibling = srcNode;
          srcNode->sibling = destNode->sibling;
          //delete destNode
          super_block->inode_list[destNode->direntEntry.inode].used = 0;
          for(int i=0; i<8; i++){
            if(super_block->inode_list[destNode->direntEntry.inode].blockptrs[i]!=-1){
              super_block->free_block_list[super_block->inode_list[destNode->direntEntry.inode].blockptrs[i]]='0';
            }
          }      
          free(destNode);
          destFileFound=1;
          break;
        }
        temp2 = temp2->sibling;
      }
    }
  }

  //going back to the parent to add the new node
  temp2=parent2;
  if(destFileFound==0){
    if(temp2->child==NULL){
      temp2->child = srcNode;
      srcNode->sibling = NULL;
    } 
    else {
      temp2=temp2->child;
      while(temp2->sibling != NULL){
        temp2 = temp2->sibling;
      }
      temp2->sibling=srcNode;
      srcNode->sibling = NULL;
    }
  }
  //propagating size changes up the tree
  while(parent1!=NULL){
    super_block->inode_list[parent1->direntEntry.inode].size = super_block->inode_list[parent1->direntEntry.inode].size - super_block->inode_list[srcNode->direntEntry.inode].size;
    parent1 = parent1->parent;
  }
  while(parent2!=NULL){
    if(destFileFound==1){
      super_block->inode_list[parent2->direntEntry.inode].size = super_block->inode_list[parent2->direntEntry.inode].size - previousDestSize;  
    }
    super_block->inode_list[parent2->direntEntry.inode].size = super_block->inode_list[parent2->direntEntry.inode].size + super_block->inode_list[srcNode->direntEntry.inode].size;
    parent2 = parent2->parent;
  }

  //freeing memory
  free_paths(srcPathArr, count);
  free_paths(destPathArr, count2);
}

void recursively_free_all_nodes(Node * node) {
    if (node == NULL) {
        return;
    }

    Node * child = node->child;
    Node * sibling = node->sibling;

    free(node);

    recursively_free_all_nodes(child);

    recursively_free_all_nodes(sibling);
}

void initialize(){

}


// Function to save the state of the filesystem to a file
int save_state(const char *filename, superblock *super_block, Node *root, block *blocks[127]) {
    FILE *fstream = fopen(filename, "w");
    //save path and block pointers
    save_all_files(super_block, root, fstream);
    //save block data
    for(int i=0; i<127; i++){
      fprintf(fstream, "%.*s\n", 1024, blocks[i]->data);
    }
    fclose(fstream);
    return 0;
}

void regenerate_file(char * filePath, int size, int * block_ptrs, superblock * super_block, block * blocks[127], Node * root) {
  int numberOfBlocks = ceil((double)size/sizeof(block));

  //splitting path into an array of strings
  char** path;
  int count = parseFileName(filePath, &path);
  char* fileName = path[count-1];

  //checking if path exists
  Node * temp = root;
  if(traverseUntilParent(&temp, count, path, super_block)==-1){
    return;
  }

  Node * parent = temp;
  //checking if path doesn't already exist
  if(temp->child!=NULL){
    if(strcmp(temp->child->direntEntry.name, fileName)==0){
      printf("File already exists.\n");
      return;
    } 
    else {
      Node * temp2 = temp->child;
      while(temp2->sibling != NULL){
        temp2 = temp2->sibling;
        if(strcmp(temp2->direntEntry.name, fileName)==0){
          printf("File already exists.\n");
          return;
        }
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
        super_block->inode_list[i].blockptrs[j] = block_ptrs[j];
      }      
      inodeNumber = i;
      break;
    }
  }

  //set unused block pointers to NULL
  for(int i=numberOfBlocks; i<8; i++){
    super_block->inode_list[inodeNumber].blockptrs[i] = -1;
  }

  if(inodeNumber==-1){
    printf("Not enough free space.\n");
    return;
  }

  //As no errors have been found, now we will start updating our data structures

  for(int i=0; i<numberOfBlocks; i++){
    //update free block list
    super_block->free_block_list[block_ptrs[i]]='1';      
  }

  //create Node
  Node * node = (Node *) malloc(sizeof(Node));
  node->direntEntry.inode = inodeNumber;
  node->direntEntry.namelen = strlen(fileName);
  strcpy(node->direntEntry.name, fileName);
  node->child = NULL;
  node->sibling = NULL;

  //add node to path
  //printf("%s\n", temp->direntEntry.name);
  if(temp->child==NULL){
    temp->child = node;
    temp->child->parent = parent;
  } else {
    temp=temp->child;
    while(temp->sibling != NULL){
      temp = temp->sibling;
    }
    temp->sibling=node;
    temp->sibling->parent = parent;
  }

  //propagate size changes up the tree
  while(parent!=NULL){
    super_block->inode_list[parent->direntEntry.inode].size = super_block->inode_list[parent->direntEntry.inode].size + size;
    parent = parent->parent;
  }

  //freeing memory
  free_paths(path, count);
}

int load_state(const char *filename, superblock *super_block, Node * rootNode, block *blocks[127]) {
  FILE *fstream = fopen(filename, "r");
  if (fstream == NULL) {
      printf("File doesn't exist. Continuing without file\n");
      return -1; // Error opening the file
  }

    // Read and process the file paths and block pointers
  char line[512]; // Adjust the buffer size as needed
  int isBlockDataSection = 0;
  int blockIndex = 0;  
  
  while (fgets(line, sizeof(line), fstream) != NULL) {
    if (strcmp(line, "\n") == 0) {
      isBlockDataSection = 1;
      continue;
    }
      
    if (!isBlockDataSection) {
      // Tokenize the line to extract data
      char *token = strtok(line, ", ");
      if (token != NULL) {
        int dir = atoi(token);
        char path[256];
        strcpy(path, strtok(NULL, ", "));
        int size = atoi(strtok(NULL, ", "));
        int blockptrs[8];
        for (int i = 0; i < 8; i++) {
          token = strtok(NULL, ", ");
          blockptrs[i] = atoi(token);
        }
          printf("%d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
          dir,
          path,
          size,
          blockptrs[0],
          blockptrs[1],
          blockptrs[2],
          blockptrs[3],
          blockptrs[4],
          blockptrs[5],
          blockptrs[6],
          blockptrs[7]);

          if(dir==1){
            create_directory(path, super_block, rootNode);
          } else {
            regenerate_file(path, size, blockptrs, super_block, blocks, rootNode);
          }

      }
    } else {                      
      for(int i = 0; i < 1024; i++){
        blocks[blockIndex]->data[i] = line[i];
      }
      blockIndex++;
    }
  }

  fclose(fstream);
  return 0; // Success
}

/*
 * main
 */
int main (int argc, char* argv[]) {
  if(argc != 2){
    printf("Incorrect number of arguments.\n");
    return 0;
  }
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
  //root Node
  Node * rootNode = (Node *) malloc(sizeof(Node));
  rootNode->direntEntry.inode = 0;
  rootNode->direntEntry.namelen = 1;
  strcpy(rootNode->direntEntry.name, "/");
  rootNode->parent = NULL;
  rootNode->child = NULL;
  rootNode->sibling = NULL;

  load_state("myfs", super_block, rootNode, blocks);

  FILE * fstream = fopen(argv[1], "r");
  char line[256];
  while(fgets(line, sizeof(line), fstream)!=NULL)
  {
    char * result[10]; //maximum 9 arguments
    int i = 0;

    // Remove trailing newline characters
    int length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
      line[length - 1] = '\0';
    }

    char * command;
    command = strdup(line); //preserving line in case of error
    char * token;
    token = strtok(command, " ");
    while (token != NULL) {
      result[i] = strdup(token);
      i++;
      token = strtok(NULL, " "); 
    }
    if(strcmp(result[0], "CD")==0){
      create_directory(result[1], super_block, rootNode);
    }
    else if(strcmp(result[0], "CR")==0){
      printf("%s\n", result[1]);
      create_file(result[1], atoi(result[2]), super_block, blocks, rootNode);
    }
    else if(strcmp(result[0], "CP")==0){
      copy_file(result[1], result[2], super_block, blocks, rootNode);
    }
    else if(strcmp(result[0], "MV")==0){
      move_file(result[1], result[2], super_block, blocks, rootNode);
    }
    else if(strcmp(result[0], "DL")==0){
      delete_file(result[1], super_block, rootNode);
    }
    else if(strcmp(result[0], "DD")==0){
      delete_directory(result[1], super_block, rootNode);
    }
    else if(strcmp(result[0], "LL")==0){
      list_all_files(super_block, rootNode);
    }
    else {
      printf("%s is an incorrect Command.\n", line);
    }
  }
  fclose(fstream);

  list_all_files(super_block, rootNode);

  save_state("myfs", super_block, rootNode, blocks);

  recursively_free_all_nodes(rootNode);
  free(file_system);

	return 0;



  //testing
  // create_file("/test.txt", 1024, super_block, blocks, rootNode);
  // create_file("/nest.txt", 1024, super_block, blocks, rootNode);
  // create_directory("/frost", super_block, rootNode);
  // create_directory("/fire", super_block, rootNode);
  // create_file("/frost/best.txt", 1024, super_block, blocks, rootNode);
  // create_file("/frost/yest.txt", 8000, super_block, blocks, rootNode);
  // create_directory("/frost/yosh", super_block, rootNode);
  // create_file("/frost/yosh/mest.txt", 1024, super_block, blocks, rootNode);
  // create_file("/frost/yosh/mest.txt", 1024, super_block, blocks, rootNode);
  
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // //delete_file("/frost/yosh/mest.txt", super_block, rootNode);
  // delete_file("/test.txt", super_block, rootNode);
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // delete_directory("/frost/yosh", super_block, rootNode);
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // copy_file("/nest.txt", "/frost/nest.txt", super_block, blocks, rootNode);
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // copy_file("/frost/yest.txt", "/nest.txt", super_block, blocks, rootNode);
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // move_file("/frost/nest.txt", "/fire/nest.txt", super_block, blocks, rootNode);
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // move_file("/nest.txt", "/fire/nest.txt", super_block, blocks, rootNode);
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // move_file("/frost/best.txt", "/fire/best.txt", super_block, blocks, rootNode);
  // list_all_files(super_block, rootNode);
  // printf("\n");
  // for(int i = 0; i < 8; i++) {
  //   printf("%s\n", blocks[super_block->inode_list[rootNode->child->child->direntEntry.inode].blockptrs[i]]->data);
  // }
  //listAllDirectories(super_block, rootNode);
}
