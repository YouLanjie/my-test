#include "include/tools.h"

int flag_3gp = 0;

typedef struct Object{
	char *input;
	char *output;
	struct Object *pn;
} Object;

Object *get_files(char *dirname, char *t)
{
	DIR *dp = opendir(dirname);
	if (!dp) return NULL;
	if (!t) t = "mp3";
	/*char *t_3gp = " -r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000 ";*/
	Object *ph = NULL, *pn = NULL, *pl = NULL;
	struct dirent *pf = readdir(dp);
	for (; pf; pf = readdir(dp)) {
		if (pf->d_type != 8) continue;
		pn = malloc(sizeof(Object));
		pn->input = malloc(strlen(dirname)+strlen(pf->d_name)+2);
		sprintf(pn->input, "%s/%s", dirname, pf->d_name);
		pn->output = malloc(strlen(dirname)+strlen(pf->d_name)+strlen(t)+7);
		sprintf(pn->output, "%s/out/%s", dirname, pf->d_name);
		int len = strlen(pn->output)-1, i = 0;
		for ( ; i <= 4 && pn->output[len-i] != '.'; i++);
		if (pn->output[len-i] == '.' && i > 0) pn->output[len-i] = 0;
		sprintf(pn->output, "%s.%s", pn->output, t);
		pn->pn = NULL;
		if (!ph) ph = pn;
		if (pl) pl->pn = pn;
		pl = pn;
	}
	closedir(dp);
	return ph;
}

int conversion(char *dirname, int i, Object *p)
{
	if (!dirname || !p) exit(-1);

	usleep(10000);
	if (access(p->output, 0) == 0) {
		printf("[SKIP]: %s\n", p->output);
		exit(1);
	}
	char *log = malloc(strlen(dirname) + 15);
	sprintf(log, "%s/out/Log%03d", dirname, i);
	const char *cmd_3gp = flag_3gp ? "-r 12 -b:v 400k -s 352x288 -ab 12.2k -ac 1 -ar 8000" : "";
	const char *format = "ffmpeg -i \"%s\" %s \"%s\" > \"%s\" 2>&1";
	char *command = malloc(strlen(format)+strlen(cmd_3gp)+
			       strlen(p->input)+strlen(p->output)+
			       strlen(log)+10);
	sprintf(command, format, p->input, cmd_3gp, p->output, log);

	struct timeval time, time2;
	gettimeofday(&time, NULL);
	/*printf("RUN: %s", command);*/
	if (system(command) == 0) {
		gettimeofday(&time2, NULL);
		remove(log);
		printf("[Spent]%2.6fs: %s\n",
		     time2.tv_sec - time.tv_sec + (double)(time2.tv_usec - time.tv_usec) / 1000000,
		     p->input);
		exit(0);
	}
	struct stat statbuf;
	stat(p->output, &statbuf);
	if (statbuf.st_size == 0) remove(p->output);
	printf("Err:[%03d] %s\n", i, p->input);
	freopen(log, "a", stdout);
	freopen(log, "a", stderr);
	printf("[INP]: %s\n[CMD]: %s\n[MSG]: ", p->input, command);
	perror(p->input);
	fclose(stdout);
	fclose(stderr);
	exit(-1);
}

int main(int argc, char *argv[])
{
	char type[10] = "\0",      /* 文件后缀名 */
	     dirname[200] = "./\0";    /* 目录文件名 */
	int opt;

	while ((opt = getopt(argc, argv, "vt:d:h")) != -1) {
		switch (opt) {
		case 'v':
			flag_3gp = 1;
			break;
		case 't':
			if (strcmp(optarg, "?") == 0)
				return -1;
			strcpy(type, optarg);
			break;
		case 'd':
			if (strcmp(optarg, "?") == 0)
				return -2;
			strcpy(dirname, optarg);
			break;
		case '?':
		case 'h':
		default:
			printf("本程序基于ffmpeg，转换格式时需要安装ffmpeg\n参数：Type_conversion [-t <目标格式>] [-d <文件夹>] [-v 3gp] [-h]帮助\n");
			return 0;
			break;
		}
	}
	if (dirname[0] == 0 || type[0] == 0) {
		printf("[!] 文件名或类型未指定\n");
		return 1;
	}
	char outputdir[220] = "";
	sprintf(outputdir, "%s/out", dirname);
	mkdir(outputdir, 0777);
	Object *list = get_files(dirname, type), *p = list;
	struct timeval time, time2;
	int i = 0;
	gettimeofday(&time, NULL);
	for (; p != NULL; p = p->pn) {
		if ((i + 1) % 7 == 0) {
			usleep(15000);
			printf("[MAIN]: 进程较多，等待后再开始\n");
			while (wait(NULL) != -1) ;
		}
		pid_t pid = fork();
		if (pid == 0) {
			printf("Loaded: %s\n", p->input);
			conversion(dirname, i+1, p);
			exit(-1);
		}
		i++;
	}
	gettimeofday(&time2, NULL);

	usleep(15000);
	printf("[MAIN]: 耗时%2.6f秒加载完毕，开始等待\n",
	     time2.tv_sec - time.tv_sec + (double)(time2.tv_usec - time.tv_usec) / 1000000);

	while (wait(NULL) != -1) ;

	gettimeofday(&time2, NULL);
	printf("[MAIN]: %d个文件完成，耗时%2.6f秒\n",
	     i, time2.tv_sec - time.tv_sec + (double)(time2.tv_usec - time.tv_usec) / 1000000);
	return 0;
}
