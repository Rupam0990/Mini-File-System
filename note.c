#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sumfs.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./note <filename>\n");
        return 1;
    }

    printf("--- SumFS Note Editor (%s) ---\n", argv[1]);
    printf("Type your content below. Press Ctrl+D (on a new line) to save.\n");
    printf("--------------------------------------------------\n");

    char buffer[4096];
    size_t len = 0;
    int c;
    while ((c = getchar()) != EOF && len < 4095) {
        buffer[len++] = (char)c;
    }
    buffer[len] = '\0';

    // Now write it using the same logic as 'say'
    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return 1;

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    long inode_offset = -1;
[]
    for (int i = 0; i < MAX_INODES; i++) {
        inode_offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && strcmp(temp.name, argv[1]) == 0) {
            
            if (temp.blocks[0] == 0) {
                temp.blocks[0] = 34 + i; // Slightly better than hardcoded 34
            }
            temp.size = len;

            fseek(disk, temp.blocks[0] * BLOCK_SIZE, SEEK_SET);
            fwrite(buffer, len, 1, disk);

            fseek(disk, inode_offset, SEEK_SET);
            fwrite(&temp, sizeof(Inode), 1, disk);

            printf("\n[SumFS] Note saved to %s (%zu bytes).\n", argv[1], len);
            fclose(disk);
            return 0;
        }
    }

    printf("\n[SumFS] Error: File '%s' not found. Create it with ./tuch first.\n", argv[1]);
    fclose(disk);
    return 1;
}
