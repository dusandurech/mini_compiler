#!/bin/bash

SOURCE="testSource"

./main -lex lex automat.txt
./main -lr grama table.txt
./main -c $SOURCE compile.out
./main -t compile.out out.asm

nasm -f elf out.asm
gcc out.o
./a.out
