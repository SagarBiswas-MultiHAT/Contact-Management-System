# Purpose: Build configuration for Contact Manager. Author: GitHub Copilot
cmake_minimum_required(VERSION 3.15)
project(ContactManager C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(SQLite3 REQUIRED)
find_package(PkgConfig)

set(HAVE_SODIUM OFF)
set(HAVE_ARGON2 OFF)

# Prefer CMake config packages (vcpkg) when available; fall back to pkg-config below
find_package(unofficial-sodium CONFIG QUIET)
if(unofficial-sodium_FOUND)
    set(HAVE_SODIUM ON)
    set(SODIUM_TARGET unofficial-sodium::sodium)
endif()

if(PkgConfig_FOUND)
    pkg_check_modules(SODIUM libsodium)
    pkg_check_modules(ARGON2 libargon2)
endif()

if(SODIUM_FOUND)
    set(HAVE_SODIUM ON)
elseif(ARGON2_FOUND)
    set(HAVE_ARGON2 ON)
else()
    message(FATAL_ERROR "Neither libsodium nor libargon2 was found. Install one of them.")
endif()

add_executable(contacts
    src/main.c
    src/db.c
    src/auth.c
    src/contacts.c
    src/csv.c
    src/util.c
)

target_include_directories(contacts PRIVATE include)

target_link_libraries(contacts PRIVATE SQLite::SQLite3)

if(HAVE_SODIUM)
    target_compile_definitions(contacts PRIVATE HAVE_LIBSODIUM)
    if(TARGET ${SODIUM_TARGET})
        target_link_libraries(contacts PRIVATE ${SODIUM_TARGET})
    else()
        target_include_directories(contacts PRIVATE ${SODIUM_INCLUDE_DIRS})
        target_link_libraries(contacts PRIVATE ${SODIUM_LIBRARIES})
    endif()
elseif(HAVE_ARGON2)
    target_compile_definitions(contacts PRIVATE HAVE_ARGON2)
    target_include_directories(contacts PRIVATE ${ARGON2_INCLUDE_DIRS})
    target_link_libraries(contacts PRIVATE ${ARGON2_LIBRARIES})
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
