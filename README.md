# CS4384 Project 2 - Team 15
**V6 Unix File System**\
**Authors: Brennan Grady, Xavier Molyneaux, Thinh Nguyen**

## How to execute:
> gcc mod-v6.c -o mod-v6\
> ./mod-v6

## Project Logs:

Part one:
- Completed building two functions `add_free_block(int)` and `get_free_block()`. `add_free_block(int)` receive block index of an unused block data as a function argument and add it to super block free array. `get_free_block()` returns the free block index or returns -1 if the file system is full
- The program has two commands `initfs` and `q`. `q` is simply to close the program after writing the super block data to the file system.`initfs` receives 3 arguments in order: path to the file system, total number of blocks of the file system, and the number of block devoted for inodes.
For example: 
> initfs abc.txt 1024 256
- The command `initfs` will check if the file system is already created and load existed super block information; if not, create system file, initialize super block and first inode for root directory.
