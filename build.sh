#!/bin/bash

# MEC System Build Script

set -e

echo "Building MEC System..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

echo "Build completed successfully!"
echo "Executable: build/mec_system"
echo "Config file: config/mec.conf"
echo ""
echo "To install:"
echo "  sudo make install"
echo ""
echo "To run:"
echo "  ./build/mec_system"