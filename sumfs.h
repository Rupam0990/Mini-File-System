#include <stdint.h>
#include <time.h>

// Color codes for visualization
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define RESET   "\033[0m"

#define MAGIC_NUMBER 0x55AA55AA
#define BLOCK_SIZE   4096
#define MAX_INODES   1024
#define MAX_NAME     32
#define DIRECT_BLOCKS 12

// File Types
#define TYPE_FILE 0
#define TYPE_DIR  1
#define TYPE_LINK 2

// Default Permissions
#define DEFAULT_DIR_MODE  0755
#define DEFAULT_FILE_MODE 0644

// The "ID Card" for every file and directory
typedef struct {
    uint32_t inode_id;           // Unique ID (9999 = free)
    char     name[MAX_NAME];     // Name
    uint32_t size;               // Size in bytes
    uint32_t type;               // TYPE_FILE, TYPE_DIR, or TYPE_LINK
    uint16_t mode;               // Permissions (e.g., 0755)
    uint16_t uid;                // User ID
    uint16_t gid;                // Group ID
    uint32_t parent_id;          // Inode ID of parent directory
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