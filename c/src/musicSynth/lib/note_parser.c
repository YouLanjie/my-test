/**
 * @file        note_parser.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       音符解释器
 */

#include "../core.h"

/* 打印音符 */
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
		note->flg_be_legato, note->flg_portamento,
		note->line, note->ind, note->ch);
}

/* 自动遍历整个链表检查合拍情况 */
void check_notes(Note_t *note, bool print)
{
	if (!note) return;
	char *colors[2][2] = { {"", ""}, {"\e[31m", "\e[0m"} };
	int istty = isatty(STDERR_FILENO);
	int istty2 = isatty(STDOUT_FILENO);
	double counter = 0;
	int track = 0;
#define p (note->pcm_data)
	for (; note && p ; note = note->next) {
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
			else if (note->pcm_data && note->flg_portamento)
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

static void notedata_set(NoteData_t *p, char *key, char *value, struct StringCtx_t ctx)
{
	if (!p || !value || !key || !*key) return;
	int possible = -1;
	double f = atof(value);

	ctx.desc="NOTE.SUBKEY";
	switch (str_switch2(key, &possible, &ctx,
		"speed", "wave_func", "har", "flo",
		"beates", "notes", "type",)) {
	case 0: p->speed = f  > 0 ? f : 120; break;
	case 4: p->beates = f > 0 ? f : 4; break;
	case 5: p->notes = f > 0 ? f : 4; break;
	case 6: p->type = f > 0 ? f : 4; break;
	case 3: if (strcmp(value, "drum")) p->flo_freq_func = flo_inverse_liner; break;
	case 2: {
#define HARMONIC(x) har_set_##x,
		size_t (*har_funcs[])(const Harmonics_t **) = {HARMONICS_LIST};
#undef HARMONIC
		possible = -1;
		ctx.desc = "HARMONIC";
#define HARMONIC(x) #x,
		int ind = str_switch2(value, &possible, &ctx, HARMONICS_LIST);
#undef HARMONIC
		if (ind >= 0 && ind < (int)ARRAY_LEN(har_funcs)) p->har_func = har_funcs[ind];
		break;
	} case 1:
		ctx.desc = "WAVE_FUNC";
		possible = -1;
		switch (str_switch2(value, &possible, &ctx,
			"sin", "square", "triangle", "sawtooth", "noise")) {
		case 0: p->wave_func = sin; break;
		case 1: p->wave_func = wave_square; break;
		case 2: p->wave_func = wave_triangle; break;
		case 3: p->wave_func = wave_sawtooth; break;
		case 4: p->wave_func = wave_noise;  break;
		}
		break;
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
	while (p && notedata_cmp(p, obj) == false) p = p->next;
	return p;
}

/* 设置 1=C 那档子事 */
static void set_note_freq_table(double note_freq[UINT8_MAX], char *pattern, struct StringCtx_t ctx)
{
	if (!note_freq || !pattern || !pattern[0]) return;
	static const double freq_a4 = 440.0;
	static const char *note_chr[] = {
		"c d ef g a bC D EF G A B1 2 34 5 6 7",
		"c de f ga b C DE F GA B 1 23 4 56 7 "
	};
	enum mode_t {MAJOR, MINOR} mode = MAJOR;
	const int note_chr_len = strlen(note_chr[MAJOR]);
	const int len_pattern = strlen(pattern);
	int i = 0;      /* 内存索引 */
	int idx = 0;    /* 音符索引(计算) */
	char c = 0;

	do {
		// 先找出调号
		char *p = NULL;
		for (; i < len_pattern && (!p || *p==' '); i++) p = strchr(note_chr[MAJOR], pattern[i]);
		if (!p) {
			printf("[WARN] (%d:%d) '%s'值无效,调号设置错误\n", ctx.line, ctx.col, ctx.desc);
			return;
		}
		c = *p;
	}while (0);

	for (; i < len_pattern; i++) {
		switch (pattern[i]) {
		case 'u': idx+=1;break;
		case 'U': idx+=12;break;
		case 'l': idx-=1;break;
		case 'L': idx-=12;break;
		case '^': mode=MAJOR;break;
		case 'v': mode=MINOR;break;
		}
	}

	const int idx_a4 = strchr(note_chr[mode], 'A') - note_chr[mode];
	const int idx_c4 = strchr(note_chr[mode], 'C') - note_chr[mode];
	idx += strchr(note_chr[mode], c) - note_chr[mode];
	for (i = 0; i < note_chr_len; i++) {
		if (note_chr[mode][i] == ' ') continue;
		note_freq[(int)note_chr[mode][i]] = freq_a4*exp2((idx-idx_a4+(int)i-idx_c4)/12.0);
	}
	// 特殊音符处理
	note_freq['{'] = 20;
	note_freq['}'] = 20000;
	note_freq['0'] = -1;
	return;
}

#define MAX_LEN_KEY 80
#define MAX_LEN_VALUE 80

static int process_key_value(char c, int *ind, char *key, char *value, Note_t *note, NoteData_t *data, double *note_freq)
{
	if (!ind || !key || !value || !note || !data) return -1;
	if (*ind == 0) return 0;

	if (*ind <= MAX_LEN_KEY) {
		if (c == '=') *ind = MAX_LEN_KEY;
		else if (*ind==MAX_LEN_KEY-1)
			*ind = MAX_LEN_KEY+MAX_LEN_VALUE-2;    /* 直接挑到value放弃状态 */
		else if (c == ';') goto EXIT_AND_CLEAN_UP_KEYVALUE_STATUE;
		else key[*ind-1] = c;
		(*ind)++;
		return 1;
	}
	if (*ind == MAX_LEN_KEY+MAX_LEN_VALUE-1) {
		if (c == ';') goto EXIT_AND_CLEAN_UP_KEYVALUE_STATUE;
		// 超出范围放弃读取但保持读值状态不变
		return 1;
	}
	if (*ind > MAX_LEN_KEY+MAX_LEN_VALUE-1)
		goto EXIT_AND_CLEAN_UP_KEYVALUE_STATUE;
	if (c != ';') {
		value[*ind-MAX_LEN_KEY-1] = c;
		(*ind)++;
		return 1;
	}

	char *subkey = strchr(key, '.');
	if (subkey) {
		*subkey = 0;
		subkey++;
	}

	struct StringCtx_t ctx = {.ch=note->ch, .line=note->line, .col=note->ind, .desc="MAINKEY"};
	int possible = -1;
	switch (str_switch2(key, &possible, &ctx,
		"track", "amp", "bq", "note", "inst", "key_name")) {
	case 0: note->track = atoi(value); break;
	case 3: notedata_set(data, subkey, value, ctx);break;
	case 5: ctx.desc = "key_name",set_note_freq_table(note_freq, value, ctx); break;
	case 1:
		note->amplitude = atof(value);
		if(note->amplitude<0||note->amplitude>=0.8) note->amplitude=0.2;
		break;
	case 2:
		if (!note->biquad || (note->prev && note->prev->biquad == note->biquad)) {
			note->biquad = malloc(sizeof(*note->biquad));
			memset(note->biquad, 0, sizeof(*note->biquad));
		}
		biquad_set(note->biquad, subkey, value, ctx);
		break;
	case 4:
		possible = -1;
		ctx.desc = "INSTRUMENT";
		switch (str_switch2(value, &possible, &ctx,
			"piano", "piano2", "piano3", "drum", "hi-hat")) {
		case 0:
			data->wave_func = sin;
			data->har_func = har_set_piano1;
			note->adsr = (ADSR_t){
				.attack  = 0.01,
				.decay   = 0.3,
				.sustain = 0.7,
				.release = 0.2
			};
			if (note->biquad) *note->biquad = (Biquad_t){};
			break;
		case 1:
			data->wave_func = sin;
			data->har_func = har_set_piano2;
			if (note->biquad) *note->biquad = (Biquad_t){};
			break;
		case 2:
			data->wave_func = sin;
			data->har_func = har_set_piano3;
			note->adsr = (ADSR_t){
				.attack  = 0.005,
				.decay   = 0.3,
				.sustain = 0.7,
				.release = 0.2
			};
			if (note->biquad) *note->biquad = (Biquad_t){};
			break;
		case 3:
			data->wave_func = sin;
			data->har_func = har_set_none;
			data->flo_freq_func = flo_inverse_liner;
			if (note->biquad) *note->biquad = (Biquad_t){};
			break;
		case 4:
			data->wave_func = wave_noise;
			data->har_func = har_set_openhat;
			note->adsr = (ADSR_t){
				.attack  = 0.001,
				.decay   = 1.0,
				.sustain = 1.0,
				.release = 0.0
			};
			break;
		}
		break;
	}
EXIT_AND_CLEAN_UP_KEYVALUE_STATUE:
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
		p=p->next;
		lp->prev = NULL;
		lp->next = NULL;
		lp->pcm_data = NULL;
		free(lp);
	}
	while (dp) {
		ldp=dp;
		dp=dp->next;
		if (ldp->pwav) free(ldp->pwav);
		ldp->pwav = NULL;
		ldp->next = NULL;
		free(ldp);
	}
	return;
}

static bool note_get_nd(Note_t *pH, Note_t *p, NoteData_t *cur_notedata, NoteData_t *ndh)
{
	if (!pH || !p || !cur_notedata || !ndh) return false;
	/* 音符共享机制 */
	p->pcm_data = notedata_search(pH->pcm_data, cur_notedata);
	if (p->pcm_data) {
		p->pcm_data->ref_count++;
		return true;
	}
	/* 设置pcm_data */
	p->pcm_data = malloc(sizeof(*p->pcm_data));
	if (!p->pcm_data) {
		return false;
	}
	*p->pcm_data = *cur_notedata;
	p->pcm_data->next = NULL;

	if (ndh->next) ndh->next->next = p->pcm_data;
	ndh->next = p->pcm_data;
	return true;
}

/* 解读字符串形式的音符并产生解析好的音符串struct */
Note_t *note_parser(int (*stream)(void*), void *stream_ctx)
{
	// 大调 C #C D #D E F #F G #G A #A B
	const double freq_half_note = exp2(1/12.0);    /* 跨半音用 */
	double note_freq[UINT8_MAX] = {0};
	set_note_freq_table(note_freq, "C", (struct StringCtx_t){.col=-1,.line=-1});    /* 默认使用1=C4 */

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
		.har_func = har_set_piano1,
		.flo_freq_func = NULL,
		.freq = 0,
		.next = NULL,
	}, cur_notedata = notedata;
	/* 顺便存储指针位置等 */
	Note_t note = {
		.line = 1,
		.ind = 0,
		.ch = 0,
		.pcm_data = NULL,
		.prev = NULL,
		.next = NULL,
		.flg_left = true,
		.flg_right = true,
		.adsr = (ADSR_t){
			.attack  = 0.01,
			.decay   = 0.3,
			.sustain = 0.7,
			.release = 0.2
		},
		.amplitude = 0.2,
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
		    process_key_value(note.ch, &setting_mode, key, value, &note, &notedata, note_freq) > 0) {
			continue;
		}

		if (note_freq[note.ch]) {    /* 新的音符 */
			notedata.freq = note_freq[note.ch];
			/* 旧节点pcm_data分配 */
			if (p && !note_get_nd(pH, p, &cur_notedata, &notedata)) {
				note_free(pH);
				return NULL;
			}

			/* 创建新节点 */
			note.next = malloc(sizeof(Note_t));
			if (!note.next) {
				note_free(pH);
				return NULL;
			}
			*note.next = note;
			if (!pH) pH = note.next;
			note.next->pcm_data = NULL;
			note.next->prev = p;
			note.next->next = NULL;
			cur_notedata = notedata;

			/* 连接上下节点并交换 */
			if (p) {
				p->next = note.next;
				if (p->flg_legato) note.next->flg_be_legato = true;
				if (p->flg_portamento) note.next->flg_be_portam = true;
				if (fade) note.next->amplitude = p->amplitude*(1.0+fade);
			}
			p = note.next;
			note.next = NULL;
			continue;
		} else if (note.ch == ':') {
			setting_mode = 1;
			continue;
		} else if (!p) continue;

		switch (note.ch) {
		case '/': cur_notedata.type *= 2; break;
		case '*': cur_notedata.type /= 2; break;
		case '.': cur_notedata.type /= (3.0/2.0); break;
		case '~': p->flg_legato^=1; break;
		case 's': p->flg_portamento^=1; break;
		case '+': p->amplitude += p->amplitude + 0.1 <= 1 ? 0.1 : 0; break;
		case '-': p->amplitude -= p->amplitude - 0.1 >= 0 ? 0.1 : 0; break;
		case 'l': cur_notedata.freq/=freq_half_note; break;    /* 跨半音 */
		case 'u': cur_notedata.freq*=freq_half_note; break;
		case 'L': cur_notedata.freq/=2; break;              /* 跨八度 */
		case 'U': cur_notedata.freq*=2; break;
		case '[': p->flg_left^=1; break;
		case ']': p->flg_right^=1; break;
		case '<': fade += 0.005; break;
		case '>': fade -= 0.005; break;
		case '=': fade = 0; break;
		case ':': setting_mode = 1; break;
		}
	}
	if (pH) notedata_setlen(pH->pcm_data);
	return pH;
}
