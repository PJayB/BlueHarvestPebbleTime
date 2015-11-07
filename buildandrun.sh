#!/bin/sh
pebble build
if [ $? -eq 0 ] 
then
    pebble install --emulator basalt build/blueharvestpebble.pbw
fi

