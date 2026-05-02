/**
 * @file        note_parser.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       音符解释器
 */

#include "../core.h"
#include <assert.h>

static char *i2b(unsigned long data, size_t bytes, char *buf, size_t buf_size)
{
	if (!buf || buf_size == 0) return NULL;
	if (bytes <= 0 || bytes > sizeof(data)) bytes = sizeof(data);

	uint64_t bits = bytes * 8;
	if (bits >= buf_size) bits = buf_size - 1;  // 留1字节给'\0'

	for (uint64_t i = 0; i < bits; ++i) {
		uint64_t mask = 1UL << (bits - 1 - i);  // 1UL 保证至少 32/64 位
		buf[i] = (data & mask) ? '1' : '0';
	}
	buf[bits] = '\0';
	return buf;
}

void print_note(Note_t *p)
{
	if (!p) return;
	char buf[30] = "";
	fprintf(stderr,
		"[%d/%d %dbpm:%d] 1/%-3.2lg %5.1lfHz (%0.4lg%%) [%s] (%d:%d:'%c')\n",
		p->notes,p->beates,p->speed,p->track,p->type,
		p->freq, p->amplitude*100, i2b(p->flag, 1, buf, 30),
		p->line, p->ind, p->ch);
}

void check_notes(Note_t *p, bool print)
{
	char *colors[2][2] = { {"", ""}, {"\e[31m", "\e[0m"} };
	int istty = isatty(STDERR_FILENO);
	int istty2 = isatty(STDOUT_FILENO);
	double counter = 0;
	int track = 0;
	for (; p ; p = p->pNext) {
		if (track!=p->track) {
			track = p->track;
			if (print)
				printf("\n:%strack=%d%s;\n", colors[istty2][0],
				       track, colors[istty2][1]);
			counter = 0;
		}
		if (print) {
			printf("%c", p->ch);
			for (double i=p->type; i && i>4; i/=2) printf("/");
			for (double i=p->type; i && i<4; i*=2) printf("*");
			for (double i=p->type; i!=floor(i); i*=3) printf(".");
			if (p->flag&NFLG_LEGATO) printf("~");
			else if (p->flag&NFLG_PORTAMENTO) printf("s");
			else printf(" ");
		}
		counter += p->notes/p->type;
		if (counter < p->beates) continue;
		if (counter > p->beates) {
			if (print)
				printf("%s%s<<%s", istty2 ? "\e[7m" : "",
				       colors[istty2][0], colors[istty2][1]);
			fprintf(stderr, "[%sWARN%s] 不合拍(%g/%d): ",
				colors[istty][0], colors[istty][1],
				counter, p->beates);
			print_note(p);
		}
		/*printf("%s(%f)| %s", colors[istty][0], tempo_counter, colors[istty][1]); */
		if (print)
			printf("%s|%s\n", colors[istty2][0], colors[istty2][1]);
		counter = 0;
		fflush(stderr);
		fflush(stdout);
	}
	if (print) printf("\n");
	return;
}

#define KEYVALUE_BUF_SIZE 40
/* 解读字符串形式的音符并产生解析好的音符串struct */
Note_t *parse_notes(const char *str, FILE *fp, double base_amplitude)
{
	static const double note_freq[256] = { 0,
		['c'] = 130.8, ['d'] = 146.8, ['e'] = 164.8, ['f'] = 174.6,
		['g'] = 196.0, ['a'] = 220.0, ['b'] = 246.9,
		['C'] = 261.6, ['D'] = 293.6, ['E'] = 329.6, ['F'] = 349.2,
		['G'] = 392.0, ['A'] = 440.0, ['B'] = 493.9,
		['1'] = 523.2, ['2'] = 587.3, ['3'] = 659.2, ['4'] = 698.5,
		['5'] = 784.0, ['6'] = 880.0, ['7'] = 987.8, ['0'] = -1,
		['{'] = 20, ['}'] = 20000};
	const double change_freq = exp2(1/12.0);
	char key[KEYVALUE_BUF_SIZE] = "",
	     value[KEYVALUE_BUF_SIZE] = "";
	double fade = 0, bq_freq = 0;
	double amplitude = 0.2;
	uint8_t track = 0, setting_mode = 0,
		instrument = 0, wave_func = 0,
		bq = 0;
	int8_t  harmonics = HAR_NOSET;
	int beates = 4, /* 每小节type拍(重置计数器用) */
	    notes = 4,  /* 以几分音符为一拍 */
	    type = 4,   /* 默认type分音符 */
	    speed = 120,
	    c = 0,
	    count = 1,
	    countline = 1;
	Note_t *pH = NULL, *p = NULL;
	while (c != EOF) {
		if (str) {
			c=*str;
			if (!c) break;
			str++;
		} else if (fp) c=fgetc(fp);
		else c=getchar();
		if (c==EOF) break;
		if (c=='\n') {
			count=0;
			countline++;
		} else count++;
		if (c >= 256 || c < 0) continue;

		if (setting_mode == 0) {
		} else if (setting_mode <= KEYVALUE_BUF_SIZE) {
			if (c == '=' || setting_mode==KEYVALUE_BUF_SIZE-1)
				setting_mode = KEYVALUE_BUF_SIZE;
			else if (c == ';') setting_mode=KEYVALUE_BUF_SIZE*2;
			else key[setting_mode-1] = c;
			setting_mode++;
			continue;
		} else if (setting_mode <= KEYVALUE_BUF_SIZE*2-1) {
			if (c != ';') {
				value[setting_mode-KEYVALUE_BUF_SIZE-1] = c;
				setting_mode++;
				if (setting_mode >= KEYVALUE_BUF_SIZE*2)
					setting_mode--;
				continue;
			}
			setting_mode = 0;
#define ifin(keystr, fmt, var, check) if (strcmp(key, keystr) == 0) {sscanf(value, fmt, &var); check;} else
			ifin("track", "%hhd", track, )
			ifin("speed", "%d", speed, if(speed<1) speed=120)
			ifin("amp", "%lf", amplitude, if(amplitude<0||amplitude>=0.8)amplitude=0.2)
			ifin("beates", "%d", beates, if(beates<1) )
			ifin("notes", "%d", notes, if(notes%4!=0||notes<1) notes=4)
			ifin("type", "%d", type, if(type%4!=0||type<1)type=4)
			ifin("inst", "%hhu", instrument, instrument%=INST_MAX)
			ifin("wfunc", "%hhu", wave_func, wave_func%=WF_MAX)
			ifin("bq", "%hhu", bq, bq%=BQ_MAX)
			ifin("bq_freq", "%lf", bq_freq, if(bq_freq<0)bq_freq=0)
			ifin("har", "%hhd", harmonics,)
			;    /* 必须加个分号终结else分支 */
#undef ifin
			memset(key, 0, KEYVALUE_BUF_SIZE);
			memset(value, 0, KEYVALUE_BUF_SIZE);
			continue;
		} else {
			memset(key, 0, KEYVALUE_BUF_SIZE);
			memset(value, 0, KEYVALUE_BUF_SIZE);
			setting_mode = 0;
		}

		if (note_freq[c]) {
			Note_t *p2 = p;
			p = malloc(sizeof(Note_t));
			assert(p);
			/*if (!p) return NULL;*/
			*p = (Note_t){
				.ch=c, .ind=count, .line=countline,
				.freq=note_freq[c], .amplitude=amplitude*base_amplitude,
				.type=type, .notes=4, .beates=beates,
				.speed=speed,
				.duration=0,
				.track=track,
				.flag=NFLG_LEFT|NFLG_RIGHT,
				.instrument=instrument,
				.wave_func=wave_func,
				.harmonics=harmonics,
				.bq=bq,
				.bq_freq=bq_freq,
				.pwav=NULL, NULL
			};
			if (!pH) pH = p;
			if (p2) {
				p2->pNext = p;
				p->flag |= p2->flag&NFLG_LEGATO ? NFLG_BE_LEGATO : 0;
				if (fade) p->amplitude = p2->amplitude*(1.0+fade);
			}
		} else if (c == ':') {
			setting_mode = 1;
		} else if (!p) continue;

		switch (c) {
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
		case 'l':    /* 跨半音 */
			p->freq/=change_freq;
			break;
		case 'L':    /* 跨八度 */
			p->freq/=2;
			break;
		case 'u':
			p->freq*=change_freq;
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
	return pH;
}

void free_notes(Note_t *p)
{
	Note_t *lp = p;
	while (p) {
		lp=p;
		p=p->pNext;
		if (lp->pwav) free(lp->pwav);
		free(lp);
	}
	return;
}
