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
#include <sys/stat.h>
#include <math.h>

#define BLOCK_SIZE 1024
#define INODE_SIZE 64
#define FREE_ARRAY_SIZE 200
#define BITS sizeof(unsigned short) * 8 // Total bits required to represent unsigned short

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
} inode_type; // 64 bytes

typedef struct { 
    unsigned int inode;
    char filename[28];
} dir_type;  //32 Bytes long

typedef struct {
    unsigned short nfree;
    unsigned short free[FREE_ARRAY_SIZE];
} free_block;

superblock_type superblock;
// int fd = -1;
FILE* fd = NULL;

int open_fs(char *file_name);
int create_and_open_fs(char *file_name);

int open_external(char *file_name);
int read_external(char *file_name, char* buffer, int offset, int buffer_size);
int buffMerger(char *merged_buff, char *to_merge_buff, int merged_buff_size);
char* read_internal(inode_type inode);

void inode_writer(int inum, inode_type inode);
void block_writer(int bidx, void* buffer, int bufferSize);

void superblock_writer();
void superblock_reader();

inode_type inode_reader(int inum, inode_type inode);
void fill_an_inode_and_write(int inum, inode_type inode_object);

int create_root_directory();
void block_reader(int bidx, int offset, void* buffer, int bufferSize);
int directory_ls(dir_type* root_dir, int verbose); // return number of items in root directory

void initfs(int number_diskBlock, int number_diskBlock_for_inodes);
int add_free_block(int blockIdx);
int get_free_block();

void decToBinary(unsigned short n);
int getMSB(unsigned short value);
int add_free_inode(int inodeIdx);
int get_free_inode();

int copyIn(char *extSourcePath, char *outputPath);
int copyOut(char *extSourcePath, char *inputPath);
int removeFile(char *v6_file);

// The main function
int main(int argc, char *argv[]) {
    char *file_name;
    int number_diskBlock = 1024;
    int number_diskBlock_for_inodes = 256;

    system("clear");
    printf("\n... UNIX V6 FILE SYSTEM ...");

    if (fd != NULL) {
        fclose(fd);
        fd = NULL;
    }

    while(1) {
        char input[256];
        char *arg0, *arg1, *arg2;
        
        printf("\n... Usage: ...\n");
        printf(" 1. initfs <file_name> <n1 - file system size in # blocks> <n2 = number of blocks for inodes> (Initialize ile system)\n");
        printf(" 2. openfs <file_name>\n");
        printf(" 3. cpin <externalfile> <v6-file>\n");
        printf(" 4. cpout <v6-file> <externalfile>\n");
        printf(" 5. rm <v6-file>\n");
        printf(" 6. q (Quit the program)\n\n");
        printf("Enter your command: ");
        scanf(" %[^\n]s", input);

        arg0 = strtok(input, " ");
        
        if(strcmp(arg0, "initfs")==0) {                
            file_name = strtok(NULL, " ");
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");

            // if (atoi(arg1) > FREE_ARRAY_SIZE + 3) number_diskBlock = atoi(arg1);
            // if (atoi(arg2) > 2) number_diskBlock_for_inodes = atoi(arg2);
            number_diskBlock = atoi(arg1);
            number_diskBlock_for_inodes = atoi(arg2);

            if (access(file_name, F_OK) != -1) { // file exists
                printf("The file system already exists. Loading existed file system\n");
                open_fs(file_name);
                superblock_reader();
            }
            else {  // file does not exist                
                printf("Creating File System\n");
                inode_type dumping_inode;

                create_and_open_fs(file_name);
                initfs(number_diskBlock, number_diskBlock_for_inodes);
                fill_an_inode_and_write(1, dumping_inode); // create root inode for root
                create_root_directory();    // write data to disk for root
            }

            // 1st inode is root
            inode_type inode1;
            inode1 = inode_reader(1,inode1);
            
            printf("Value of inode1's addr[0] is %d\n", inode1.addr[0]);
            printf("Value of inode1's addr[1] is %d\n", inode1.addr[1]);
        }
        else if (strcmp(arg0, "openfs")==0) {
            file_name = strtok(NULL, " ");
            if (access(file_name, F_OK) != -1) { // file exists
                open_fs(file_name);
                superblock_reader();
                printf("The file system is loaded\n");
            }
            else {
                printf("The file system is not existed\n");
            }
        }
        else if (strcmp(arg0, "cpin")==0) {
            char * extSourcePath = strtok(NULL, " ");
            char * outputPath   = strtok(NULL, " ");

            if (access(extSourcePath, F_OK) != -1) { // file exists
                printf("The external file is loaded\n");
                copyIn(extSourcePath, outputPath);
            }
            else {
                printf("The external file is not existed\n");
            }
        }        
        else if (strcmp(arg0, "cpout")==0) {
            char * inputPath = strtok(NULL, " ");
            char * extSourcePath   = strtok(NULL, " ");

            int result = copyOut(extSourcePath, inputPath);
            if (result == -1) {
                printf("The internal file, %s, is not existed\n", inputPath);
            }
        }
        else if (strcmp(arg0, "rm")==0) {
            char * v6_file = strtok(NULL, " ");
            int result = removeFile(v6_file);
            if (result == -1) {
                printf("The internal file, %s, is not existed\n", v6_file);
            }
            else {
                printf("The internal file(s), %s, has been deleted\n", v6_file);
            }
        }
        else if (strcmp(arg0, "q")==0) {
            if (fd != NULL) {
                superblock_writer(); // save superblock to the disk before closing the program
                fclose(fd);
            }
            exit(0);
        }
    }
}

int open_fs(char *file_name) {
    fd = fopen (file_name, "r+");

    if (fd == NULL) {
        return -1;
    }
    else {
        return 1;
    }
}


int create_and_open_fs(char *file_name) {
    fd = fopen (file_name, "w+");

    if (fd == NULL) {
        return -1;
    }
    else {
        return 1;
    }
}

int open_external(char *file_name) {
    FILE* external_fd;
    external_fd = fopen(file_name, "r");

    if (external_fd == NULL) {
        fclose(external_fd);
        return -1;
    }
    else {
        int file_size = 0;
        struct stat sb;
        stat(file_name, &sb);
        file_size = sb.st_size;

        fclose(external_fd);
        return file_size;
    }
}

int read_external(char *file_name, char *buffer, int offset, int buffer_size) {
    FILE* external_fd;
    external_fd = fopen(file_name, "r");

    if (external_fd == NULL) {
        fclose(external_fd);
        return -1;
    }
    else {
        fseek(external_fd, offset, SEEK_SET);
        fread(buffer, buffer_size, 1, external_fd);

        fclose(external_fd);

        return 1;
    }
}

int buffMerger(char *merged_buff, char *to_merge_buff, int merged_buff_size) {
    //printf("21321323\n");
    strcat(merged_buff, to_merge_buff);

    return 1; 
}

char* read_internal(inode_type file_inode) {
    int buff_size = file_inode.size1;

    char *emty = "";
    char *mergedBuffer = malloc(buff_size);
    strcpy(mergedBuffer, emty);

    int number_blocks = (int) ((file_inode.size1 / BLOCK_SIZE) + 1);

    for (int i = 0; i < number_blocks; i++) {
        int reading_buff_size = 0;
        if (i == number_blocks - 1) {
            reading_buff_size = file_inode.size1 % BLOCK_SIZE;
        } else {
            reading_buff_size = BLOCK_SIZE;
        }

        char reading_buff[reading_buff_size];

        block_reader(file_inode.addr[i], 0, reading_buff, reading_buff_size);
        buffMerger(mergedBuffer, reading_buff, reading_buff_size);
    }

    return mergedBuffer;
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

    // initialize all disk blocks, last block is dumping block
    char last_byte = '0';
    fseek(fd, BLOCK_SIZE * superblock.fsize + 1023, SEEK_SET); 
    fwrite(&last_byte , 1, 1, fd);

    // write superblock
    superblock_writer();
    
    // genetate all free inodes, except the first one for root
    // int max_number_of_inode = (int)(superblock.isize * BLOCK_SIZE / INODE_SIZE);
    // for (int inode_idx = 2; inode_idx < max_number_of_inode; inode_idx++) {
    //     add_free_inode(inode_idx);
    // }

    // generate all free blocks, except the first one for root
    for (int blockIdx = 2 + superblock.isize + 1; blockIdx < number_diskBlock; blockIdx++) {
        add_free_block(blockIdx);
    }
}

void block_writer(int bidx, void* buffer, int bufferSize) {
    fseek(fd, BLOCK_SIZE * bidx, SEEK_SET); 
    fwrite(buffer , 1, bufferSize, fd);
}

void block_reader(int bidx, int offset, void* buffer, int bufferSize)
{
    fseek(fd,(BLOCK_SIZE * bidx) + offset, SEEK_SET);
    fread(buffer, bufferSize, 1, fd);
}

// Function to write inode
void inode_writer(int inum, inode_type inode) {
    fseek(fd, 2 * BLOCK_SIZE + (inum-1) * INODE_SIZE , SEEK_SET); 
    fwrite(&inode , 1, sizeof(inode), fd);
}

// Function to write super block
void superblock_writer() {
    fseek(fd, BLOCK_SIZE, SEEK_SET); // superblock is in the 2nd block
    fwrite(&superblock, 1, sizeof(superblock), fd);
}

// Function to read super block
void superblock_reader() {
    fseek(fd, BLOCK_SIZE, SEEK_SET);
    fread(&superblock, sizeof(superblock), 1, fd);
}

// Function to read inodes
inode_type inode_reader(int inum, inode_type inode) {
    fseek(fd, 2 * BLOCK_SIZE + (inum-1) * INODE_SIZE, SEEK_SET); 
    fread(&inode, sizeof(inode), 1, fd);  
    
    return inode;
}

// Function to write inode number after filling some fileds
void fill_an_inode_and_write(int inum, inode_type inode_object) {
    // total # inodes = number_diskBlock_for_inodes * BLOCK_SIZE / INODE_SIZE
    if (inum >= superblock.isize * BLOCK_SIZE / INODE_SIZE) return;
    
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
        root.addr[0] = superblock.isize + 2;
        
        for (int i=1;i<9;i++) 
            root.addr[i]=-1; // all other addr elements are null so setto -1
        
        inode_writer(inum, root);
    } else {
        inode_writer(inum, inode_object);
    }
}

int create_root_directory() {
    inode_type inode1;
    inode1 = inode_reader(1,inode1);
    
    dir_type root_dir[2];

    root_dir[0].inode = 1;
    strcpy(root_dir[0].filename, ".");
    root_dir[1].inode = 1;
    strcpy(root_dir[1].filename, "..");

    block_writer(inode1.addr[0], root_dir, 2 * sizeof(dir_type));

    return 1;
}


int directory_ls(dir_type* root_dir, int verbose) {
    inode_type inode1;
    inode1 = inode_reader(1,inode1);    
    
    fseek(fd, inode1.addr[0] * BLOCK_SIZE, SEEK_SET);
    fread(root_dir, inode1.size1, 1, fd);

    int number_item = (int)(inode1.size1 / sizeof(dir_type));
    if (verbose == 1) {
        for (int i = 0; i < number_item; i++) {
            printf("%d %s\n", root_dir[i].inode, root_dir[i].filename);
        }
    }

    return number_item;
}

// Function to release an unused block
int add_free_block(int blockIdx) {
    if (superblock.nfree == FREE_ARRAY_SIZE) {
        free_block copyingBlock;

        copyingBlock.nfree = superblock.nfree;
        for (int i = 0; i < FREE_ARRAY_SIZE; i++) copyingBlock.free[i] = superblock.free[i];

        fseek(fd, BLOCK_SIZE * blockIdx, SEEK_SET);
        fwrite(&copyingBlock, 1, sizeof(copyingBlock), fd);

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
        fseek(fd, BLOCK_SIZE * freeBlockIdx, SEEK_SET);
        fread(&copyingBlock, sizeof(copyingBlock), 1, fd);

        superblock.nfree = copyingBlock.nfree;
        for (int i = 0; i < FREE_ARRAY_SIZE; i++) superblock.free[i] = copyingBlock.free[i];    
    }

    return superblock.free[superblock.nfree];
}

int add_free_inode(int inode_idx) {
    // inode_type free_inode;

    // free_inode.flags |= 0 << 15;
    // inode_writer(inode_idx, free_inode);

    // return 1;
    return 1;
}

void decToBinary(unsigned short n)
{
    // array to store binary number
    unsigned short binaryNum[16];
 
    // counter for binary array
    int i = 0;
    while (n > 0) {
 
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }
 
    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--)
        printf("%d", binaryNum[j]);

    printf("\n");
}

int getMSB(unsigned short value)
{
    return (value & 0xFF00);
}

int get_free_inode() {
    int inode_idx = 2; // inode 1 is devoted for root
    inode_type free_inode;
    int max_number_of_inode = (int)(superblock.isize * BLOCK_SIZE / INODE_SIZE);

    for (;inode_idx < max_number_of_inode; inode_idx++) {
        free_inode = inode_reader(inode_idx, free_inode);
        int msb = getMSB(free_inode.flags);

        if (msb == 0) {
            return inode_idx;
        }
    }

    return -1; // run out of free inode
}


// Function to copy file to v6-file system
int copyIn(char *extSourcePath, char *outputPath) {
    int extSource_size = open_external(extSourcePath);
    
    if  (extSource_size <= 0) {
        printf("!!! Error - File does not exist !!!\n");
        return -1;
    }

    if (extSource_size <= 9 * BLOCK_SIZE) { // small size
        // initialize an inode
        inode_type outputInode;
        int inode_num = get_free_inode();
        int current_addr_idx = 0;
 
        int number_needed_blocks = (int) ((extSource_size / BLOCK_SIZE) + 1);

        outputInode.flags |= 1 << 15; //Root is allocated
        outputInode.flags |= 0 << 14; //It is a file
        outputInode.actime = time(NULL);
        outputInode.modtime = time(NULL);

        outputInode.size0 = 0;
        outputInode.size1 = extSource_size;
        
        for (current_addr_idx = 0; current_addr_idx < number_needed_blocks; current_addr_idx++) {
            outputInode.addr[current_addr_idx] = get_free_block();
        }
        
        for (; current_addr_idx < 9; current_addr_idx++) {
            outputInode.addr[current_addr_idx] = -1; // unused
        }

        inode_writer(inode_num,outputInode);

        // create an entry in root directiory
        dir_type file_entry;
        file_entry.inode = inode_num;
        strcpy(file_entry.filename, outputPath);

        dir_type root_file_list[100];
        int number_entry = directory_ls(root_file_list,0);
        root_file_list[number_entry] = file_entry;

        // update inode 1
        inode_type inode1;
        inode1 = inode_reader(1,inode1);
        inode1.size1 = inode1.size1 + sizeof(dir_type);    
        inode1.modtime = time(NULL);

        inode_writer(1, inode1);
        
        // update root diretory
        inode1 = inode_reader(1, inode1);
        block_writer(inode1.addr[0], root_file_list, inode1.size1);

        dir_type checking_root_file_list[100]; 
        directory_ls(checking_root_file_list,1);

        // Write file content to disk
        inode_type file_inode;
        file_inode = inode_reader(inode_num, file_inode);
        
        // decToBinary (file_inode.flags);
        // printf("%d\n", inode_num);
        // printf("%d\n", file_inode.addr[0]);
        // printf("%d\n", inode1.size1);
        FILE * external_fd;

        int number_block_to_write =  (int) ((file_inode.size1 / BLOCK_SIZE) + 1);

        for (int i = 0; i < number_block_to_write; i++) {
            if (i == number_block_to_write - 1) {   // last block to write   
                int byte_size = file_inode.size1 % BLOCK_SIZE;
                char buffer[BLOCK_SIZE];
                read_external(extSourcePath, buffer, BLOCK_SIZE * i, byte_size);
                block_writer(file_inode.addr[i] + BLOCK_SIZE * i, buffer, byte_size);
            }
            else {
                char buffer[BLOCK_SIZE];
                read_external(extSourcePath, buffer, BLOCK_SIZE * i, BLOCK_SIZE);
                block_writer(file_inode.addr[i] + BLOCK_SIZE * i, buffer, BLOCK_SIZE);
            }
        }
        
        char * data;
        data = read_internal(file_inode);
        printf("\nData read from v6 file system:\n%s\n", data);

        return 1;
        
    }
    else {
        return -1;  // Does not support medium and large file at this moment
    }
}

int copyOut(char *extSourcePath, char *inputPath) {
    // Searching for the input file in the file system
    inode_type inode1;
    inode1 = inode_reader(1,inode1);
    
    int number_of_entry = (int)(inode1.size1 / sizeof(dir_type));
    dir_type root_file_list[100]; 
    int inode_num = -1;
    
    directory_ls(root_file_list,1);
    for (int i = 0; i < number_of_entry; i++) {
        char *file_name = malloc(28); // file name is 28-byte long
        strcpy(file_name, root_file_list[i].filename);

        int result = strcmp(file_name, inputPath);
        if (result == 0) {
            inode_num = root_file_list[i].inode;
            break;
        }
    }

    if (inode_num <= 0) {
        return -1; // file does not exist in the file system
    }

    inode_type file_inode;
    file_inode = inode_reader(inode_num, file_inode);

    char * data;
    data = read_internal(file_inode);
    printf("\nData read from v6 file system:\n%s\n", data);
    
    // generate a file
    FILE * external_fd = fopen (extSourcePath, "w+");

    if (external_fd == NULL) {
        return -1;
    }

    fwrite(data , 1 , file_inode.size1 , external_fd);
    fclose(external_fd);

    return 1;
}

int removeFile(char *v6_file) {
    // Searching for the file to remove in the file system
    inode_type inode1;
    inode1 = inode_reader(1,inode1);
    
    int list_inode_file_to_remove[100];
    int size_list_inode_file_to_remove = 0;
    
    dir_type list_file_to_keep[100];
    int size_list_file_to_keep = 0;
    
    int number_of_entry = (int)(inode1.size1 / sizeof(dir_type));
    dir_type root_file_list[100]; 
    int inode_num = -1;
    
    directory_ls(root_file_list,0);
    for (int i = 0; i < number_of_entry; i++) {
        char *file_name = malloc(28); // file name is 28-byte long
        strcpy(file_name, root_file_list[i].filename);

        int result = strcmp(file_name, v6_file);
        if (result == 0) {
            inode_num = root_file_list[i].inode;
            list_inode_file_to_remove[size_list_inode_file_to_remove] = inode_num;
            size_list_inode_file_to_remove++;
        }
        else {
            list_file_to_keep[size_list_file_to_keep] = root_file_list[i];
            size_list_file_to_keep++;
        }
    }

    if (inode_num <= 0) {
        return -1; // file does not exist in the file system
    }

    // update root directory, remove files to be deleted
        // update root inode
    inode1.size1 = size_list_file_to_keep * sizeof(dir_type);
    inode1.modtime = time(NULL);
    inode_writer(1, inode1);

        // update root directory
    inode1 = inode_reader(1,inode1);
    block_writer(inode1.addr[0], list_file_to_keep, inode1.size1);
    
    // printf("%d\n", inode1.size1);
    directory_ls(list_file_to_keep,1);

    // free data blocks and unallocate inode
    for (int i = 0; i < size_list_inode_file_to_remove; i++) {
        int inode_num = list_inode_file_to_remove[i];
        
        inode_type file_inode;
        file_inode = inode_reader(inode_num, file_inode);

        // free data block
        int block_num = file_inode.addr[0];
        for (int i = 0; i <  9; i++) {
            if (block_num > 0) {
                add_free_block(block_num);
            }
        }

        // unallocate inode
        file_inode.flags = 0;
        inode_writer(inode_num, file_inode);
    }

    return 1;
}
