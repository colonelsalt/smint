#!/bin/sh

mkdir -p build

g++ -g -o build/smint.o -Iinclude src/smint.cpp
