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

int main(int argc, char *argv[]) {
    int append = 0;
    int opt;

    while ((opt = getopt(argc, argv, "a")) != -1) {
        switch (opt) {
            case 'a': append = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-a] <filename> <text>\n", argv[0]);
                return 1;
        }
    }

    if (optind + 2 > argc) {
        fprintf(stderr, "Usage: %s [-a] <filename> <text>\n", argv[0]);
        return 1;
    }

    char *filename = argv[optind];
    char *new_content = argv[optind + 1];
    uint32_t current_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        long inode_offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, filename) == 0) {
            if (temp.type != TYPE_FILE) {
                printf(RED "[ERROR]" RESET " '%s' is not a file.\n", filename);
                fclose(disk);
                return 1;
            }

            // Simple allocation logic: using block index based on inode for demo
            if (temp.blocks[0] == 0) {
                temp.blocks[0] = 500 + i; 
            }

            char final_buffer[BLOCK_SIZE];
            memset(final_buffer, 0, BLOCK_SIZE);

            if (append) {
                // Read existing content
                fseek(disk, temp.blocks[0] * BLOCK_SIZE, SEEK_SET);
                fread(final_buffer, 1, temp.size, disk);
                strncat(final_buffer, new_content, BLOCK_SIZE - temp.size - 1);
                temp.size = strlen(final_buffer);
            } else {
                strncpy(final_buffer, new_content, BLOCK_SIZE - 1);
                temp.size = strlen(final_buffer);
            }

            // Write back to disk
            fseek(disk, temp.blocks[0] * BLOCK_SIZE, SEEK_SET);
            fwrite(final_buffer, 1, BLOCK_SIZE, disk);

            // Update Inode
            fseek(disk, inode_offset, SEEK_SET);
            fwrite(&temp, sizeof(Inode), 1, disk);

            printf(MAGENTA "[SAY   ]" RESET " File '%s' updated (%s).\n", filename, (append ? "appended" : "overwritten"));
            fclose(disk);
            return 0;
        }
    }

    printf(RED "[ERROR]" RESET " File '%s' not found.\n", filename);
    fclose(disk);
    return 1;
}
