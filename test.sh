#!/bin/bash

printf "arc.c "
cat seq.dat | ./arc 10
printf "lru "
cat seq.dat | ./lru 10
printf "mru "
cat seq.dat | ./mru 10
printf "min "
cat seq.dat | ./min 10
printf "opt "
cat seq.dat | bash ./opt.sh 10


{ seq 1 10 && seq 1 10 && seq 1 10; } | awk '{print "W",$0}' | ./min 3
{ seq 1 10 && seq 1 10 && seq 1 10; } | awk '{print "W",$0}' | bash ./opt.sh 3
