#!/bin/bash

for (( ; ; ))
do

	read -n1 -r -p "Press key to program..." key

	./write_328p.sh
done
