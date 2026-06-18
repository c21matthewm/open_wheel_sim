#!/bin/sh

set -eu

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"

APP_PATH="${BUILD_DIR}/LightweightSim.app"
if [ ! -d "${APP_PATH}" ]; then
    APP_PATH="${BUILD_DIR}/${BUILD_TYPE}/LightweightSim.app"
fi

if [ ! -d "${APP_PATH}" ]; then
    echo "Could not find LightweightSim.app under ${BUILD_DIR}" >&2
    exit 1
fi

open "${APP_PATH}"
