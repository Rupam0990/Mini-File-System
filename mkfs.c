#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minifs.h"

int main() {
    FILE *dest = fopen("vdisk", "wb");
    if (!dest) {
        perror("Error creating disk");
        return 1;
    }

    // 1. Prepare the Superblock
    Superblock sb;
    sb.magic = MAGIC_NUMBER;
    sb.total_blocks = (50 * 1024 * 1024) / BLOCK_SIZE;
    sb.inode_count = MAX_INODES;
    sb.free_blocks = sb.total_blocks - 33; // 1 (SB) + 32 (Inodes) = 33 blocks used
    sb.free_inodes = MAX_INODES;

    // 2. Stretch the file to 50MB
    fseek(dest, (50 * 1024 * 1024) - 1, SEEK_SET);
    fputc('\0', dest);

    // 3. Write Superblock at the very beginning (Block 0)
    fseek(dest, 0, SEEK_SET);
    fwrite(&sb, sizeof(Superblock), 1, dest);

    // 4. Initialize the Inode Table (Blocks 1 to 32)
    // We create one "empty" inode and write it 1024 times
    Inode empty_inode;
    memset(&empty_inode, 0, sizeof(Inode));
    empty_inode.inode_id = 9999; // Using 9999 to mean "Empty/Unused"
    
    // Move to the start of Block 1 (4096 bytes in)
    fseek(dest, BLOCK_SIZE, SEEK_SET); 
    for(int i = 0; i < MAX_INODES; i++) {
        fwrite(&empty_inode, sizeof(Inode), 1, dest);
    }

    fclose(dest);
    printf("Disk formatted! Superblock and 1024 Inodes initialized.\n");
    return 0;
}