#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sumfs.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./moveit <source> <destination>\n");
        return 1;
    }

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return 1;

    Inode temp;
    int found = 0;
    long offset = -1;

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < MAX_INODES; i++) {
        offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && strcmp(temp.name, argv[1]) == 0) {
            strncpy(temp.name, argv[2], MAX_NAME - 1);
            fseek(disk, offset, SEEK_SET);
            fwrite(&temp, sizeof(Inode), 1, disk);
            found = 1;
            break;
        }
    }

    if (found) {
        printf("[SumFS] Moved/Renamed %s to %s\n", argv[1], argv[2]);
    } else {
        printf("[SumFS] File '%s' not found.\n", argv[1]);
    }

    fclose(disk);
    return 0;
}
