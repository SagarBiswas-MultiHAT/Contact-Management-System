#!/usr/bin/env sh
# Purpose: Build and run tests. Author: Sagar Biswas
set -e
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build
