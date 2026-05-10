#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sumfs.h"

int main() {
    printf(CYAN "[INIT]" RESET " Starting SumFS disk formatting...\n");
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
    sb.free_blocks = sb.total_blocks - 33; 
    sb.free_inodes = MAX_INODES;

    printf(CYAN "[INIT]" RESET " Total capacity: " YELLOW "50 MB" RESET "\n");
    printf(CYAN "[INIT]" RESET " Total Inodes: " YELLOW "%u" RESET "\n", MAX_INODES);

    // 2. Stretch the file
    fseek(dest, (50 * 1024 * 1024) - 1, SEEK_SET);
    fputc('\0', dest);

    // 3. Write Superblock
    fseek(dest, 0, SEEK_SET);
    fwrite(&sb, sizeof(Superblock), 1, dest);
    printf(CYAN "[INIT]" RESET " Superblock initialized at block 0\n");

    // 4. Initialize Inode Table
    Inode empty_inode;
    memset(&empty_inode, 0, sizeof(Inode));
    empty_inode.inode_id = 9999;
    
    fseek(dest, BLOCK_SIZE, SEEK_SET); 
    for(int i = 0; i < MAX_INODES; i++) {
        if (i == 0) {
            // Initialize Root Directory
            Inode root;
            memset(&root, 0, sizeof(Inode));
            root.inode_id = 0;
            strcpy(root.name, "/");
            root.type = TYPE_DIR;
            root.mode = DEFAULT_DIR_MODE;
            root.uid = 1000;
            root.gid = 1000;
            root.parent_id = 0; // Root is its own parent
            root.created_at = time(NULL);
            fwrite(&root, sizeof(Inode), 1, dest);
            sb.free_inodes--;
        } else {
            fwrite(&empty_inode, sizeof(Inode), 1, dest);
        }
    }
    
    // Update Superblock with final free counts
    fseek(dest, 0, SEEK_SET);
    fwrite(&sb, sizeof(Superblock), 1, dest);

    printf(CYAN "[INIT]" RESET " Inode Table initialized (Blocks 1-32)\n");
    printf(CYAN "[INIT]" RESET " Root directory " YELLOW "/" RESET " created at Inode 0\n");

    fclose(dest);
    printf("\n" GREEN "SUCCESS: SumFS disk is ready!" RESET "\n");
    return 0;
}
