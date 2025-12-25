#!/bin/bash
#
# @file install_deps.sh
# @brief Installation script for sandbox platform dependencies.
#
# This script installs all required dependencies for building and running
# the sandbox platform on Ubuntu/Debian systems.
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Installing sandbox platform dependencies...${NC}"

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    echo -e "${YELLOW}Warning: Not running as root. Some operations may require sudo.${NC}"
fi

# Update package lists
echo -e "${GREEN}[1/6] Updating package lists...${NC}"
apt-get update

# Install build dependencies
echo -e "${GREEN}[2/6] Installing build dependencies...${NC}"
apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# Install required libraries
echo -e "${GREEN}[3/6] Installing required libraries...${NC}"
apt-get install -y \
    libcurl4-openssl-dev \
    libcap-dev \
    libseccomp-dev

# Install system tools for sandboxing
echo -e "${GREEN}[4/6] Installing system tools...${NC}"
apt-get install -y \
    debootstrap \
    uidmap \
    dbus-user-session

# Create required directories
echo -e "${GREEN}[5/6] Creating required directories...${NC}"
mkdir -p /var/lib/sandbox/rootfs
mkdir -p /var/lib/sandbox/images
mkdir -p /var/log/sandbox
mkdir -p /etc/sandbox

# Install sandbox binary
echo -e "${GREEN}[6/6] Building sandbox...${NC}"

# Navigate to source directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

# Build the project
if [[ -f "CMakeLists.txt" ]]; then
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    make install
    cd ..
    echo -e "${GREEN}Sandbox built and installed successfully!${NC}"
else
    echo -e "${YELLOW}CMakeLists.txt not found. Skipping build step.${NC}"
    echo -e "${YELLOW}Please build manually: mkdir build && cd build && cmake .. && make${NC}"
fi

# Verify installation
if command -v sandbox &> /dev/null; then
    echo -e "${GREEN}Installation complete!${NC}"
    echo -e "Run ${YELLOW}sandbox --help${NC} for usage information."
else
    echo -e "${YELLOW}Installation may require adding /usr/local/bin to PATH.${NC}"
fi

# Enable unprivileged user namespaces (optional, for rootless mode)
echo -e "${YELLOW}Note: For user namespace support, you may need to enable it:${NC}"
echo -e "${YELLOW}  echo 2 > /proc/sys/kernel/unprivileged_userns_clone${NC}"
