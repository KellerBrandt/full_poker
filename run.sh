#!/usr/bin/env bash
# Usage:
#   ./run.sh            build everything
#   ./run.sh <target>   build and run <target>, e.g. ./run.sh kuhn_test
set -e

cd "$(dirname "$0")"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug > /dev/null

if [ -n "$1" ]; then
	cmake --build build --target "$1"
	./build/"$1"
else
	cmake --build build
fi
