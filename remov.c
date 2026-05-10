#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sumfs.h"

uint32_t get_cwd_inode() {
    FILE *f = fopen(".sumfs_cwd", "r");
    if (!f) return 0;
    uint32_t inode_id;
    fscanf(f, "%u", &inode_id);
    fclose(f);
    return inode_id;
}

void delete_recursive(FILE *disk, uint32_t inode_id) {
    Inode temp;
    
    // If it's a directory, find and delete all children first
    fseek(disk, BLOCK_SIZE + (inode_id * sizeof(Inode)), SEEK_SET);
    fread(&temp, sizeof(Inode), 1, disk);
    
    if (temp.type == TYPE_DIR) {
        for (int i = 0; i < MAX_INODES; i++) {
            Inode child;
            fseek(disk, BLOCK_SIZE + (i * sizeof(Inode)), SEEK_SET);
            fread(&child, sizeof(Inode), 1, disk);
            
            if (child.inode_id != 9999 && child.parent_id == inode_id && child.inode_id != inode_id) {
                delete_recursive(disk, child.inode_id);
            }
        }
    }

    // Mark current Inode as free
    memset(&temp, 0, sizeof(Inode));
    temp.inode_id = 9999;
    fseek(disk, BLOCK_SIZE + (inode_id * sizeof(Inode)), SEEK_SET);
    fwrite(&temp, sizeof(Inode), 1, disk);

    // Update Superblock counts
    Superblock sb;
    fseek(disk, 0, SEEK_SET);
    fread(&sb, sizeof(Superblock), 1, disk);
    sb.free_inodes++;
    fseek(disk, 0, SEEK_SET);
    fwrite(&sb, sizeof(Superblock), 1, disk);
}

int main(int argc, char *argv[]) {
    int recursive = 0;
    int opt;

    while ((opt = getopt(argc, argv, "r")) != -1) {
        switch (opt) {
            case 'r': recursive = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-r] <name>\n", argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        return 1;
    }

    char *target = argv[optind];
    uint32_t current_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    // Find target in current directory
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, target) == 0) {
            
            if (temp.type == TYPE_DIR && !recursive) {
                printf(RED "[ERROR]" RESET " '%s' is a directory. Use -r to remove recursively.\n", target);
                fclose(disk);
                return 1;
            }

            if (recursive) {
                printf(MAGENTA "[NUKE ]" RESET " Deleting everything inside '%s'...\n", target);
                delete_recursive(disk, temp.inode_id);
            } else {
                // Single file delete
                temp.inode_id = 9999;
                fseek(disk, offset, SEEK_SET);
                fwrite(&temp, sizeof(Inode), 1, disk);

                Superblock sb;
                fseek(disk, 0, SEEK_SET);
                fread(&sb, sizeof(Superblock), 1, disk);
                sb.free_inodes++;
                fseek(disk, 0, SEEK_SET);
                fwrite(&sb, sizeof(Superblock), 1, disk);
            }

            printf(RED "[DELETE]" RESET " SUCCESS: '%s' removed.\n", target);
            fclose(disk);
            return 0;
        }
    }

    printf(RED "[ERROR]" RESET " '%s' not found.\n", target);
    fclose(disk);
    return 1;
}
