#!/bin/sh

mkdir -p build

g++ -g -o ./build/smint -I./include ./src/smint.cpp
