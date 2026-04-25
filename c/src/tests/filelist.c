#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

char *dirtypes[][2] = {
	[DT_UNKNOWN]
		  = {"未  知", "31"},
	[DT_FIFO] = {"管  道", "37"},
	[DT_CHR]  = {"块设备", "33"},
	[DT_DIR]  = {"文件夹", "34"},
	[DT_BLK]  = {"存  储", "33"},
	[DT_REG]  = {"文  件", "37"},
	[DT_LNK]  = {"链  接", "36"},
	[DT_SOCK] = {"套接字", "35"},
	[DT_WHT]  = {"啥玩意", "44"},
};
int typeslen = sizeof(dirtypes)/sizeof(dirtypes[0]);

void listdir(char *dirname)
{
	if (!dirname) return;
	DIR *dp = opendir(dirname);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", dirname);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return;
	}
	printf("\033[1;33m%s :\033[0m\n", dirname);
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		uint8_t type = dp_item->d_type;
		if (type >= typeslen) type = DT_UNKNOWN;
		printf("%6s  ||  \033[1;%sm%s\033[0m\n",
		       dirtypes[type][0], dirtypes[type][1],
		       dp_item->d_name);
	}
	closedir(dp);
	printf("\033[1;36m==================\033[0m\n");
}

int main(int argc, char *argv[])
{
	printf("\033[1;33m类  型  ||  名  字\n"
	       "\033[1;36m==================\033[0m\n");
	if (argc == 1) {
		listdir("./");
		return 0;
	}
	while (--argc && ++argv) {
		listdir(*argv);
	}
	return 0;
}
