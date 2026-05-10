#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sumfs.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./coppy <source> <destination>\n");
        return 1;
    }

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return 1;

    Inode src_inode, dest_inode;
    int src_found = 0, dest_index = -1;
    long dest_offset = -1;

    // 1. Find source and a free slot for destination
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        Inode temp;
        fread(&temp, sizeof(Inode), 1, disk);
        
        if (temp.inode_id != 9999 && strcmp(temp.name, argv[1]) == 0) {
            src_inode = temp;
            src_found = 1;
        }
        if (temp.inode_id == 9999 && dest_index == -1) {
            dest_index = i;
            dest_offset = offset;
        }
    }

    if (!src_found) {
        printf("[SumFS] Source file '%s' not found.\n", argv[1]);
        fclose(disk);
        return 1;
    }
    if (dest_index == -1) {
        printf("[SumFS] No space for destination file.\n");
        fclose(disk);
        return 1;
    }

    // 2. Read source data
    char *buffer = malloc(src_inode.size);
    fseek(disk, src_inode.blocks[0] * BLOCK_SIZE, SEEK_SET);
    fread(buffer, src_inode.size, 1, disk);

    // 3. Prepare destination inode
    memset(&dest_inode, 0, sizeof(Inode));
    dest_inode.inode_id = dest_index;
    strncpy(dest_inode.name, argv[2], MAX_NAME - 1);
    dest_inode.size = src_inode.size;
    dest_inode.created_at = time(NULL);
    dest_inode.blocks[0] = 34 + dest_index; // Basic block assignment

    // 4. Write destination data
    fseek(disk, dest_inode.blocks[0] * BLOCK_SIZE, SEEK_SET);
    fwrite(buffer, dest_inode.size, 1, disk);

    // 5. Write destination inode
    fseek(disk, dest_offset, SEEK_SET);
    fwrite(&dest_inode, sizeof(Inode), 1, disk);

    // 6. Update Superblock
    Superblock sb;
    fseek(disk, 0, SEEK_SET);
    fread(&sb, sizeof(Superblock), 1, disk);
    sb.free_inodes--;
    fseek(disk, 0, SEEK_SET);
    fwrite(&sb, sizeof(Superblock), 1, disk);

    printf("[SumFS] Copied %s to %s\n", argv[1], argv[2]);
    free(buffer);
    fclose(disk);
    return 0;
}
