#!/bin/bash

set -e

cmake -B build
cmake --build build

XEPHYR=$(whereis -b Xephyr | cut -f2 -d' ')

xinit ./xinitrc -- \
  "$XEPHYR" \
  :100 \
  -ac \
  -screen 800x600 \
  -host-cursor