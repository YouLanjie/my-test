/*
 *   Copyright (C) 2026 Chglish
 *
 *   文件名称：ALSA_str_read.c
 *   创 建 者：Chglish
 *   创建日期：2026年02月11日
 *   描    述：ALSA.c的乐谱语法读取前瞻测试程序
 *
 */


#include "tools.h"
#include <limits.h>

#define BUFFSIZE 2048
#define SCR_HEIGHT 8
#define SCR_WIDTH 32

#define NFLG_LEFT (1 << 0)
#define NFLG_RIGHT (1 << 1)
#define NFLG_LEGATO (1 << 2)
#define NFLG_PORTAMENTO (1 << 3)

unsigned int SAMPLE_RATE = 44100;

struct Note {
	double freq;
	double amplitude;
	double fade;
	unsigned long duration;
	int flag;
	short *pwav;
	struct Note *pNext;
};

struct Note *create_note(double freq)
{
	struct Note *p = malloc(sizeof(struct Note));
	if (!p) return NULL;
	*p = (struct Note){freq, 0.5, 0, INT_MAX,
		NFLG_LEFT|NFLG_RIGHT, NULL, NULL};
	return p;
}

char *i2b(long data, int len)
{
	static char s[41] = "";
	memset(s, 0, 41);
	for (int i = 0; i < len*8; ++i) {
		s[i] = data & (1 << (len*8-i-1)) ? '1' : '0';
	}
	return s;
}

int main(int argc, char *argv[])
{
	int (*input_func)() = getchar;
	if (isatty(STDIN_FILENO)) {
		printf("stdin是tty,改用_getch()\n");
		input_func = _getch;
	}
	static const double note_freq[256] = { 0,
		['c'] = 130.8, ['d'] = 146.8, ['e'] = 164.8, ['f'] = 174.6,
		['g'] = 196.0, ['a'] = 220.0, ['b'] = 246.9,
		['C'] = 261.6, ['D'] = 293.6, ['E'] = 329.6, ['F'] = 349.2,
		['G'] = 392.0, ['A'] = 440.0, ['B'] = 493.9,
		['1'] = 523.2, ['2'] = 587.3, ['3'] = 659.2, ['4'] = 698.5,
		['5'] = 784.0, ['6'] = 880.0, ['7'] = 987.8, ['0'] = -1,
		['{'] = 20, ['}'] = 20000};
	char scr[BUFSIZ] = "",
	     key[40] = "",
	     value[40] = "";
	unsigned short setting_mode = 0;
	int type = 4, base = 4, speed = 120,
	    track = 0, c = 0;
	struct Note *pH = NULL, *p = NULL;
	for (int i = 0; i < SCR_HEIGHT; ++i) printf("\n");
	while (c != EOF) {
		if (p) {
			sprintf(scr,
				"Typ:%d;Base:%d;SPD:%d;TRAC:%d\n"
				"KEY: '%s'  VALUE: '%s'\n"
				"Freq: %5.1lf  Amp: %lf\nduration: %ld (%d)\n"
				"fadeout: %lf\nLEGATO: %d\nPORTAMENTO: %d\n"
				"LR: %d,%d (flg:%s)",
				type, base, speed, track, key, value,
				p->freq, p->amplitude, p->duration,
				(int)(INT_MAX/(p->duration?p->duration:INT_MAX)),
				p->fade, p->flag&NFLG_LEGATO, p->flag&NFLG_PORTAMENTO,
				p->flag&NFLG_LEFT, p->flag&NFLG_RIGHT, i2b(p->flag, 2));
		} else sprintf(scr,
			       "Typ: %d; Base: %d; SPD:%d\n"
				"KEY: '%s'  VALUE: '%s'\n"
				"No Note was created.\n",
				type, base, speed, key, value);
		print_in_box(scr, 0, get_winsize_row()-SCR_HEIGHT,
			     SCR_WIDTH, SCR_HEIGHT, 0, 0, NULL, 0);

		c=input_func();
		if (c==EOF) break;
		if (c >= 256 || c < 0) continue;

		if (setting_mode == 0) {
			if (c == 'q') c = EOF;
		} else if (setting_mode <= 40) {
			if (c != '=') key[setting_mode-1] = c;
			else setting_mode = 40;
			setting_mode++;
			continue;
		} else if (setting_mode <= 40+40) {
			if (c != ';') value[setting_mode-41] = c;
			else {
				setting_mode = 0;
#define ifin(str, var, check) if (strcmp(key, str) == 0) {sscanf(value, "%d", &var); check;} else
				ifin("track", track, )
				ifin("base", base, if(base%4!=0||base<1) base=4)
				ifin("type", type, if(type<1)type=4);
#undef ifin
				memset(key, 0, 40);
				memset(value, 0, 40);
				continue;
			}
			setting_mode++;
			continue;
		} else {
			setting_mode = 0;
		}

		if (note_freq[c]) {
			struct Note *p2 = p;
			p = create_note(note_freq[c]);
			p->duration = SAMPLE_RATE*(base/type)*(60/speed);
			if (p2) {
				if (!pH) pH = p2;
				p2->pNext = p;
			}
		} else if (c == ':') {
			setting_mode = 1;
		} else if (!p) continue;

		switch (c) {
		case '|':
		case ';':
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			if (p) free(p);
			p = NULL;
			c = 0;
			break;
		case '/':
			p->duration /= 2;
			break;
		case '*':
			p->duration *= 2;
			break;
		case '.':
			p->duration *= (3.0/2.0);
			break;
		case '~':
			p->flag ^= NFLG_LEGATO;
			break;
		case 's':
			p->flag ^= NFLG_PORTAMENTO;
			break;
		case '+':
			p->amplitude += p->amplitude + 0.1 <= 1 ? 0.1 : 0;
			break;
		case '-':
			p->amplitude -= p->amplitude - 0.1 >= 0 ? 0.1 : 0;
			break;
		case 'l':
			p->freq-=10;
			break;
		case 'L':
			p->freq/=2;
			break;
		case 'u':
			p->freq+=10;
			break;
		case 'U':
			p->freq*=2;
			break;
		case '[':
			p->flag ^= NFLG_LEFT;
			break;
		case ']':
			p->flag ^= NFLG_RIGHT;
			break;
		case '<':
			p->fade += 0.005;
			break;
		case '>':
			p->fade -= 0.005;
			break;
		case '=':
			p->fade = 0;
			break;
		case ':':
			setting_mode = 1;
			break;
		case 'q':
			c = EOF;
			break;
		}
	}
	printf("\e[%d;%dH\n====Quit====\n", get_winsize_row()-1, 0);

	p = pH;
	struct Note *p2 = p;
	while (p) {
		printf("Freq: %5.1lf  Amp: %lf\nduration: %ld (%d)\n"
		       "fadeout: %lf\nLEGATO: %d\nPORTAMENTO: %d\n"
		       "LR: %d,%d (flg:%s)\n=============\n",
		       p->freq, p->amplitude, p->duration,
		       (int)(INT_MAX/(p->duration?p->duration:INT_MAX)),
		       p->fade, p->flag&NFLG_LEGATO, p->flag&NFLG_PORTAMENTO,
		       p->flag&NFLG_LEFT, p->flag&NFLG_RIGHT, i2b(p->flag, 2));
		p = p->pNext;
		free(p2);
		p2 = p;
	}
	return 0;
}

