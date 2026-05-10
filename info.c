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

    char *target = argv[1];
    uint32_t current_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id != 9999 && temp.parent_id == current_id && strcmp(temp.name, target) == 0) {
            printf("\n" MAGENTA "========= FILE INFO: " YELLOW "%s" MAGENTA " =========" RESET "\n", temp.name);
            printf(" Inode ID    : " CYAN "%u" RESET "\n", temp.inode_id);
            printf(" Type        : " YELLOW "%s" RESET "\n", (temp.type == TYPE_DIR ? "Directory" : (temp.type == TYPE_LINK ? "Symlink" : "Regular File")));
            printf(" Size        : " GREEN "%u" RESET " bytes\n", temp.size);
            printf(" Permissions : " CYAN "%03o" RESET "\n", temp.mode);
            printf(" Owner (UID) : " YELLOW "%u" RESET "\n", temp.uid);
            printf(" Group (GID) : " YELLOW "%u" RESET "\n", temp.gid);
            printf(" Parent ID   : " CYAN "%u" RESET "\n", temp.parent_id);
            
            char time_buf[64];
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&temp.created_at));
            printf(" Created At  : " GREEN "%s" RESET "\n", time_buf);
            
            printf(" Data Blocks : ");
            int has_blocks = 0;
            for(int j=0; j<DIRECT_BLOCKS; j++) {
                if(temp.blocks[j] != 0) {
                    printf(CYAN "%u " RESET, temp.blocks[j]);
                    has_blocks = 1;
                }
            }
            if(!has_blocks) printf(YELLOW "(None)" RESET);
            printf("\n");
            printf(MAGENTA "==========================================" RESET "\n\n");
            
            fclose(disk);
            return 0;
        }
    }

    printf(RED "[ERROR]" RESET " File/Directory '%s' not found!\n", target);
    fclose(disk);
    return 1;
}
