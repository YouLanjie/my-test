#!/usr/bin/make

CC = gcc
BIN = ./
SRC = $(shell find ./ -name "*.c")
incl = $(shell find ./ -name "*.h")
OBJ = $(SRC:%.c=%.o)
prom = arch-fast-install

all: arch-fast-install

$(prom): $(OBJ)
	$(CC) -z now $(OBJ) -lncurses -o $(BIN)/$(prom)

%.o: %.c $(incl)
	$(CC) -g -Wall -c $< -o $@

clean:
	rm -rf $(OBJ)

