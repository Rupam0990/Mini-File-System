#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include "minifs.h"

// Helper: Find an inode by name in the vdisk
static int find_inode(const char *path, Inode *inode) {
    FILE *disk = fopen("vdisk", "rb");
    if (!disk) return -ENOENT;

    // Skip Superblock, go to Inode Table
    fseek(disk, BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < MAX_INODES; i++) {
        fread(inode, sizeof(Inode), 1, disk);
        // Compare path (ignoring the leading '/')
        if (inode->inode_id != 9999 && strcmp(inode->name, path + 1) == 0) {
            fclose(disk);
            return 0;
        }
    }
    fclose(disk);
    return -ENOENT;
}

// 1. GETATTR: System calls this to check if file exists and get its size
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

// 2. READDIR: Called when you run 'ls' in the mount directory
static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    FILE *disk = fopen("vdisk", "rb");
    if (!disk) return -EIO;

    fseek(disk, BLOCK_SIZE, SEEK_SET);
    Inode inode;
    for (int i = 0; i < MAX_INODES; i++) {
        fread(&inode, sizeof(Inode), 1, disk);
        if (inode.inode_id != 9999) {
            filler(buffer, inode.name, NULL, 0);
        }
    }
    fclose(disk);
    return 0;
}

// 3. CREATE: Called when you run 'touch'
static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return -EIO;

    // Read SB to update counts
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

            sb.free_inodes--;
            fseek(disk, 0, SEEK_SET);
            fwrite(&sb, sizeof(Superblock), 1, disk);
            
            fclose(disk);
            return 0;
        }
    }
    fclose(disk);
    return -ENOSPC;
}

// 4. READ: Called when you run 'cat'
static int do_read(const char *path, char *buffer, size_t size,
                   off_t offset, struct fuse_file_info *fi)
{
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

    // Use inode block pointer (NOT hardcoded 34)
    uint32_t block = inode.blocks[0];

    fseek(disk, block * BLOCK_SIZE + offset, SEEK_SET);
    int res = fread(buffer, 1, size, disk);

    fclose(disk);
    return res;
}


// 5. WRITE: Called when you use '>' or 'echo'
static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    Inode inode;
    if (find_inode(path, &inode) != 0) return -ENOENT;

    FILE *disk = fopen("vdisk", "rb+");
    if (!disk) return -EIO;

    // --- NEW BLOCK ALLOCATOR LOGIC ---
    // If the file is new (size 0 and no block assigned), find a free block
    if (inode.blocks[0] == 0) {
        uint32_t candidate = 34; // Start search after the Inode Table
        int is_used = 1;

        while (is_used) {
            is_used = 0;
            fseek(disk, BLOCK_SIZE, SEEK_SET); // Scan the Inode Table
            Inode temp;
            for (int i = 0; i < MAX_INODES; i++) {
                fread(&temp, sizeof(Inode), 1, disk);
                // If another file is already using this block, skip to next
                if (temp.inode_id != 9999 && temp.blocks[0] == candidate) {
                    candidate++;
                    is_used = 1;
                    break;
                }
            }
        }
        inode.blocks[0] = candidate;
    }

    // Write the data to the uniquely assigned block
    fseek(disk, inode.blocks[0] * BLOCK_SIZE + offset, SEEK_SET);
    fwrite(buffer, 1, size, disk);

    // Update metadata and save the Inode
    inode.size = offset + size;
    fseek(disk, BLOCK_SIZE + (inode.inode_id * sizeof(Inode)), SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, disk);

    fclose(disk);
    return size;
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
    .utimens = do_utimens,
};


int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &operations, NULL);
}