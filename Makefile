#!/usr/bin/make

CC = gcc
BIN = ./
SRC = $(shell find ./ -name "*.c")
incl = $(shell find ./ -name "*.h")
OBJ = $(SRC:%.c=%.o)

all: build

.PHONY: build
build: $(OBJ)
	$(CC) -z now $(OBJ) -lncurses -o $(BIN)/arch-fast-install

%.o: %.c $(incl)
	$(CC) -g -Wall -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ)

