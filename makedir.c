#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

uint32_t create_single_dir(FILE *disk, uint32_t parent_id, const char *name) {
    // Check if exists
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == parent_id && strcmp(temp.name, name) == 0) {
            return temp.inode_id;
        }
    }

    // Find free Inode
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id == 9999) {
            Inode new_dir;
            memset(&new_dir, 0, sizeof(Inode));
            new_dir.inode_id = i;
            strncpy(new_dir.name, name, MAX_NAME - 1);
            new_dir.type = TYPE_DIR;
            new_dir.mode = DEFAULT_DIR_MODE;
            new_dir.uid = 1000;
            new_dir.gid = 1000;
            new_dir.parent_id = parent_id;
            new_dir.created_at = time(NULL);

            fseek(disk, offset, SEEK_SET);
            fwrite(&new_dir, sizeof(Inode), 1, disk);

            // Update Superblock
            Superblock sb;
            fseek(disk, 0, SEEK_SET);
            fread(&sb, sizeof(Superblock), 1, disk);
            sb.free_inodes--;
            fseek(disk, 0, SEEK_SET);
            fwrite(&sb, sizeof(Superblock), 1, disk);

            printf(GREEN "[MKDIR]" RESET " Created directory '%s' (Inode %d)\n", name, i);
            return i;
        }
    }
    return 9999;
}

int main(int argc, char *argv[]) {
    int parents = 0;
    int opt;

    while ((opt = getopt(argc, argv, "p")) != -1) {
        switch (opt) {
            case 'p': parents = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-p] <path>\n", argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        return 1;
    }

    char *path_raw = argv[optind];
    uint32_t current_parent = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    if (!parents) {
        if (create_single_dir(disk, current_parent, path_raw) == 9999) {
            fprintf(stderr, "Error creating directory\n");
            fclose(disk);
            return 1;
        }
    } else {
        // Recursive creation for -p
        char *path = strdup(path_raw);
        char *token = strtok(path, "/");
        while (token != NULL) {
            current_parent = create_single_dir(disk, current_parent, token);
            if (current_parent == 9999) {
                fprintf(stderr, "Error in recursive creation\n");
                free(path);
                fclose(disk);
                return 1;
            }
            token = strtok(NULL, "/");
        }
        free(path);
    }

    fclose(disk);
    return 0;
}
