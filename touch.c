#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "minifs.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./touch <filename>\n");
        return 1;
    }

    FILE *disk = fopen("vdisk", "rb+"); // Open for Reading and Writing
    if (!disk) {
        perror("Error opening vdisk");
        return 1;
    }

    // 1. Read Superblock to see if we have space
    Superblock sb;
    fread(&sb, sizeof(Superblock), 1, disk);
    if (sb.free_inodes == 0) {
        printf("Error: No free inodes left!\n");
        fclose(disk);
        return 1;
    }

    // 2. Scan for a free Inode (ID 9999)
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp_inode;
    int found_index = -1;

    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&temp_inode, sizeof(Inode), 1, disk);
        if (temp_inode.inode_id == 9999) {
            found_index = i;
            
            // 3. Prepare the new Inode data
            Inode new_file;
            memset(&new_file, 0, sizeof(Inode));
            new_file.inode_id = i;
            strncpy(new_file.name, argv[1], MAX_NAME - 1);
            new_file.size = 0; // Empty file
            new_file.is_directory = 0;
            new_file.created_at = time(NULL);

            // 4. Write it back to the exact same spot
            fseek(disk, offset, SEEK_SET);
            fwrite(&new_file, sizeof(Inode), 1, disk);
            break;
        }
    }

    // 5. Update Superblock
    sb.free_inodes--;
    fseek(disk, 0, SEEK_SET);
    fwrite(&sb, sizeof(Superblock), 1, disk);

    printf("File '%s' created successfully.\n", argv[1]);
    fclose(disk);
    return 0;
}