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

uint32_t calculate_dir_size(FILE *disk, uint32_t dir_id) {
    uint32_t total = 0;
    Inode temp;
    
    // This is a bit inefficient (scans whole table), but works for a mini FS
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == dir_id && temp.inode_id != dir_id) {
            if (temp.type == TYPE_DIR) {
                // We need to save the current position because of recursion
                long current_pos = ftell(disk);
                total += calculate_dir_size(disk, temp.inode_id);
                fseek(disk, current_pos, SEEK_SET);
            } else {
                total += temp.size;
            }
        }
    }
    return total;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename/dirname>\n", argv[0]);
        return 1;
    }

    char *target = argv[1];
    uint32_t current_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, target) == 0) {
            uint32_t size = temp.size;
            if (temp.type == TYPE_DIR) {
                size = calculate_dir_size(disk, temp.inode_id);
            }
            
            printf(GREEN "[SIZE]" RESET " " YELLOW "%s" RESET ": " CYAN "%u" RESET " bytes\n", target, size);
            fclose(disk);
            return 0;
        }
    }

    printf(RED "[ERROR]" RESET " File/Directory '%s' not found!\n", target);
    fclose(disk);
    return 1;
}
