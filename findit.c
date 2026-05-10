#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sumfs.h"

void search_recursive(FILE *disk, uint32_t dir_id, const char *target, const char *current_path) {
    Inode temp;
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    
    // Copy the Inode table to memory to avoid constant fseeks during recursion
    // Or just be careful with file pointers. 
    // For simplicity in a mini-fs, let's just re-read.
    
    for (int i = 0; i < MAX_INODES; i++) {
        fseek(disk, BLOCK_SIZE + (i * sizeof(Inode)), SEEK_SET);
        fread(&temp, sizeof(Inode), 1, disk);
        
        if (temp.inode_id != 9999 && temp.parent_id == dir_id && temp.inode_id != dir_id) {
            char next_path[256];
            snprintf(next_path, sizeof(next_path), "%s/%s", current_path, temp.name);
            
            if (strstr(temp.name, target) != NULL) {
                printf(GREEN "[FOUND]" RESET " %s " CYAN "(Inode %u)" RESET "\n", next_path, temp.inode_id);
            }
            
            if (temp.type == TYPE_DIR) {
                search_recursive(disk, temp.inode_id, target, next_path);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <search_term>\n", argv[0]);
        return 1;
    }

    char *target = argv[1];

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    printf(MAGENTA "Searching for '%s'..." RESET "\n", target);
    search_recursive(disk, 0, target, "");
    printf(MAGENTA "Done." RESET "\n");

    fclose(disk);
    return 0;
}
