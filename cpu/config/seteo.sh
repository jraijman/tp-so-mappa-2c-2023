#!/bin/bash

echo "\n\t\tSETEO DE IPs\n"

read -p "Ingrese nueva IP_MEMORIA: " IP_MEMORIA

grep -RiIl 'IP_MEMORIA' | xargs sed -i 's|\(IP_MEMORIA\s*=\).*|\1'$IP_MEMORIA'|'

echo "\nLos .config han sido modificados correctamente\n"