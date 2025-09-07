#!/bin/bash

# Consts
BUILD_DIR=build

# Options
DO_CLEAN=0
ADD_UNIT_TESTS=OFF
ADD_EXAMPLES=OFF

for arg in "$@"; do
    case "$arg" in
        all)
            ADD_UNIT_TESTS=ON
            ADD_EXAMPLES=ON
            ;;
        clean)
            DO_CLEAN=1
            ;;
        TESTS)
            ADD_UNIT_TESTS=ON
            ;;
        EXAMPLES)
            ADD_EXAMPLES=ON
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [TESTS] [EXAMPLES] [clean]"
            exit 1
            ;;
    esac
done

if [ "$DO_CLEAN" -eq 1 ]; then
    echo "Cleaning build directory..."
    rm -rf ${BUILD_DIR}
    echo "Clean complete"
    exit 0
fi
    
cmake \
    -DENABLE_UNIT_TESTS=${ADD_UNIT_TESTS} \
    -DEXAMPLES=${ADD_EXAMPLES} \
    -B ${BUILD_DIR}

cmake --build ${BUILD_DIR}

cmake --build ${BUILD_DIR}
