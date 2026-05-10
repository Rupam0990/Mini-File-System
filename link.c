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
    if (argc < 3) {
        printf("Usage: %s <target_path> <link_name>\n", argv[0]);
        return 1;
    }

    char *target_path = argv[1];
    char *link_name = argv[2];
    uint32_t current_id = get_cwd_inode();

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    Superblock sb;
    fread(&sb, sizeof(Superblock), 1, disk);

    if (sb.free_inodes == 0 || sb.free_blocks == 0) {
        printf(RED "[ERROR]" RESET " Not enough space for link!\n");
        fclose(disk);
        return 1;
    }

    // Find free Inode
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id == 9999) {
            // Find free Block to store the target path
            // For simplicity, we'll just pick a block (in a real FS we'd have a bitmap)
            // Using the same simple logic as sumfs_fuse for now
            uint32_t link_block = 50; // Placeholder, real logic would find a free block
            // Better: search for used blocks. But for demo, let's just use 100+
            link_block = 100 + i; 

            Inode new_link;
            memset(&new_link, 0, sizeof(Inode));
            new_link.inode_id = i;
            strncpy(new_link.name, link_name, MAX_NAME - 1);
            new_link.type = TYPE_LINK;
            new_link.mode = 0777;
            new_link.parent_id = current_id;
            new_link.size = strlen(target_path);
            new_link.blocks[0] = link_block;
            new_link.created_at = time(NULL);

            // Write target path to the block
            fseek(disk, link_block * BLOCK_SIZE, SEEK_SET);
            fwrite(target_path, 1, strlen(target_path), disk);

            // Write Inode
            fseek(disk, offset, SEEK_SET);
            fwrite(&new_link, sizeof(Inode), 1, disk);

            // Update Superblock
            sb.free_inodes--;
            sb.free_blocks--;
            fseek(disk, 0, SEEK_SET);
            fwrite(&sb, sizeof(Superblock), 1, disk);

            printf(GREEN "[SUCCESS]" RESET " Link " YELLOW "%s" RESET " -> " CYAN "%s" RESET " created.\n", link_name, target_path);
            fclose(disk);
            return 0;
        }
    }

    fclose(disk);
    return 1;
}
