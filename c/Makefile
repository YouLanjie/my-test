#!/usr/bin/make

BUILD_DIR = ".build"
BIN_DIR = ".build"
SRCS = $(wildcard src/*.c)
# BUILD_EXEC = "./build"

.PHONY: all
all: build ## 构建所有程序(指定.c文件为目标可直接执行)
	@./build

.PHONY: clean
clean: ## 清除构建文件
	rm -rf $(BUILD_DIR) $(BIN_DIR)

help: ## Prints help for targets with comments
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

build: .build/build.o
	gcc -o $@ -O2 -g -Wall -Wextra $<

.build/build.o: build.c
	-mkdir $(BUILD_DIR)
	gcc -o $@ -c -O2 -g -Wall -Wextra $<

build.c:
	touch $@

$(SRCS): build
	@./build run $@
