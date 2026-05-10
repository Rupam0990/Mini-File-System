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
    if (argc < 4) {
        printf("Usage: %s <uid> <gid> <filename>\n", argv[0]);
        return 1;
    }

    uint16_t new_uid = (uint16_t)atoi(argv[1]);
    uint16_t new_gid = (uint16_t)atoi(argv[2]);
    char *target = argv[3];
    uint32_t current_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, target) == 0) {
            temp.uid = new_uid;
            temp.gid = new_gid;
            fseek(disk, offset, SEEK_SET);
            fwrite(&temp, sizeof(Inode), 1, disk);
            printf(GREEN "[OWNER]" RESET " Ownership for " YELLOW "%s" RESET " updated to " CYAN "%u:%u" RESET "\n", target, new_uid, new_gid);
            fclose(disk);
            return 0;
        }
    }

    printf(RED "[ERROR]" RESET " File/Directory '%s' not found!\n", target);
    fclose(disk);
    return 1;
}
