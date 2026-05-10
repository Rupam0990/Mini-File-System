#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    uint32_t parent_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    Superblock sb;
    fread(&sb, sizeof(Superblock), 1, disk);

    // Check if exists in current dir
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == parent_id && strcmp(temp.name, filename) == 0) {
            printf(RED "[ERROR]" RESET " File/Directory '%s' already exists!\n", filename);
            fclose(disk);
            return 1;
        }
    }

    // Find free inode
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id == 9999) {
            Inode new_file;
            memset(&new_file, 0, sizeof(Inode));
            new_file.inode_id = i;
            strncpy(new_file.name, filename, MAX_NAME - 1);
            new_file.size = 0;
            new_file.type = TYPE_FILE;
            new_file.mode = DEFAULT_FILE_MODE;
            new_file.uid = 1000;
            new_file.gid = 1000;
            new_file.parent_id = parent_id;
            new_file.created_at = time(NULL);

            fseek(disk, offset, SEEK_SET);
            fwrite(&new_file, sizeof(Inode), 1, disk);

            sb.free_inodes--;
            fseek(disk, 0, SEEK_SET);
            fwrite(&sb, sizeof(Superblock), 1, disk);

            printf(GREEN "[SUCCESS]" RESET " File " YELLOW "%s" RESET " created at Inode " CYAN "%d" RESET "\n", filename, i);
            fclose(disk);
            return 0;
        }
    }

    fclose(disk);
}
