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
        printf("Usage: ./readit <filename>\n");
        return 1;
    }

    uint32_t current_id = get_cwd_inode();
    FILE *disk = fopen("vdisk", "rb");
    if (!disk) return 1;

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    int found = 0;

    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, argv[1]) == 0) {
            
            printf(BLUE "[READ  ]" RESET " Accessing Inode " CYAN "%d" RESET "\n", i);
            if (temp.size == 0) {
                printf(YELLOW "(File is empty)" RESET "\n");
            } else {
                uint32_t block_num = temp.blocks[0];
                printf(BLUE "[BLOCK ]" RESET " Reading block " CYAN "%u" RESET "\n", block_num);
                
                fseek(disk, block_num * BLOCK_SIZE, SEEK_SET);

                char *buffer = malloc(temp.size + 1);
                fread(buffer, temp.size, 1, disk);
                buffer[temp.size] = '\0';

                printf("\n" GREEN "Content:" RESET "\n%s\n", buffer);
                free(buffer);
            }
            found = 1;
            break;
        }
    }

    if (!found) printf(RED "[ERROR]" RESET " File '%s' not found.\n", argv[1]);

    fclose(disk);
    return 0;
}
