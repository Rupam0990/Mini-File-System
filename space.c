#include <stdio.h>
#include <stdlib.h>
#include "sumfs.h"

int main() {
    FILE *disk = fopen("vdisk", "rb");
    if (!disk) {
        perror("Error opening disk");
        return 1;
    }

    Superblock sb;
    fread(&sb, sizeof(Superblock), 1, disk);
    fclose(disk);

    uint32_t used_blocks = sb.total_blocks - sb.free_blocks;
    uint32_t used_inodes = sb.inode_count - sb.free_inodes;

    double block_percent = (double)used_blocks / sb.total_blocks * 100;
    double inode_percent = (double)used_inodes / sb.inode_count * 100;

    printf("\n" CYAN "========= SUMFS DISK SPACE =========" RESET "\n");
    printf(" Total Space : " YELLOW "%.2f MB" RESET " (%u blocks)\n", (double)(sb.total_blocks * BLOCK_SIZE) / (1024 * 1024), sb.total_blocks);
    printf(" Used Space  : " RED "%.2f MB" RESET " (%u blocks) [" MAGENTA "%.1f%%" RESET "]\n", (double)(used_blocks * BLOCK_SIZE) / (1024 * 1024), used_blocks, block_percent);
    printf(" Free Space  : " GREEN "%.2f MB" RESET " (%u blocks)\n", (double)(sb.free_blocks * BLOCK_SIZE) / (1024 * 1024), sb.free_blocks);
    printf("------------------------------------\n");
    printf(" Total Inodes: " YELLOW "%u" RESET "\n", sb.inode_count);
    printf(" Used Inodes : " RED "%u" RESET " [" MAGENTA "%.1f%%" RESET "]\n", used_inodes, inode_percent);
    printf(" Free Inodes : " GREEN "%u" RESET "\n", sb.free_inodes);
    printf(CYAN "====================================" RESET "\n\n");

    return 0;
}
