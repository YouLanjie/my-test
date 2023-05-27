#!/usr/bin/make

CC = gcc
BIN = .
SRC = $(shell find ./ -name "*.c")
incl = $(shell find ./ -name "*.h")
OBJ = $(SRC:%.c=%.o)
prom = my-test

all: my-test

$(prom): $(OBJ)
	$(CC) -z now $(OBJ) -lncurses -o $(BIN)/main

%.o: %.c $(incl)
	$(CC) -g -Wall -c $< -o $@

clean:
	rm -rf $(OBJ)

