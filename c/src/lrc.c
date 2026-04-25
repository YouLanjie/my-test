/*
 *   Copyright (C) 2025 u0_a221
 *
 *   文件名称：lrc.c
 *   创 建 者：u0_a221
 *   创建日期：2025年12月07日
 *   描    述：build lrc file?
 *
 */

#include "tools.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>

double get_time()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + t.tv_usec/1000000.0;
}

char *get_str_by_linenum(char *s, int n1, int n2)
{
	static char *p = NULL;
	if (p != NULL) {
		free(p);
		p = NULL;
	}
	if (!s) return NULL;
	if (n1 < 0) return NULL;
	if (n2 < n1) n2 = n1;
	int count = 0;
	char *op = NULL, *np = s;
	while (np && *np) {
		if (!op && count == n1) op = np;
		if (*np == '\n') count++;
		if (count > n2) break;
		np++;
	}
	if (!op) {
		return NULL;
	}
	p = malloc(sizeof(char)*(np - op + 1));
	memset(p, 0, np - op + 1);
	memcpy(p, op, np-op);
	/*printf("[LEN:%ld;LINE:%d;N1:%d]\n%s\n=====\n", np-op, n2-n1+1, n1, p);*/
	return p;
}

int main(int argc, char *argv[])
{
	int ch = 0;
	char *f_audio = "example.mp3",
	     *f_source = "example.txt";
	char *opt_rules = "i:s:";
	while ((ch = getopt(argc, argv, opt_rules)) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("RULES: %s\n", opt_rules);
			return ch == '?' ? -1 : 0;
			break;
		case 'i':
			f_audio = malloc(sizeof(char)*strlen(optarg));
			strcpy(f_audio, optarg);
			break;
		case 's':
			f_source = malloc(sizeof(char)*strlen(optarg));
			strcpy(f_source, optarg);
			break;
		default:
			break;
		}
	}

	printf("音频文件：%s\n歌词txt文件：%s\n",
	       f_audio, f_source);

	FILE *fp = fopen(f_source, "r");
	char *s_source = "NULL of SOURCES\n2\n3\n4\n5\n6\n7\n8\n9\n10\n";
	if (fp) {
		s_source = _fread(fp);
		fclose(fp);
	}


	// 初始化SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n",
		       SDL_GetError());
		return 1;
	}

	// 初始化SDL_mixer
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n",
		       Mix_GetError());
		return 1;
	}

	// 加载音频文件
	Mix_Chunk *music = Mix_LoadWAV(f_audio);
	if (music == NULL) {
		printf("Failed to load music! SDL_mixer Error: %s\n",
		       Mix_GetError());
		return 1;
	}

	// 播放音频文件
	int channel = Mix_PlayChannel(-1, music, -1);
	if (channel == -1) {
		printf("Failed to play music! SDL_mixer Error: %s\n",
		       Mix_GetError());
		return 1;
	}

	int input = 0;
	int count = 0;
	double total_time = 0,
	       base_time = get_time();
	int repeat = 0;
	printf("===================\n\n\n\n\n\n\n\n");
	while (input != 'q') {
		char info2[100] = "" ;
		sprintf(info2, repeat ? " [REPEAT:%d]" : "%d", repeat);
		char info[1024] = "";
		sprintf(info,"[COUNT:%3d] [TIME:%02d:%05.02lf] [STATUE:%s]%s%s [LAST:%s]",
		       count, (int)total_time/60, (int)total_time%60+(total_time-(int)total_time),
		       Mix_Paused(channel) ? "Paused" : "Playing",
		       isatty(STDERR_FILENO) ? " [WARN:stderr is tty!]" : "",
		       repeat ? info2 : "",
		       get_str_by_linenum(s_source, count-1, 0));
		print_in_box(info, 0, get_winsize_row()-7, -1, 1, 0, 0, "\033[0;30;42m", 1);
		print_in_box(s_source, 0, get_winsize_row()-6, -1, 5, count-2, count+1, "\033[0;30;43m", 1);
		// 等待用户输入以防止程序立即退出
		input = _getch();
		total_time += get_time() - base_time;
		base_time = get_time();
		if (input == ' ') {
			if (!Mix_Paused(channel)) {
				Mix_Pause(channel);
				base_time = -1;
			} else {
				Mix_Resume(channel);
				base_time = get_time();
			}
		} else if (input == '\n' && base_time > 0) {
			char *p = get_str_by_linenum(s_source, count, 0);
			if (!p) continue;
			fprintf(stderr, "[%02d:%05.02lf]%s\n",
				(int)total_time/60, (int)total_time%60+(total_time-(int)total_time),
				p);
			count++;
			repeat = 0;
		} else if (input == 'p' && base_time > 0) {
			fprintf(stderr, "[%02d:%05.02lf]%s\n",
				(int)total_time/60, (int)total_time%60+(total_time-(int)total_time),
				"==========");
			repeat++;
		}
	}

	// 释放资源并关闭SDL_mixer
	Mix_FreeChunk(music);

	Mix_Quit();
	SDL_Quit();
	return 0;

}
