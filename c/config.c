/**
 * @file        config.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       构建器的配置文件(用.h报错真的是)
 */

#define SOURCE_DIR "./src/"
#define LIB_DIR    "./lib/"
#define BUILD_DIR  "./.build/"
#define BIN_DIR    "./bin/"
#define COMPILOR   "gcc"
#define CCOMFLAGS  "-Wall -Wextra -O2 -g"
#define CLINKFLAGS "-L"BUILD_DIR" -lctools"

typedef struct {
	char *libname;
	char *sources[5];
	/*char *flags;*/
} CLIBS_t;
#define ARRAY_LEN(v) (sizeof(v)/sizeof(v[0]))

CLIBS_t CLIBS[] = {
	{BUILD_DIR"libctools.a", {LIB_DIR"tools.c", LIB_DIR"print_in_box.c"}},
	{BUILD_DIR"libcmenu.a", {LIB_DIR"menu.c"}},
};
char *CFILEFLAGS[][3] = {
	{SOURCE_DIR"lrc.c", "", "-lSDL2_mixer -lSDL2"},
	{SOURCE_DIR"musicSynth/ALSA.c", "", "-lasound -fopenmp -lm"},
	{SOURCE_DIR"network/socket.c", "", "-lpthread"},
	{SOURCE_DIR"tests/libav_test.c", "", "-lavformat -lavcodec -lavutil -lswresample -lm"},
	{SOURCE_DIR"tests/social.c", "", "-lm"},
	{SOURCE_DIR"tests/try_iconv.c", "", "-liconv"},
	{SOURCE_DIR"tetris.c", "", "-lncurses"},
	{SOURCE_DIR"render3d.c", "", "-lm"},
	{SOURCE_DIR"tests/input.c", "", "-lm"},
	{SOURCE_DIR"tests/sin.c", "", "-lm"},
};

