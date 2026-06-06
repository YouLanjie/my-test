/**
 * @file        note_parser.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       音符解释器
 */

#include "../core.h"

void print_note(Note_t *note)
{
	if (!note) return;
	NoteData_t *p = note->pcm_data;
	if (!p) return;
	fprintf(stderr,
		"[%d/%d %dbpm:%d] 1/%-3.2lg %5.1lfHz (%0.4lg%%) [%d%d%d%d%d] (%d:%d:'%c')\n",
		p->notes,p->beates,p->speed,note->track,p->type,
		p->freq, note->amplitude*100,
		note->flg_left, note->flg_right, note->flg_legato,
		note->flg_be_legato, p->portamento_from > 0,
		note->line, note->ind, note->ch);
}

void check_notes(Note_t *note, bool print)
{
	if (!note) return;
	char *colors[2][2] = { {"", ""}, {"\e[31m", "\e[0m"} };
	int istty = isatty(STDERR_FILENO);
	int istty2 = isatty(STDOUT_FILENO);
	double counter = 0;
	int track = 0;
#define p (note->pcm_data)
	for (; note && p ; note = note->pNext) {
		if (track!=note->track) {
			track = note->track;
			if (print)
				printf("\n:%strack=%d%s;\n", colors[istty2][0],
				       track, colors[istty2][1]);
			counter = 0;
		}
		if (print) {
			printf("%c", note->ch);
			for (double i=p->type; i && i>4; i/=2) printf("/");
			for (double i=p->type; i && i<4; i*=2) printf("*");
			for (double i=p->type; (int)i!=floor(i); i*=3) printf(".");
			if (note->flg_legato) printf("~");
			else if (note->pcm_data && note->pcm_data->portamento_from>0)
				printf("s");
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
			print_note(note);
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
#undef p

void notedata_set(NoteData_t *p, char *key, char *value)
{
	if (!p || !value || !key || !*key) return;
	double f = atof(value);
	if (strcmp(key, "speed") == 0) {
		p->speed = f  > 0 ? f : 120;
	} else if (strcmp(key, "wave_func") == 0) {
		if (strcmp(value, "sin") == 0) p->wave_func = sin;
		else if (strcmp(value, "triangle") == 0) p->wave_func = wave_triangle;
		else if (strcmp(value, "sawtooth") == 0) p->wave_func = wave_sawtooth;
		else if (strcmp(value, "noise") == 0) p->wave_func = wave_noise;
	} else if (strcmp(key, "har") == 0) {
		if (strcmp(value, "piano") == 0) p->har_func = har_set_piano1;
		if (strcmp(value, "piano2") == 0) p->har_func = har_set_piano2;
	} else if (strcmp(key, "flo") == 0) {
		if (strcmp(value, "drum")) p->flo_freq_func = flo_inverse_liner;
	} else if (strcmp(key, "beates") == 0) {
		p->beates = f > 0 ? f : 4;
	} else if (strcmp(key, "notes") == 0 || strcmp(key, "base_on") == 0) {
		p->notes = f > 0 ? f : 4;
	} else if (strcmp(key, "type") == 0 || strcmp(key, "default_type") == 0) {
		p->type = f > 0 ? f : 4;
	}
	return;
}

bool notedata_cmp(NoteData_t *n1, NoteData_t *n2)
{
	if (!n1 || !n2) return false;
	if (n1 == n2) return true;
	if (n1->har_func != n2->har_func) return false;
	if (n1->wave_func != n2->wave_func) return false;
	if (n1->flo_freq_func != n2->flo_freq_func) return false;
	if (n1->portamento_from != n2->portamento_from) return false;
	if (n1->freq != n2->freq) return false;
	if (n1->type != n2->type) return false;
	if (n1->speed != n2->speed) return false;
	if (n1->notes != n2->notes) return false;
	if (n1->beates != n2->beates) return false;
	return true;
}

NoteData_t *notedata_search(NoteData_t *pHead, NoteData_t *obj)
{
	if (!obj) return NULL;
	NoteData_t *p = pHead;
	while (p && notedata_cmp(p, obj) == false) p = p->pNext;
	return p;
}

#define MAX_LEN_KEY 80
#define MAX_LEN_VALUE 80

static int process_key_value(char c, int *ind, char *key, char *value, Note_t *note, NoteData_t *data)
{
	if (!ind || !key || !value || !note || !data) return -1;
	if (*ind == 0) return 0;

	if (*ind <= MAX_LEN_KEY) {
		if (c == '=') *ind = MAX_LEN_KEY;
		else if (*ind==MAX_LEN_KEY-1)
			*ind = MAX_LEN_KEY+MAX_LEN_VALUE-1;    /* 直接挑到value放弃状态 */
		else if (c == ';') *ind=MAX_LEN_KEY+MAX_LEN_VALUE;
		else key[*ind-1] = c;
		(*ind)++;
		return 1;
	}
	if (*ind > MAX_LEN_KEY+MAX_LEN_VALUE-1) {
		goto EXIT_KVPROCESS_AND_CLEANUP;
	}
	if (c != ';') {
		if (*ind >= MAX_LEN_KEY+MAX_LEN_VALUE-1) {
			// 超出范围放弃读取但保持读值状态不变
			value[0] = 0;
			return 1;
		}
		value[*ind-MAX_LEN_KEY-1] = c;
		(*ind)++;
		return 1;
	}

	char *subkey = strchr(key, '.');
	if (subkey) {
		*subkey = 0;
		subkey++;
	}
	if (key[0] == 0 || value[0] == 0) goto EXIT_KVPROCESS_AND_CLEANUP;

	if (strcmp(key, "track") == 0) {
		note->track = atoi(value);
	} else if (strcmp(key, "amp") == 0) {
		note->amplitude = atof(value);
		if(note->amplitude<0||note->amplitude>=0.8) note->amplitude=0.2;
	} else if (strcmp(key, "bq") == 0) {
		if (!note->biquad || (note->pNext && note->pNext->biquad == note->biquad)) {
			note->biquad = malloc(sizeof(*note->biquad));
			memset(note->biquad, 0, sizeof(*note->biquad));
		}
		biquad_set(note->biquad, subkey, value);
	} else if (strcmp(key, "note") == 0) {    /* NoteData_t辖区 */
		notedata_set(data, subkey, value);
	} else if (strcmp(key, "inst") == 0) {    /* 整体设置 */
		if (strcmp(value, "piano") == 0) {
			data->wave_func = sin;
			data->har_func = har_set_piano2;
			if (note->biquad) *note->biquad = (Biquad_t){};
		} else if (strcmp(value, "drum") == 0) {
			data->wave_func = sin;
			data->har_func = har_set_none;
			data->flo_freq_func = flo_inverse_liner;
			if (note->biquad) *note->biquad = (Biquad_t){};
		}
		/* 未完成 
		case INST_HIHAT:
		wave_func = WF_SQUARE;
		if (harmonics == HAR_NOSET) harmonics = HAR_NONE;
		if (!p->bq) p->bq = BQ_HP;
		double (*func)(double) = wave_funcs[wave_func];
		double f2 = f;
		for (int i = 1; i < 8; ++i) {
			value += har_amps[HAR_PIANO][i] * func(2 * M_PI * f2 * x);
			f2 += rand() % 50;
		}
		value = tanh(value);
		break;*/
	}
EXIT_KVPROCESS_AND_CLEANUP:
	*ind = 0;
	memset(key, 0, MAX_LEN_KEY);
	memset(value, 0, MAX_LEN_VALUE);
	return 1;
}

void note_free(Note_t *p)
{
	if (!p) return;
	NoteData_t *dp = p->pcm_data, *ldp;
	Note_t *lp;
	while (p) {
		lp=p;
		p=p->pNext;
		lp->pNext = NULL;
		lp->pcm_data = NULL;
		free(lp);
	}
	while (dp) {
		ldp=dp;
		dp=dp->pNext;
		if (ldp->pwav) free(ldp->pwav);
		ldp->pwav = NULL;
		ldp->pNext = NULL;
		free(ldp);
	}
	return;
}

bool note_get_nd(Note_t *pH, Note_t *p, NoteData_t *cur_notedata, NoteData_t *ndh)
{
	if (!pH || !p || !cur_notedata || !ndh) return false;
	/* 音符共享机制 */
	p->pcm_data = notedata_search(pH->pcm_data, cur_notedata);
	if (p->pcm_data) return true;
	/* 设置pcm_data */
	p->pcm_data = malloc(sizeof(*p->pcm_data));
	if (!p->pcm_data) {
		return false;
	}
	*p->pcm_data = *cur_notedata;
	p->pcm_data->pNext = NULL;

	if (ndh->pNext) ndh->pNext->pNext = p->pcm_data;
	ndh->pNext = p->pcm_data;
	return true;
}

/* 解读字符串形式的音符并产生解析好的音符串struct */
Note_t *note_parser(int (*stream)(void*), void *stream_ctx, double base_amplitude)
{
	static const double note_freq[256] = { 0,
		['c'] = 130.8, ['d'] = 146.8, ['e'] = 164.8, ['f'] = 174.6,
		['g'] = 196.0, ['a'] = 220.0, ['b'] = 246.9,
		['C'] = 261.6, ['D'] = 293.6, ['E'] = 329.6, ['F'] = 349.2,
		['G'] = 392.0, ['A'] = 440.0, ['B'] = 493.9,
		['1'] = 523.2, ['2'] = 587.3, ['3'] = 659.2, ['4'] = 698.5,
		['5'] = 784.0, ['6'] = 880.0, ['7'] = 987.8, ['0'] = -1,
		['{'] = 20, ['}'] = 20000};
	const double change_freq = exp2(1/12.0);    /* 跨半音用 */

	char key[MAX_LEN_KEY] = "",
	     value[MAX_LEN_VALUE] = "";
	double fade = 0;
	int setting_mode = 0;
	NoteData_t notedata = {
		.beates = 4,
		.notes = 4,
		.type = 4,
		.speed = 120,
		.wave_func = sin,
		.har_func = har_set_piano2,
		.flo_freq_func = NULL,
		.freq = 0,
		.pNext = NULL,
		.portamento_from = -1,
	}, cur_notedata = notedata;
	/* 顺便存储指针位置等 */
	Note_t note = {
		.line = 1,
		.ind = 0,
		.ch = 0,
		.pcm_data = NULL,
		.pNext = NULL,
		.flg_left = true,
		.flg_right = true,
		.adsr = (ADSR_t){
			.attack  = 0.005,
			.decay   = 0.3,
			.sustain = 0.7,
			.release = 0.2
		},
		.amplitude = base_amplitude,
	};
	Note_t *pH = NULL, *p = NULL;
	while (note.ch != EOF) {
		note.ch = stream(stream_ctx);
		if (note.ch==EOF) {
			if (p && !note_get_nd(pH, p, &cur_notedata, &notedata)) {
				note_free(pH);
				return NULL;
			}
			break;
		}
		if (note.ch=='\n') {
			note.ind = 0;
			note.line++;
		} else note.ind++;
		if (note.ch >= 256 || note.ch < 0) continue;

		if (setting_mode != 0 &&
		    process_key_value(note.ch, &setting_mode, key, value, &note, &notedata) > 0) {
			continue;
		}

		if (note_freq[note.ch]) {    /* 新的音符 */
			notedata.freq = note_freq[note.ch];
			if (p && cur_notedata.portamento_from > 0 && cur_notedata.freq <= 0) {
				/* 处理滑音 */
				cur_notedata.freq = notedata.freq;
				continue;
			}

			/* 旧节点pcm_data分配 */
			if (p && !note_get_nd(pH, p, &cur_notedata, &notedata)) {
				note_free(pH);
				return NULL;
			}

			/* 创建新节点 */
			note.pNext = malloc(sizeof(Note_t));
			if (!note.pNext) {
				note_free(pH);
				return NULL;
			}
			*note.pNext = note;
			if (!pH) pH = note.pNext;
			note.pNext->pcm_data = NULL;
			note.pNext->pNext = NULL;
			cur_notedata = notedata;

			/* 连接上下节点并交换 */
			if (p) {
				p->pNext = note.pNext;
				if (p->flg_legato) note.pNext->flg_be_legato = true;
				if (fade) note.pNext->amplitude = p->amplitude*(1.0+fade);
			}
			p = note.pNext;
			continue;
		} else if (note.ch == ':') {
			setting_mode = 1;
			continue;
		} else if (!p) continue;

		switch (note.ch) {
		case '/': cur_notedata.type *= 2; break;
		case '*': cur_notedata.type /= 2; break;
		case '.': cur_notedata.type /= (3.0/2.0); break;
		case '~': p->flg_legato = !p->flg_legato; break;
		case 's': cur_notedata.portamento_from=cur_notedata.freq, cur_notedata.freq=0; break;
		case '+': p->amplitude += p->amplitude + 0.1 <= 1 ? 0.1 : 0; break;
		case '-': p->amplitude -= p->amplitude - 0.1 >= 0 ? 0.1 : 0; break;
		case 'l': cur_notedata.freq/=change_freq; break;    /* 跨半音 */
		case 'u': cur_notedata.freq*=change_freq; break;
		case 'L': cur_notedata.freq/=2; break;              /* 跨八度 */
		case 'U': cur_notedata.freq*=2; break;
		case '[': p->flg_left = !p->flg_left; break;
		case ']': p->flg_right = !p->flg_left; break;
		case '<': fade += 0.005; break;
		case '>': fade -= 0.005; break;
		case '=': fade = 0; break;
		case ':': setting_mode = 1; break;
		}
	}
	if (pH) notedata_setlen(pH->pcm_data);
	return pH;
}
