# CS4384 Project 2 - Team 15
**V6 Unix File System**\
**Authors: Brennan Grady, Xavier Molyneaux, Thinh Nguyen**

## How to execute:
> gcc mod-v6.c -o mod-v6\
> ./mod-v6

## Project Files:
.\
&nbsp;&nbsp;&nbsp;| `mod-v6.c` main source code file\
&nbsp;&nbsp;&nbsp;| `a.out`, `success.c`, and `test_file.txt` are sample external files for testing

## Sample Commands:
> initfs abc.txt 1024 256
>
> initfs abc.tct 6 3
>
> openfs abc.txt
>
> cpin alo.txt alo\
> cpin test_file.txt test_file\
> cpin success.c success\
> cpin a.out a\
>
> cpout test_file output.txt
> cpout success a.c
>
> rm test_file
>
>q

## Project Logs:
<ins>Part two:</ins> Adding four functions for the V6 File System:
- openfs: open existing File System file
- cpin: clone the context of the external file and save it into the file system place inside of root directory
- cpout: copy the context of a file in the system and store it outside
- rm: delete files in the system

<ins>Part one:</ins>
- Completed building two functions `add_free_block(int)` and `get_free_block()`. `add_free_block(int)` receive block index of an unused block data as a function argument and add it to super block free array. `get_free_block()` returns the free block index or returns -1 if the file system is full
- The program has two commands `initfs` and `q`. `q` is simply to close the program after writing the super block data to the file system.`initfs` receives 3 arguments in order: path to the file system, total number of blocks of the file system, and the number of block devoted for inodes.
For example: 
> initfs abc.txt 1024 256
- The command `initfs` will check if the file system is already created and load existed super block information; if not, create system file, initialize super block and first inode for root directory.
