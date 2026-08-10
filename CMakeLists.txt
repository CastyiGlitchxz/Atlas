cmake_minimum_required(VERSION 3.16)
project(AtlasScarlet)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")
endif()

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)

add_executable(atlas_server 
    main.cpp 
    src/database.cpp 
    src/messaging.cpp 
    src/servers/server_service.cpp
    src/servers/channel_service.cpp 
    src/key_manager.cpp
    src/relationships.cpp
    src/notifications/notification_daemon.cpp
)

target_include_directories(atlas_server PRIVATE 
    ${PROJECT_SOURCE_DIR}/libs/jwt-cpp/include
)

if(APPLE)
    message(STATUS "Configuring for macOS")
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/homebrew/opt/libpq/lib/pkgconfig:/opt/homebrew/lib/pkgconfig")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/libpq")
    
    pkg_check_modules(LIBPQXX REQUIRED IMPORTED_TARGET libpqxx)
    
    find_path(ARGON2_INCLUDE_DIR argon2.h PATHS "/opt/homebrew/include")
    find_library(ARGON2_LIBRARY argon2 PATHS "/opt/homebrew/lib")
    
    target_include_directories(atlas_server PRIVATE ${Boost_INCLUDE_DIRS} ${ARGON2_INCLUDE_DIR})
    target_link_libraries(atlas_server PRIVATE PkgConfig::LIBPQXX)

elseif(UNIX)
    message(STATUS "Configuring for Linux/Unix")
    
    find_package(libpqxx REQUIRED)
    
    find_path(ARGON2_INCLUDE_DIR argon2.h)
    find_library(ARGON2_LIBRARY argon2)
    
    target_include_directories(atlas_server PRIVATE ${ARGON2_INCLUDE_DIR})
    target_link_libraries(atlas_server PRIVATE libpqxx::pqxx)
endif()

target_link_libraries(atlas_server
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    yaml-cpp
    ${ARGON2_LIBRARY}
    ${Boost_LIBRARIES}
)

target_compile_options(atlas_server PRIVATE -Wno-unknown-attributes)