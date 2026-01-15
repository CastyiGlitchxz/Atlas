cmake_minimum_required(VERSION 3.16)
project(AtlasScarlet)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

# Add executable first
add_executable(atlas_server main.cpp database.cpp messaging.cpp server.cpp invites.cpp)

# Include directories
target_include_directories(atlas_server PRIVATE 
    ${PROJECT_SOURCE_DIR}/libs/jwt-cpp/include
    ${OPENSSL_INCLUDE_DIR}
)

# OS-specific configuration
if(APPLE)
    message(STATUS "Configuring for macOS")

    # Boost
    find_package(Boost REQUIRED)
    target_include_directories(atlas_server PRIVATE ${Boost_INCLUDE_DIRS})

    # libpqxx
    set(LIBPQXX_INCLUDE_DIR "/opt/homebrew/opt/libpqxx/include")
    set(LIBPQXX_LIBRARY "/opt/homebrew/opt/libpqxx/lib/libpqxx.dylib")

    set(ARGON2_INCLUDE_DIR "/opt/homebrew/include")
    set(ARGON2_LIBRARY "/opt/homebrew/lib/libargon2.dylib")

    target_include_directories(atlas_server PRIVATE ${LIBPQXX_INCLUDE_DIR})
    target_link_libraries(atlas_server PRIVATE ${LIBPQXX_LIBRARY} /opt/homebrew/opt/libpq/lib/libpq.dylib)

    include_directories(/opt/homebrew/opt/openssl@3/include)
    link_directories(/opt/homebrew/opt/openssl@3/lib)

    target_include_directories(atlas_server PRIVATE ${ARGON2_INCLUDE_DIR})
    target_link_libraries(atlas_server PRIVATE ${ARGON2_LIBRARY})

elseif(UNIX)
    message(STATUS "Configuring for Linux/Unix")

    pkg_check_modules(PQXX REQUIRED libpqxx)
    target_include_directories(atlas_server PRIVATE ${PQXX_INCLUDE_DIRS})
    target_link_libraries(atlas_server PRIVATE ${PQXX_LIBRARIES})

    find_path(ARGON2_INCLUDE_DIR argon2.h)
    find_library(ARGON2_LIBRARY argon2)
endif()

# Link common libraries
target_link_libraries(atlas_server
    PRIVATE
    yaml-cpp
    PostgreSQL::PostgreSQL
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ARGON2_LIBRARY}
    ${Boost_LIBRARIES}  # empty on Linux if Boost not found
)

# Compiler options
target_compile_options(atlas_server PRIVATE -Wno-unknown-attributes)
