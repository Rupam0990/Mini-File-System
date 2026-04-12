#include <stdio.h>
#include <stdlib.h>
#include "minifs.h"

int main() {
    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening vdisk");
        return 1;
    }

    // 1. Read Superblock to verify disk
    Superblock sb;
    fread(&sb, sizeof(Superblock), 1, disk);
    if (sb.magic != MAGIC_NUMBER) {
        printf("Invalid file system!\n");
        return 1;
    }

    printf("Listing files in Mini FS:\n");
    printf("--------------------------\n");

    // 2. Jump to Inode Table (Block 1 = 4096 bytes in)
    fseek(disk, BLOCK_SIZE, SEEK_SET);

    Inode current_inode;
    int found_files = 0;

    for (int i = 0; i < MAX_INODES; i++) {
        fread(&current_inode, sizeof(Inode), 1, disk);
        
        // If inode_id is NOT our "empty" marker (9999), it's a file!
        if (current_inode.inode_id != 9999) {
            printf("Name: %-16s | Size: %u bytes | ID: %u\n", 
                    current_inode.name, current_inode.size, current_inode.inode_id);
            found_files++;
        }
    }

    if (found_files == 0) {
        printf("(No files found)\n");
    }

    fclose(disk);
    return 0;
}