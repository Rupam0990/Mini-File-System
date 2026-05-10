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

void set_cwd_inode(uint32_t inode_id) {
    FILE *f = fopen(".sumfs_cwd", "w");
    if (f) {
        fprintf(f, "%u", inode_id);
        fclose(f);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <dirname>\n", argv[0]);
        return 1;
    }

    char *target = argv[1];
    uint32_t current_id = get_cwd_inode();

    if (strcmp(target, "/") == 0) {
        set_cwd_inode(0);
        printf(GREEN "[HOP]" RESET " Moved to " YELLOW "/" RESET "\n");
        return 0;
    }

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    // Load current directory to handle ".."
    fseek(disk, INODE_TABLE_START * BLOCK_SIZE + (current_id * sizeof(Inode)), SEEK_SET);
    Inode current_dir;
    fread(&current_dir, sizeof(Inode), 1, disk);

    if (strcmp(target, "..") == 0) {
        set_cwd_inode(current_dir.parent_id);
        
        // Find parent name for display
        fseek(disk, INODE_TABLE_START * BLOCK_SIZE + (current_dir.parent_id * sizeof(Inode)), SEEK_SET);
        Inode parent;
        fread(&parent, sizeof(Inode), 1, disk);
        
        printf(GREEN "[HOP]" RESET " Moved to " YELLOW "%s" RESET "\n", parent.name);
        fclose(disk);
        return 0;
    }

    // Search for target in current directory
    fseek(disk, INODE_TABLE_START * BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, target) == 0) {
            if (temp.type != TYPE_DIR) {
                printf(RED "[ERROR]" RESET " '%s' is not a directory!\n", target);
                fclose(disk);
                return 1;
            }
            set_cwd_inode(temp.inode_id);
            printf(GREEN "[HOP]" RESET " Moved to " YELLOW "%s" RESET "\n", target);
            fclose(disk);
            return 0;
        }
    }

    printf(RED "[ERROR]" RESET " Directory '%s' not found!\n", target);
    fclose(disk);
    return 1;
}
