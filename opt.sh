#!/bin/bash

# gets a sequence of accesses <a> and
# prints next to each access, the pos
# of the next access
# <a> <next>

if [ -z "$1" ]
then
	echo "Usage: $0 <size>"
	exit 1
fi

if [ -z "$2" ]
then
	awk '{print $1,$2,NR}' | tac | \
	awk '{ print $1, $2, $3, last[$2] ? last[$2]: 0; last[$2] = $3; }' | tac | ./opt $1 \
	< /dev/stdin
else
	./vscsiplay $1 | \
	awk '{print $1,NR}' | tac | \
	awk '{	print $1, $2, $3, last[$2] ? last[$2]: 0; last[$2] = $3; }' | tac
fi
