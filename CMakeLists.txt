cmake_minimum_required(VERSION 3.16)
project(AtlasScarlet)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PostgreSQL / libpqxx include paths
include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

# Threads
find_package(Threads REQUIRED)

# Include jwt-cpp + picojson
include_directories(${PROJECT_SOURCE_DIR}/libs/jwt-cpp/include)

# OpenSSL include and link
include_directories(/opt/homebrew/opt/openssl@3/include)
link_directories(/opt/homebrew/opt/openssl@3/lib)

# Add executable
add_executable(atlas_server main.cpp database.cpp messaging.cpp server.cpp invites.cpp)

# Link libraries
target_link_libraries(atlas_server
    PRIVATE
    pqxx
    /opt/homebrew/opt/libpq/lib/libpq.dylib
    Threads::Threads
    ssl
    crypto
    argon2
)

target_compile_options(atlas_server PRIVATE -Wno-unknown-attributes)
