# Atlas
This is the official repository for Atlas.

**__Developer Note:__** This is NOT a full release of atlas nor is it stable. This is a preview, expect bugs.

# Dependencies
 - Node.js 18+
 - Node Package Manager
 - Tailscale (optional)
 - Nginx
 - Cmake
 - GCC or Clang (C++ 20 compatible)
 - Nlohmann-json
 - Boost Libraries
 - PostgreSQL
 - libpqxx
 - Argon2
 - yaml-cpp
 - jwt-cpp

# Atlas Setup

> This portion of the guide is if you're running on MacOS

# Update Packages

```bash
brew update
brew upgrade
```

# Install Dependencies

```bash
brew install argon2 boost cmake gcc nginx libyaml nlohmann-json libpq libpqxx postgresql
```

> This portion of the guide is if you're running on Arch Linux

# Update Packages

```bash
sudo pacman -Syu
```

# Install Dependencies

```bash
sudo pacman -S nodejs npm libpqxx argon2 postgresql yaml-cpp nlohmann-json gcc cmake tailscale nginx boost
```

> This portion of the guide is if you're running on Ubuntu
 
# Update System

```bash
sudo apt update && sudo apt upgrade -y
```
---

# Install Dependencies

```bash
sudo apt install -y nodejs npm nginx postgresql postfresql-contrib libargon2-1 libargon2-dev nlohmann-json3-dev libpqxx-dev argon2 libboost-all-dev libyaml-cpp-dev
```

Make sure that postgresql is running on the system
```bash
sudo systemctl start postgresql
```

# Building jwt-cpp From Source

You'll need both JWT-CPP and Yaml-CPP to compile the server side. Navigate to `/libs/` and run the following commands
```bash
git clone https://github.com/Thalhammer/jwt-cpp.git
git clone https://github.com/jbeder/yaml-cpp.git
```

`cd` into the `jwt-cpp` directory and run:
```bash
mkdir build
cmake -S . -B build
cmake --build build
make install
```

then, `cd` back to `libs` and to the `jwp-cpp` directory and run:
```bash
mkdir build
cmake -S . -B build
cmake --build build
make install
```

# Finishing Setup

`cd` back into the atlas root directory then run:

```bash
chmod +x setup.sh
./setup.sh
```

That's all for setting up Atlas, now let's go on to setting up Nginx.

# Nginx Setup

# PostgreSQL Database Setup

# Tailscale Funnel (Optional but recommended)

## Atlas Setup Guide Contributions
*Last updated: March 5th, 2026 (3/5/26)*
 - [CastyiGlitchxz](https://github.com/CastyiGlitchxz)
 - [OzzieBeanie](https://github.com/ozziebeanie)