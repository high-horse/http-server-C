GC=gcc
WARNINGS=-Wall -Wextra
STANDARD=-std=c2x


build:
	 $(GC) $(STANDARD) $(WARNINGS) main.c -o a.out
run:
	 ./a.out
