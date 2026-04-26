#!/usr/bin/make
# ====================================================
#   通用 Makefile
# ====================================================
CC    = gcc
RM    = rm -f
MKDIR = mkdir -p

CFLAGS   = -Wall -Wextra -O2 -g
LDFLAGS  = -lm
ARFLAGS  = rcs

# 目录定义
SRC_DIR   = src
LIB_DIR   = lib
BUILD_DIR = build
BIN_DIR   = bin
INCLUDE   = include

# 自动获取所有 .c 文件（排除 lib/ 和自身）
ALL_C_FILES = $(shell find $(SRC_DIR) -name "*.c" -not -path "*/lib/*" -not -path "*/res/*")
ALL_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(ALL_C_FILES))
# 可执行文件路径列表
ALL_PROGS = $(addprefix $(BIN_DIR)/, $(notdir $(basename $(ALL_C_FILES))))

# libxxx.so 配置
LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(LIB_SRCS:$(LIB_DIR)/%.c=$(BUILD_DIR)/lib/%.o)
LIB_TARGET = $(BUILD_DIR)/libctools.so

# 特定.c文件编译选项
LIBS_$(BUILD_DIR)/lrc = -lSDL2_mixer -lSDL2
LIBS_$(BUILD_DIR)/musicSynth/ALSA = -lasound -fopenmp
LIBS_$(BUILD_DIR)/network/socket = -lpthread
LIBS_$(BUILD_DIR)/tests/libav_test = -lavformat -lavcodec -lavutil -lswresample -lm
LIBS_$(BUILD_DIR)/tests/social = -lpthread
LIBS_$(BUILD_DIR)/tests/try_iconv = -liconv
LIBS_$(BUILD_DIR)/tetris = -lncurses

all: $(ALL_PROGS) ## 默认目标：编译所有程序
help: ## Prints help for targets with comments
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

# 确保构建目录存在
$(BUILD_DIR) $(BUILD_DIR)/lib $(BIN_DIR) $(sort $(dir $(ALL_OBJS))):
	$(MKDIR) $@

# 编译公共库
$(LIB_TARGET): $(LIB_OBJS) | $(BUILD_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.c | $(BUILD_DIR)/lib
	$(CC) $(CFLAGS) -c $< -o $@

# 通用编译规则：每个可执行文件由对应的 .c 和 libctools.a 链接而成
# 使用模式规则，但需要手动指定依赖的 libctools.a 和额外库
# 由于额外库变量名动态，这里用静态模式规则 + 二次展开
.SECONDEXPANSION:
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(LIB_TARGET) | $$(dir $$@)
	$(CC) $(CFLAGS) -c $< -o $@ $(LIB_TARGET) $(LDFLAGS) $(LIBS_$(@))

clean: ## 清理
	$(RM) -r $(BUILD_DIR)

list: ## 显示将会编译的程序列表（调试用）
	@echo "Available programs:"
	@echo $(ALL_PROGS) | tr ' ' '\n' | sed 's|^build/||'

.PHONY: all clean list
