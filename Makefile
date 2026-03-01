.PHONY: build, run

build:
	gcc -std=c2x -Wall -Wextra -pedantic main.c -o a.out

run:
	./a.out
