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

cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type" > /dev/null

if [ -n "$1" ]; then
	cmake --build "$build_dir" --target "$1"
	./"$build_dir"/"$1"
else
	cmake --build "$build_dir"
fi
