cmake_minimum_required(VERSION 3.16)
project(AtlasScarlet)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(libpqxx REQUIRED)
find_package(PkgConfig REQUIRED)

add_executable(atlas_server 
    main.cpp 
    database.cpp 
    messaging.cpp 
    server.cpp 
    invites.cpp 
    filesystem.cpp
)

target_include_directories(atlas_server PRIVATE 
    ${PROJECT_SOURCE_DIR}/libs/jwt-cpp/include
)

if(APPLE)
    message(STATUS "Configuring for macOS")
    find_package(Boost REQUIRED)
    
    find_path(ARGON2_INCLUDE_DIR argon2.h PATHS "/opt/homebrew/include")
    find_library(ARGON2_LIBRARY argon2 PATHS "/opt/homebrew/lib")
    
    target_include_directories(atlas_server PRIVATE ${Boost_INCLUDE_DIRS} ${ARGON2_INCLUDE_DIR})

elseif(UNIX)
    message(STATUS "Configuring for Linux/Unix")
    
    find_path(ARGON2_INCLUDE_DIR argon2.h)
    find_library(ARGON2_LIBRARY argon2)
    
    target_include_directories(atlas_server PRIVATE ${ARGON2_INCLUDE_DIR})
endif()

target_link_libraries(atlas_server
    PRIVATE
    libpqxx::pqxx
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    yaml-cpp
    ${ARGON2_LIBRARY}
    ${Boost_LIBRARIES}
)

target_compile_options(atlas_server PRIVATE -Wno-unknown-attributes)