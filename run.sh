#!/bin/bash
set -e
# build
./build.sh
# run
cd ~/bin/audio/vcvrack/
./vcv_start.sh
