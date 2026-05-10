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

void list_contents(FILE *disk, uint32_t dir_id, int long_format, int recursive, const char *prefix) {
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    
    // We need to read the whole table for each level of recursion
    // A bit slow but simple for this demo
    for (int i = 0; i < MAX_INODES; i++) {
        fseek(disk, BLOCK_SIZE + (i * sizeof(Inode)), SEEK_SET);
        fread(&temp, sizeof(Inode), 1, disk);
        
        if (temp.inode_id != 9999 && temp.parent_id == dir_id && temp.inode_id != dir_id) {
            if (long_format) {
                char time_buf[64];
                strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&temp.created_at));
                
                char *type_label = (temp.type == TYPE_DIR ? "d" : (temp.type == TYPE_LINK ? "l" : "-"));
                printf("%s%03o  %4u %4u  %6u  %s  %s%s%s\n", 
                    type_label, temp.mode, temp.uid, temp.gid, temp.size, time_buf, 
                    prefix, temp.name, (temp.type == TYPE_DIR ? "/" : ""));
            } else {
                printf(" %s%-15s%s", 
                    (temp.type == TYPE_DIR ? BLUE : (temp.type == TYPE_LINK ? CYAN : RESET)),
                    temp.name, RESET);
                if (!recursive) printf("\n");
            }

            if (recursive && temp.type == TYPE_DIR) {
                if (!long_format) printf("\n%s%s/:\n", prefix, temp.name);
                char next_prefix[256];
                snprintf(next_prefix, sizeof(next_prefix), "%s%s/", prefix, temp.name);
                list_contents(disk, temp.inode_id, long_format, recursive, next_prefix);
            }
        }
    }
    if (!long_format && !recursive) printf("\n");
}

int main(int argc, char *argv[]) {
    int long_format = 0;
    int recursive = 0;
    int opt;

    while ((opt = getopt(argc, argv, "lR")) != -1) {
        switch (opt) {
            case 'l': long_format = 1; break;
            case 'R': recursive = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-R]\n", argv[0]);
                return 1;
        }
    }

    uint32_t current_id = get_cwd_inode();
    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    if (long_format) {
        printf(MAGENTA "PERMS  UID  GID    SIZE  DATE          NAME" RESET "\n");
    } else {
        printf(MAGENTA "=== Listing: " YELLOW "/" MAGENTA " ===" RESET "\n");
    }

    list_contents(disk, current_id, long_format, recursive, "");

    fclose(disk);
    return 0;
}
