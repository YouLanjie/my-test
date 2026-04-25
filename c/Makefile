#!/usr/bin/make
# ====================================================
#   通用 Makefile
# ====================================================
SHELL = /bin/sh
CC    = gcc
AR    = ar
RM    = rm -f
MKDIR = mkdir -p

CFLAGS   = -Wall -Wextra -O2 -g
#LDFLAGS  = -lm
ARFLAGS  = rcs

# 目录定义
SRC_DIR   = src
LIB_DIR   = lib
BUILD_DIR = build
INCLUDE   = include

# 自动获取所有 .c 文件（排除 lib/ 和自身）
ALL_C_FILES = $(shell find $(SRC_DIR) -name "*.c" -not -path "*/lib/*" -not -path "*/res/*")

# 生成对应的可执行文件路径列表（去掉 .c 后缀，放在 build/ 下）
# 例如 src/musicSynth/ALSA.c -> build/musicSynth/ALSA
ALL_PROGS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%, $(ALL_C_FILES))

# libctools.a 依赖的所有 .c 文件
LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(LIB_SRCS:$(LIB_DIR)/%.c=$(BUILD_DIR)/lib/%.o)
LIB_TARGET = $(BUILD_DIR)/libctools.a

# ====================================================
#   特殊程序需要的额外链接库（可扩展）
# ====================================================
# 格式：build/相对路径:库列表（空格分隔）
LIBS_$(BUILD_DIR)/lrc = -lSDL2_mixer -lSDL2
LIBS_$(BUILD_DIR)/musicSynth/ALSA = -lasound -fopenmp -lm
LIBS_$(BUILD_DIR)/network/socket = -lpthread
LIBS_$(BUILD_DIR)/render3d = -lm
LIBS_$(BUILD_DIR)/tests/input = -lm
LIBS_$(BUILD_DIR)/tests/libav_test = -lavformat -lavcodec -lavutil -lswresample -lm
LIBS_$(BUILD_DIR)/tests/sin = -lm
LIBS_$(BUILD_DIR)/tests/social = -lpthread -lm
LIBS_$(BUILD_DIR)/tests/try_iconv = -lm -liconv
LIBS_$(BUILD_DIR)/tetris = -lncurses

# ====================================================
#   默认目标：编译所有程序
# ====================================================
all: $(ALL_PROGS)

help: ## Prints help for targets with comments
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

# 确保构建目录存在
$(BUILD_DIR) $(BUILD_DIR)/lib $(sort $(dir $(ALL_PROGS))):
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
$(BUILD_DIR)/%: $(SRC_DIR)/%.c $(LIB_TARGET) | $$(dir $$@)
	$(CC) $(CFLAGS) $< -o $@ $(LIB_TARGET) $(LDFLAGS) $(LIBS_$(@))


clean: ## 清理
	$(RM) -r $(BUILD_DIR)

list: ## 显示将会编译的程序列表（调试用）
	@echo "Available programs:"
	@echo $(ALL_PROGS) | tr ' ' '\n' | sed 's|^build/||'

.PHONY: all clean list

# ====================================================
#   链接目标：将 build/ 中的可执行文件链接到 bin/ 下
# ====================================================
LINK_DIR = bin

# 查找 build/ 下所有可执行文件（普通文件且可执行）
EXEC_FILES = $(shell find $(BUILD_DIR) -type f -executable)

# ---------- 方案1：扁平结构（所有文件直接放在 bin/ 下）----------
# 如果有同名文件（如 build/foo/bar 和 build/baz/bar），后写的会覆盖前一个
LINK_TARGETS_FLAT = $(addprefix $(LINK_DIR)/, $(notdir $(EXEC_FILES)))

$(LINK_DIR)/%: $(BUILD_DIR)/*/% | $(LINK_DIR)
	@echo "Linking $< -> $@"
	ln -sf $(realpath --relative-to=$(LINK_DIR) $<) $@

# ---------- 方案2：保留子目录结构（bin/ 下结构与 build/ 相同）----------
LINK_TARGETS_TREE = $(patsubst $(BUILD_DIR)/%, $(LINK_DIR)/%, $(EXEC_FILES))

$(LINK_DIR)/%: $(BUILD_DIR)/% | $(LINK_DIR)
	mkdir -p $(dir $@)
	ln -sf $(realpath --relative-to=$(LINK_DIR) $<) $@

# 默认采用方案1（扁平），若要改用方案2，将下面一行注释互换即可
link: $(LINK_TARGETS_FLAT)  ## 将 build/ 中的可执行文件链接到 bin/ 下
# link: $(LINK_TARGETS_TREE)

$(LINK_DIR):
	mkdir -p $@

unlink:  ## 移除链接
	rm -rf $(LINK_DIR)

.PHONY: link unlink
