/**
    Group #15
    Authors: Brennan Grady, Xavier Molyneaux, Thinh Nguyen
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define BLOCK_SIZE 1024
#define INODE_SIZE 64
#define FREE_ARRAY_SIZE 200

typedef struct {
    int isize;
    int fsize;
    int nfree;
    unsigned int free[FREE_ARRAY_SIZE];
    char flock;
    char ilock;
    char fmod;
    unsigned int time;
} superblock_type; // Block size is 1024 Bytes; not all bytes of superblock are used.

typedef struct {
    unsigned short flags;
    unsigned short nlinks;
    unsigned int uid;
    unsigned int gid;
    unsigned int size0;
    unsigned int size1;
    unsigned int addr[9];
    unsigned int actime;
    unsigned int modtime;
} inode_type;

typedef struct { 
    unsigned int inode;
    char filename[28];
} dir_type;  //32 Bytes long

typedef struct {
    unsigned short nfree;
    unsigned short free[FREE_ARRAY_SIZE];
} free_block;

superblock_type superblock;
int fd = -1;

int create_and_open_fs(char *file_name);
int open_fs(char *file_name);
void inode_writer(int inum, inode_type inode);
void block_writer(int bidx, void* buffer, int bufferSize);
void superblock_writer();
void superblock_reader();
inode_type inode_reader(int inum, inode_type inode);
void fill_an_inode_and_write(int inum, int number_diskBlock_for_inodes);

void initfs(int number_diskBlock, int number_diskBlock_for_inodes);
int add_free_block(int blockIdx);
int get_free_block();

// The main function
int main(int argc, char *argv[]) {
    char *file_name;
    int number_diskBlock = 1024;
    int number_diskBlock_for_inodes = 256;

    system("clear");
    printf("\n... UNIX V6 FILE SYSTEM ...");
    while(1) {
        char input[256];
        char *arg0, *arg1, *arg2;
        
        printf("\n... Usage: ...\n");
        printf(" 1. initfs <file_name> <n1 - file system size in # blocks> <n2 = number of blocks for inodes> (Initialize ile system)\n");
        printf(" 2. q (Quit the program)\n\n");
        printf("Enter your command: ");
        scanf(" %[^\n]s", input);

        arg0 = strtok(input, " ");
        
        if(strcmp(arg0, "initfs")==0) {    
            if (fd >= 0) close(fd);
            file_name = strtok(NULL, " ");
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");

            if (atoi(arg1) > FREE_ARRAY_SIZE + 3) number_diskBlock = atoi(arg1);
            if (atoi(arg2) > 2) number_diskBlock_for_inodes = atoi(arg2);

            if (access(file_name, F_OK) != -1) { // file exists
                printf("The file system already exists. Loading existed file system\n");
                open_fs(file_name);
                superblock_reader();
            }
            else {  // file does not exist                
                printf("Creating File System\n");
                create_and_open_fs(file_name);
                initfs(number_diskBlock, number_diskBlock_for_inodes);
                fill_an_inode_and_write(1, superblock.isize);
            }

            // 1st inode is root
            inode_type inode1;
            inode1 = inode_reader(1,inode1);
            
            printf("Value of inode1's addr[0] is %d\n", inode1.addr[0]);
            printf("Value of inode1's addr[1] is %d\n", inode1.addr[1]);
        }
        else if (strcmp(arg0, "q")==0) {
            if (fd >= 0) {
                superblock_writer(); // save superblock to the disk before closing the program
                close(fd);
            }
            exit(0);
        }
    }
}

int open_fs(char *file_name) {
    fd = open(file_name, O_RDWR);

    if (fd == 1) {
        return -1;
    }
    else {
        return 1;
    }
}

int create_and_open_fs(char *file_name) {
    fd = open(file_name, O_RDWR | O_CREAT, 0600);

    if(fd == -1){
        return -1;
    }
    else{
        return 1;
    }
}

void initfs(int number_diskBlock, int number_diskBlock_for_inodes) {
    // generate super block
    superblock.isize = number_diskBlock_for_inodes;
    superblock.fsize = number_diskBlock;

    superblock.nfree = 0;
    for(int i = 0; i < FREE_ARRAY_SIZE; i++)
        superblock.free[i] = 0;
    
    superblock.flock = 'f'; // dump data
    superblock.ilock = 'i'; // dump data
    superblock.fmod = 'f'; // dump data

    superblock.time = (unsigned int)time(0);

    // initialize all disk blocks
    char zeroArray[BLOCK_SIZE] = {0};
    block_writer(number_diskBlock - 1, zeroArray, BLOCK_SIZE);

    // write superblock
    superblock_writer();

    // generate all free blocks, except the first one for root
    for (int blockIdx = 2 + number_diskBlock_for_inodes + 1; blockIdx < number_diskBlock; blockIdx++) {
        add_free_block(blockIdx);
    }
}

void block_writer(int bidx, void* buffer, int bufferSize) {
    lseek(fd, BLOCK_SIZE * bidx, SEEK_SET); 
    write(fd, &buffer, bufferSize);
}

// Function to write inode
void inode_writer(int inum, inode_type inode) {
    lseek(fd, 2 * BLOCK_SIZE + (inum-1) * INODE_SIZE , SEEK_SET); 
    write(fd, &inode, sizeof(inode));
}

// Function to write super block
void superblock_writer() {
    lseek(fd, BLOCK_SIZE, SEEK_SET); // superblock is in the 2nd block
    write(fd, &superblock, sizeof(superblock));
}

// Function to read super block
void superblock_reader() {
    lseek(fd, BLOCK_SIZE, SEEK_SET);
    read(fd, &superblock, sizeof(superblock));
}

// Function to read inodes
inode_type inode_reader(int inum, inode_type inode) {
    lseek(fd, 2 * BLOCK_SIZE + (inum-1) * INODE_SIZE, SEEK_SET); 
    read(fd, &inode, sizeof(inode));
    return inode;
}

// Function to write inode number after filling some fileds
void fill_an_inode_and_write(int inum, int number_diskBlock_for_inodes) {
    // total # inodes = number_diskBlock_for_inodes * BLOCK_SIZE / INODE_SIZE
    if (inum > number_diskBlock_for_inodes * BLOCK_SIZE / INODE_SIZE) return;
    
    else if (inum == 1) {    // create root
        /**     for number of blocks devoted for inodes = 256
        *       0 | 1 | 2 |  ... | 256 | 257 | 258 |... 
        *         | inodes               | data block
        */
        
        inode_type root;

        root.flags |= 1 << 15; //Root is allocated
        root.flags |= 1 << 14; //It is a directory
        root.actime = time(NULL);
        root.modtime = time(NULL);

        root.size0 = 0;
        root.size1 = 2 * sizeof(dir_type);
        root.addr[0] = number_diskBlock_for_inodes + 2;
        
        for (int i=1;i<9;i++) 
            root.addr[i]=-1; // all other addr elements are null so setto -1
        
        inode_writer(inum, root);
    } else {

    }
}

// Function to release an unused block
int add_free_block(int blockIdx) {
    if (superblock.nfree == FREE_ARRAY_SIZE) {
        free_block copyingBlock;

        copyingBlock.nfree = superblock.nfree;
        for (int i = 0; i < FREE_ARRAY_SIZE; i++) copyingBlock.free[i] = superblock.free[i];

        lseek(fd, BLOCK_SIZE * blockIdx, SEEK_SET);
        write(fd, &copyingBlock, sizeof(copyingBlock));

        superblock.nfree = 0;
        return 1;
    } else {
        superblock.free[superblock.nfree] = blockIdx;
        superblock.nfree++;
        return 1;
    }
}

// Function to ask for a free block, it returns the block index number
int get_free_block() {
    superblock.nfree--;

    if (superblock.nfree > 0) {
        if (superblock.free[superblock.nfree] == 0) {
            printf("!!! Error - System Full !!!\n");
            return -1; // file system is full
        }
    }

    else {
        int freeBlockIdx = superblock.free[0];
        free_block copyingBlock;

        // Read the content of the block with the index is superblock.free[0]
        lseek(fd, BLOCK_SIZE*freeBlockIdx, SEEK_SET);
        
        lseek(fd, BLOCK_SIZE * freeBlockIdx, SEEK_SET);
        read(fd, &copyingBlock, sizeof(copyingBlock));

        superblock.nfree = copyingBlock.nfree;
        for (int i = 0; i < FREE_ARRAY_SIZE; i++) superblock.free[i] = copyingBlock.free[i];    
    }

    return superblock.free[superblock.nfree];
}