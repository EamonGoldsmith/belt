#!/bin/sh

for file in $(ls examples/*.c)
do
	bin="${file%.*}.out"
	cc "$file" -g -Wall -o "$bin"
done
