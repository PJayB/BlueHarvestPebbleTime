#!/bin/sh
pebble build
if [ $? -eq 0 ] 
then
    pebble install --phone $1
    if [ $? -eq 0 ] 
    then
        pebble logs --phone $1
    fi
fi

