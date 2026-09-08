#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "========== 清理旧的 build =========="
rm -rf "$BUILD_DIR"

echo "========== 创建 build =========="
mkdir -p "$BUILD_DIR"

echo "========== CMake =========="
cd "$BUILD_DIR"
cmake ..

echo "========== 编译 =========="
cmake --build . -j8

echo "========== 编译完成 =========="
echo "生成的程序："
ls -lh "$PROJECT_DIR/bin"

echo "生成的库："
ls -lh "$PROJECT_DIR/lib"