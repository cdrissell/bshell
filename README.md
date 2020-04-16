# bshell

This program, written in C, implements a minimal shell. Finds executables by
parsing through the directories in the PATH environment variable.

## Getting Started

To run the program, first compile the 'bshell.c' file using gcc. Then run the executable. 

- Example:
  - to compile, enter: 'gcc -o bshell bshell.c'
  - to run: './bshell'

## Requirements and Dependencies

- GCC
  - To check if GCC is installed, type "gcc -v" in the command line
  - If it is not installed, go here https://gcc.gnu.org/install/
- Dependencies
  - unistd.h
  - stdio.h
  - stdlib.h
  - signal.h
  - string.h
  - sys/wait.h