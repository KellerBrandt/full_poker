#!/usr/bin/env bash
# Usage:
#   ./run.sh            build everything
#   ./run.sh <target>   build and run <target>, e.g. ./run.sh kuhn_test
#   *_bench targets build in Release, in build-release/
set -e

cd "$(dirname "$0")"

build_dir=build
build_type=Debug

if [[ "$1" == *_bench ]]; then
	build_dir=build-release
	build_type=Release
fi

if [ ! -f "$build_dir/Makefile" ] || [ CMakeLists.txt -nt "$build_dir/Makefile" ]; then
	torch_cmake_dir=$(ls -d "$CONDA_PREFIX"/lib/python3*/site-packages/torch/share/cmake 2>/dev/null | head -1 || true)

	cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type" -DCMAKE_PREFIX_PATH="$torch_cmake_dir" > /dev/null
fi

if [ -n "$1" ]; then
	cmake --build "$build_dir" --target "$1"
	./"$build_dir"/"$1"
else
	cmake --build "$build_dir"
fi
