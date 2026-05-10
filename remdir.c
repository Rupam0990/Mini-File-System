#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sumfs.h"

uint32_t get_cwd_inode() {
    FILE *f = fopen(".sumfs_cwd", "r");
    if (!f) return 0;
    uint32_t inode_id;
    fscanf(f, "%u", &inode_id);
    fclose(f);
    return inode_id;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <dirname>\n", argv[0]);
        return 1;
    }

    char *target = argv[1];
    uint32_t current_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    // Find the directory
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    int found = -1;
    long target_offset = 0;
    for (int i = 0; i < MAX_INODES; i++) {
        target_offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, target) == 0) {
            if (temp.type != TYPE_DIR) {
                printf(RED "[ERROR]" RESET " '%s' is not a directory!\n", target);
                fclose(disk);
                return 1;
            }
            if (temp.inode_id == 0) {
                printf(RED "[ERROR]" RESET " Cannot remove root directory!\n");
                fclose(disk);
                return 1;
            }
            found = i;
            break;
        }
    }

    if (found == -1) {
        printf(RED "[ERROR]" RESET " Directory '%s' not found!\n", target);
        fclose(disk);
        return 1;
    }

    // Check if empty
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode child;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&child, sizeof(Inode), 1, disk);
        if (child.inode_id != 9999 && child.parent_id == (uint32_t)found) {
            printf(RED "[ERROR]" RESET " Directory '%s' is not empty!\n", target);
            fclose(disk);
            return 1;
        }
    }

    // Delete
    fseek(disk, target_offset, SEEK_SET);
    Inode free_inode;
    memset(&free_inode, 0, sizeof(Inode));
    free_inode.inode_id = 9999;
    fwrite(&free_inode, sizeof(Inode), 1, disk);

    // Update Superblock
    Superblock sb;
    fseek(disk, 0, SEEK_SET);
    fread(&sb, sizeof(Superblock), 1, disk);
    sb.free_inodes++;
    fseek(disk, 0, SEEK_SET);
    fwrite(&sb, sizeof(Superblock), 1, disk);

    printf(GREEN "[SUCCESS]" RESET " Directory " YELLOW "%s" RESET " removed.\n", target);
    fclose(disk);
    return 0;
}
