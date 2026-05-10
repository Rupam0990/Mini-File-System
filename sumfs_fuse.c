#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include "sumfs.h"

// Helper to print current disk stats
void print_disk_stats() {
    FILE *disk = fopen("vdisk", "rb");
    if (!disk) return;
    Superblock sb;
    fread(&sb, sizeof(Superblock), 1, disk);
    fclose(disk);

    printf("\n" CYAN "==== CURRENT SUMFS STATE ====" RESET "\n");
    printf("Free Inodes : " YELLOW "%u" RESET " / %u\n", sb.free_inodes, sb.inode_count);
    printf("Free Blocks : " YELLOW "%u" RESET " / %u\n", sb.free_blocks, sb.total_blocks);
    printf(CYAN "=============================" RESET "\n\n");
}

// Helper: Find an inode by name in the vdisk
static int find_inode(const char *path, Inode *inode) {
    FILE *disk = fopen("vdisk", "rb");
    if (!disk) return -ENOENT;

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < MAX_INODES; i++) {
        fread(inode, sizeof(Inode), 1, disk);
        if (inode->inode_id != 9999 && strcmp(inode->name, path + 1) == 0) {
            fclose(disk);
            return 0;
        }
    }
    fclose(disk);
    return -ENOENT;
}

static int do_getattr(const char *path, struct stat *st) {
    memset(st, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    Inode inode;
    if (find_inode(path, &inode) == 0) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = inode.size;
        st->st_atime = st->st_mtime = st->st_ctime = inode.created_at;
        return 0;
    }

    return -ENOENT;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    printf(BLUE "[READDIR]" RESET " Scanning root directory...\n");
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) return -EIO;

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode inode;
    int count = 0;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&inode, sizeof(Inode), 1, disk);
        if (inode.inode_id != 9999) {
            filler(buffer, inode.name, NULL, 0);
            count++;
        }
    }
    fclose(disk);
    printf(BLUE "[READDIR]" RESET " Found " YELLOW "%d" RESET " entries\n", count);
    return 0;
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf(GREEN "[CREATE]" RESET " File: " YELLOW "%s" RESET "\n", path + 1);
    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return -EIO;

    Superblock sb;
    fread(&sb, sizeof(Superblock), 1, disk);

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode temp;
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&temp, sizeof(Inode), 1, disk);
        if (temp.inode_id == 9999) {
            Inode new_file;
            memset(&new_file, 0, sizeof(Inode));
            new_file.inode_id = i;
            strncpy(new_file.name, path + 1, MAX_NAME - 1);
            new_file.size = 0;
            new_file.created_at = time(NULL);

            fseek(disk, offset, SEEK_SET);
            fwrite(&new_file, sizeof(Inode), 1, disk);

            printf(GREEN "[ALLOC ]" RESET " Inode " CYAN "%d" RESET " assigned to " YELLOW "%s" RESET "\n", i, new_file.name);

            sb.free_inodes--;
            fseek(disk, 0, SEEK_SET);
            fwrite(&sb, sizeof(Superblock), 1, disk);
            
            fclose(disk);
            print_disk_stats();
            return 0;
        }
    }
    fclose(disk);
    printf(RED "[ERROR ]" RESET " No free inodes left!\n");
    return -ENOSPC;
}

static int do_read(const char *path, char *buffer, size_t size,
                   off_t offset, struct fuse_file_info *fi)
{
    printf(BLUE "[READ  ]" RESET " File: " YELLOW "%s" RESET " | Offset: %ld | Size: %zu\n", path + 1, offset, size);
    Inode inode;
    if (find_inode(path, &inode) != 0)
        return -ENOENT;

    if (offset >= inode.size)
        return 0;

    if (offset + size > inode.size)
        size = inode.size - offset;

    FILE *disk = fopen("vdisk", "rb");
    if (!disk)
        return -EIO;

    uint32_t block = inode.blocks[0];
    printf(BLUE "[BLOCK ]" RESET " Accessing block " CYAN "%u" RESET "\n", block);

    fseek(disk, block * BLOCK_SIZE + offset, SEEK_SET);
    int res = fread(buffer, 1, size, disk);

    fclose(disk);
    printf(BLUE "[READ  ]" RESET " Successfully read " YELLOW "%d" RESET " bytes\n", res);
    return res;
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf(MAGENTA "[WRITE ]" RESET " File: " YELLOW "%s" RESET " | Size: %zu bytes\n", path + 1, size);
    Inode inode;
    if (find_inode(path, &inode) != 0) return -ENOENT;

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return -EIO;

    // --- BLOCK ALLOCATOR LOGIC ---
    if (inode.blocks[0] == 0) {
        uint32_t candidate = 34; 
        int is_used = 1;

        while (is_used) {
            is_used = 0;
            fseek(disk, BLOCK_SIZE, SEEK_SET);
            Inode temp;
            for (int i = 0; i < MAX_INODES; i++) {
                fread(&temp, sizeof(Inode), 1, disk);
                if (temp.inode_id != 9999 && temp.blocks[0] == candidate) {
                    candidate++;
                    is_used = 1;
                    break;
                }
            }
        }
        inode.blocks[0] = candidate;
        printf(MAGENTA "[ALLOC ]" RESET " Block " CYAN "%u" RESET " assigned to " YELLOW "%s" RESET "\n", candidate, inode.name);
        
        // Update superblock free blocks
        Superblock sb;
        fseek(disk, 0, SEEK_SET);
        fread(&sb, sizeof(Superblock), 1, disk);
        sb.free_blocks--;
        fseek(disk, 0, SEEK_SET);
        fwrite(&sb, sizeof(Superblock), 1, disk);
    }

    fseek(disk, inode.blocks[0] * BLOCK_SIZE + offset, SEEK_SET);
    fwrite(buffer, 1, size, disk);

    inode.size = offset + size;
    fseek(disk, BLOCK_SIZE + (inode.inode_id * sizeof(Inode)), SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, disk);

    fclose(disk);
    printf(MAGENTA "[WRITE ]" RESET " " GREEN "SUCCESS" RESET " | File size is now " YELLOW "%u" RESET " bytes\n", inode.size);
    print_disk_stats();
    return size;
}

static int do_unlink(const char *path) {
    printf(RED "[DELETE]" RESET " Removing file: " YELLOW "%s" RESET "\n", path + 1);
    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return -EIO;

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode inode;
    for (int i = 0; i < MAX_INODES; i++) {
        long offset = ftell(disk);
        fread(&inode, sizeof(Inode), 1, disk);
        if (inode.inode_id != 9999 && strcmp(inode.name, path + 1) == 0) {
            
            uint32_t freed_block = inode.blocks[0];
            
            // Mark Inode as free
            inode.inode_id = 9999;
            memset(inode.name, 0, MAX_NAME);
            fseek(disk, offset, SEEK_SET);
            fwrite(&inode, sizeof(Inode), 1, disk);

            // Update Superblock
            Superblock sb;
            fseek(disk, 0, SEEK_SET);
            fread(&sb, sizeof(Superblock), 1, disk);
            sb.free_inodes++;
            if (freed_block != 0) {
                sb.free_blocks++;
                printf(RED "[FREE  ]" RESET " Block " CYAN "%u" RESET " returned to pool\n", freed_block);
            }
            fseek(disk, 0, SEEK_SET);
            fwrite(&sb, sizeof(Superblock), 1, disk);

            printf(RED "[DELETE]" RESET " " GREEN "SUCCESS" RESET " | Inode " CYAN "%d" RESET " is now free\n", i);
            fclose(disk);
            print_disk_stats();
            return 0;
        }
    }
    fclose(disk);
    return -ENOENT;
}

static int do_utimens(const char *path, const struct timespec tv[2]) {
    return 0; 
}

static struct fuse_operations operations = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .create  = do_create,
    .read    = do_read,
    .write   = do_write,
    .unlink  = do_unlink,
    .utimens = do_utimens,
};

int main(int argc, char *argv[]) {
    printf(GREEN "Starting SumFS Driver..." RESET "\n");
    printf(CYAN "Waiting for operations in Terminal 2..." RESET "\n\n");
    return fuse_main(argc, argv, &operations, NULL);
}
