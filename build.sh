#!/bin/bash

# Color codes for the script
GREEN='\033[1;32m'
CYAN='\033[1;36m'
YELLOW='\033[1;33m'
RESET='\033[0m'

echo -e "${CYAN}🚀 Starting SumFS Master Build...${RESET}"

# List of all source files
commands=("format" "liss" "tuch" "remov" "readit" "say" "makedir" "hop" "remdir" "guard" "owner" "info" "space" "size" "findit" "link" "wherami" "cls" "quit")

for cmd in "${commands[@]}"; do
    if [ -f "$cmd.c" ]; then
        echo -e "  [🔨] Compiling ${YELLOW}$cmd${RESET}..."
        gcc "$cmd.c" -o "$cmd"
        chmod +x "$cmd"
    else
        echo -e "  [⚠️] Warning: $cmd.c not found, skipping."
    fi
done

# Initialize the current working directory tracker if it doesn't exist
if [ ! -f ".sumfs_cwd" ]; then
    echo "0" > .sumfs_cwd
fi

echo -e "\n${GREEN}✅ SUCCESS: All SumFS commands are ready to run!${RESET}"
echo -e "${YELLOW}👉 Next Step: Run ./format to initialize your disk.${RESET}\n"
