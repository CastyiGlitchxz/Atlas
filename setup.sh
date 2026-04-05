#!/bin/bash
set -euo pipefail

echo "Preparing to setup Atlas. Please wait..."

# Creates server directories
mkdir -p build
mkdir -p assets/users/photos

# Creates the app config files
touch config/cenv config/app-config.yml

# Creates the server build directory
cmake -S . -B build
cmake --build build

# Installs node packages and builds the Next server
cd client
touch .env.local
npm install
npm run build
cd ..

# Exit message on success
echo "     Project successfully built.    "
echo "                                    "
echo "===================================="
echo "                                    "
echo "     Thank you for using Atlas      "
echo "                                    "
echo "===================================="