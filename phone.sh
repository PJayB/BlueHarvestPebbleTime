#!/bin/sh
pebble build
pebble install --phone $1
pebble logs --phone $1
