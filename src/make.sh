#!/bin/bash

CURRENT_DIR="$(pwd)"
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

IDF_PATH=$(realpath "${SCRIPT_DIR}/../libs/ESP8266_RTOS_SDK")
docker run --rm -it \
    -e ESPPORT="/dev/ttyUSB0" \
    -e IDF_PATH="$IDF_PATH" \
    -v $(git rev-parse --show-toplevel):$(git rev-parse --show-toplevel) \
    --device=/dev/ttyUSB0 \
    --workdir $(git rev-parse --show-toplevel)/src esp-docker:1.0 \
    bash -c "$@"
