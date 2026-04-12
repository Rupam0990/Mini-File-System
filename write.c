#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minifs.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./write <filename> <text_content>\n");
        return 1;
    }

    char *filename = argv[1];
    char *content = argv[2];
    int content_len = strlen(content);

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return 1;

    // 1. Find the Inode for this filename
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    long inode_offset = -1;

    for (int i = 0; i < MAX_INODES; i++) {
        inode_offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && strcmp(temp.name, filename) == 0) {
            
            // 2. We found it! Now pick a data block.
            // For simplicity, we'll use block ID 34 (the first data block)
            uint32_t first_data_block = 34; 
            temp.blocks[0] = first_data_block;
            temp.size = content_len;

            // 3. Write the text into the Data Block area
            // Offset = Block ID * Block Size
            fseek(disk, first_data_block * BLOCK_SIZE, SEEK_SET);
            fwrite(content, content_len, 1, disk);

            // 4. Save the updated Inode back to the Inode Table
            fseek(disk, inode_offset, SEEK_SET);
            fwrite(&temp, sizeof(Inode), 1, disk);

            printf("Wrote %d bytes to %s\n", content_len, filename);
            fclose(disk);
            return 0;
        }
    }

    printf("File not found!\n");
    fclose(disk);
    return 0;
}