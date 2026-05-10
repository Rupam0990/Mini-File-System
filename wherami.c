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

void resolve_path(FILE *disk, uint32_t inode_id, char *path) {
    if (inode_id == 0) {
        return;
    }

    fseek(disk, INODE_TABLE_START * BLOCK_SIZE + (inode_id * sizeof(Inode)), SEEK_SET);
    Inode temp;
    fread(&temp, sizeof(Inode), 1, disk);

    // Recursively get parent path first
    resolve_path(disk, temp.parent_id, path);
    
    strcat(path, "/");
    strcat(path, temp.name);
}

int main() {
    uint32_t current_id = get_cwd_inode();
    
    if (current_id == 0) {
        printf(GREEN "Current Path: " YELLOW "/" RESET "\n");
        return 0;
    }

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    char path[512] = "";
    resolve_path(disk, current_id, path);
    fclose(disk);

    printf(GREEN "Current Path: " YELLOW "%s" RESET "\n", path);
    return 0;
}
