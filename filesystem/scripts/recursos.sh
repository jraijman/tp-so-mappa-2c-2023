#!/bin/bash

@echo off

rm ./fs/bloques.dat
rm ./fs/fat.dat
rm ./fs/fcbs/*.fcb

echo "Eliminación completada."

make
./bin/filesystem.out ./config/recursos.config