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
#include <stdint.h>

#define BUFFSIZE 2048
#define SCR_HEIGHT 4

#define NFLG_LEFT (1 << 0)
#define NFLG_RIGHT (1 << 1)
#define NFLG_LEGATO (1 << 2)
#define NFLG_PORTAMENTO (1 << 3)

unsigned int SAMPLE_RATE = 44100;

struct Note {
	double freq;
	double amplitude;
	double   type;		/* type分音符 */
	uint8_t  notes;		/* 以notes分音符为一拍 */
	uint8_t  beates;	/* 每小节beates拍 */
	uint8_t  speed;		/* 速度，n拍/分钟 */
	uint8_t  track;
	uint8_t  flag;		/* NFLG_* flags */
	uint16_t *pwav;
	struct Note *pNext;
};

struct Note *create_note(double freq)
{
	struct Note *p = malloc(sizeof(struct Note));
	if (!p) return NULL;
	*p = (struct Note){
		freq, .amplitude=0.5,
		.type=4, .notes=4, .beates=4,
		.speed=120, .track=0,
		.flag=NFLG_LEFT|NFLG_RIGHT,
		.pwav=NULL, NULL
	};
	return p;
}

char *i2b(unsigned long data, int bytes, char *buf, size_t buf_size)
{
	if (!buf || buf_size == 0) return NULL;
	if (bytes <= 0 || bytes > sizeof(data)) bytes = sizeof(data);

	int bits = bytes * 8;
	if (bits >= buf_size) bits = buf_size - 1;  // 留1字节给'\0'

	for (int i = 0; i < bits; ++i) {
		unsigned long mask = 1UL << (bits - 1 - i);  // 1UL 保证至少 32/64 位
		buf[i] = (data & mask) ? '1' : '0';
	}
	buf[bits] = '\0';
	return buf;
}

struct Note *parse_notes()
{
	char istty = 0;
	int (*input_func)() = getchar;
	if (isatty(STDIN_FILENO)) {
		printf("stdin是tty,改用_getch()\n");
		input_func = _getch;
		istty = 1;
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
	     scr2[BUFSIZ-200] = "",
	     buf[30] = "",
	     key[40] = "",
	     value[40] = "";
	double fade = 0;
	uint8_t track = 0, setting_mode = 0;
	int beates = 4, /* 每小节type拍(重置计数器用) */
	    notes = 4,  /* 以几分音符为一拍 */
	    type = 4,   /* 默认type分音符 */
	    speed = 120,
	    c = 0;
	struct Note *pH = NULL, *p = NULL;
	if (istty) for (int i = 0; i < SCR_HEIGHT; ++i) printf("\n");
	while (c != EOF) {
		if (istty) {
			if (p) {
				sprintf(scr2,
					"[%d/%d %dbpm:%d] 1/%-3.2lg %5.1lfHz (%0.4lg%%) [%s]\n",
					p->notes,p->beates,p->speed,p->track,p->type,
					p->freq, p->amplitude*100, i2b(p->flag, 1, buf, 30));
			} else sprintf(scr2, "No Note was created.\n");
			sprintf(scr,
				"[%d/%d %dbpm:%d] 1/%d  || fade:%lf\n"
				"KEY: '%s' VALUE: '%s'\n%s",
				notes,beates,speed,track,type, fade,
				key, value, scr2);
			print_in_box(scr, 0, get_winsize_row()-SCR_HEIGHT,
				     -1, SCR_HEIGHT, 0, 0, NULL, 0);
		}

		c=input_func();
		if (c==EOF) break;
		if (c >= 256 || c < 0) continue;

		if (setting_mode == 0) {
			if (istty && c == 'q') break;
		} else if (setting_mode <= 40) {
			if (c != '=') key[setting_mode-1] = c;
			else setting_mode = 40;
			setting_mode++;
			continue;
		} else if (setting_mode <= 40+40) {
			if (c != ';') value[setting_mode-41] = c;
			else {
				setting_mode = 0;
#define ifin(str, fmt, var, check) if (strcmp(key, str) == 0) {sscanf(value, fmt, &var); check;} else
				ifin("track", "%hhd", track, )
				ifin("speed", "%d", speed, if(speed<1) speed=120)
				ifin("beates", "%d", beates, if(beates<1) )
				ifin("notes", "%d", notes, if(notes%4!=0||notes<1) notes=4)
				ifin("type", "%d", type, if(type%4!=0||type<1)type=4);
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
			/*p->duration = SAMPLE_RATE*((double)base/4)*((double)60/speed);*/
			p->type = type;
			p->track = track;
			p->speed = speed;
			if (!pH) pH = p;
			if (p2) {
				p2->pNext = p;
				p->amplitude = p2->amplitude*(1.0+fade);
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
			/*if (p) free(p);*/
			/*p = NULL;*/
			c = 0;
			break;
		case '/':
			p->type *= 2;
			break;
		case '*':
			p->type /= 2;
			break;
		case '.':
			p->type /= (3.0/2.0);
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
			fade += 0.005;
			break;
		case '>':
			fade -= 0.005;
			break;
		case '=':
			fade = 0;
			break;
		case ':':
			setting_mode = 1;
			break;
		case 'q':
			c = EOF;
			break;
		}
	}
	if (istty)printf("\e[%d;%dH\n====Quit====\n", get_winsize_row()-1, 0);
	return pH;
}

int main(int argc, char *argv[])
{

	int count = 0;
	struct Note *pH = NULL,
		    *p = NULL,
		    *p2 = NULL;
	char buf[30] = "";
	p2 = p = pH = parse_notes();
	while (p) {
		printf("[%d/%d %dbpm:%d] 1/%-3.2lg %5.1lfHz (%0.4lg%%) [%s]\n",
		       p->notes,p->beates,p->speed,p->track,p->type,
		       p->freq, p->amplitude*100, i2b(p->flag, 1, buf, 30));
		p = p->pNext;
		count++;
		free(p2);
		p2 = p;
	}
	printf("Count: %d\n", count);
	return 0;
}

