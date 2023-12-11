#!/bin/bash

echo "\n\t\tSETEO DE IPs\n"

read -p "Ingrese nueva IP_FILESYSTEM: " IP_FILESYSTEM

grep -RiIl 'IP_FILESYSTEM' | xargs sed -i 's|\(IP_FILESYSTEM\s*=\).*|\1'$IP_FILESYSTEM'|'

echo "\nLos .config han sido modificados correctamente\n"