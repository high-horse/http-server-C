CC = gcc
CFLAGS = -I./lib -std=c2x -Wall -Wextra
SRC = main.c lib/server.c
OBJ = $(SRC:.c=.o)
OUT = bin/server.out

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(OUT)