#!/bin/bash

if [ -z "$1" ]
  then
    echo "No argument supplied"
    exit 1
fi

if [ -d /Volumes/RPI-RP2 ]; then
    cp build/src/$1.uf2 /Volumes/RPI-RP2
else
    openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program build/src/$1.elf verify reset exit"
fi
