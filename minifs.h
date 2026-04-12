#include <stdint.h>
#include <time.h>

#define MAGIC_NUMBER 0x55AA55AA
#define BLOCK_SIZE   4096
#define MAX_INODES   1024
#define MAX_NAME     32
#define DIRECT_BLOCKS 12

// The "ID Card" for every file and directory
typedef struct {
    uint32_t inode_id;           // Unique ID
    char     name[MAX_NAME];     // Filename
    uint32_t size;               // File size in bytes
    uint32_t is_directory;       // 1 if directory, 0 if file
    time_t   created_at;         // Timestamp
    uint32_t blocks[DIRECT_BLOCKS]; // Pointers to data blocks
} Inode;

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t inode_count;
    uint32_t free_inodes;
} Superblock;