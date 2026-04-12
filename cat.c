#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minifs.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./cat <filename>\n");
        return 1;
    }

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) return 1;

    // 1. Search for the Inode by name
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    int found = 0;

    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && strcmp(temp.name, argv[1]) == 0) {
            
            // 2. Found it! Check if there is data
            if (temp.size == 0) {
                printf("(File is empty)\n");
            } else {
                // 3. Jump to the data block stored in the Inode
                uint32_t block_num = temp.blocks[0];
                fseek(disk, block_num * BLOCK_SIZE, SEEK_SET);

                // 4. Read the exact number of bytes
                char *buffer = malloc(temp.size + 1);
                fread(buffer, temp.size, 1, disk);
                buffer[temp.size] = '\0'; // Null-terminate for printing

                printf("%s\n", buffer);
                free(buffer);
            }
            found = 1;
            break;
        }
    }

    if (!found) printf("File '%s' not found.\n", argv[1]);

    fclose(disk);
    return 0;
}