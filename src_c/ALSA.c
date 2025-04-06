/*
 *   Copyright (C) 2025 Chglish
 *
 *   文件名称：ALSA.c
 *   创 建 者：Chglish
 *   创建日期：2025年04月05日
 *   描    述：用于测试ALSA，内谱有《小星星》一首
 *             资料借助deepseek，代码最初由其生成，由我修改
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include <string.h>

/* 采样率（Hz） */
unsigned int SAMPLE_RATE = 44100;

snd_pcm_t *init()
{
	snd_pcm_t *pcm_handle = NULL;
	int err;

	// 打开PCM设备("default")
	if ((err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0))) {
		fprintf(stderr, "无法打开PCM设备: %s\n", snd_strerror(err));
		return NULL;
	}

	// 设置硬件参数
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_hw_params_any(pcm_handle, hw_params);

	// 交错模式、16位有符号小端格式、双声道、44100Hz
	if ((err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED))
	    || (err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE))
	    || (err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 2))
	    || (err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &SAMPLE_RATE, 0))) {
		fprintf(stderr, "参数设置失败: %s\n", snd_strerror(err));
		snd_pcm_close(pcm_handle);
		return NULL;
	}

	// 应用参数
	if ((err = snd_pcm_hw_params(pcm_handle, hw_params))) {
		fprintf(stderr, "无法应用硬件参数: %s\n", snd_strerror(err));
		snd_pcm_close(pcm_handle);
		return NULL;
	}
	return pcm_handle;
}

// WAV文件头结构
typedef struct {
	char RIFF[4];		// "RIFF"
	uint32_t file_size;	// 文件总大小-8
	char WAVE[4];		// "WAVE"
	char fmt[4];		// "fmt "
	uint32_t fmt_size;	// fmt块大小（16）
	uint16_t audio_format;	// 1=PCM
	uint16_t channels;	// 声道数
	uint32_t sample_rate;
	uint32_t byte_rate;	// 每秒字节数
	uint16_t block_align;	// 每个样本的字节数
	uint16_t bits_per_sample;
	char data[4];		// "data"
	uint32_t data_size;	// 音频数据大小
} WavHeader;

void create_wav_header(WavHeader *header, float duration)
{
	memcpy(header->RIFF, "RIFF", 4);
	memcpy(header->WAVE, "WAVE", 4);
	memcpy(header->fmt, "fmt ", 4);
	memcpy(header->data, "data", 4);

	header->fmt_size = 16;
	header->audio_format = 1;	// PCM
	header->channels = 2;
	header->sample_rate = SAMPLE_RATE;
	header->bits_per_sample = 16;
	header->block_align = header->channels * header->bits_per_sample / 8;
	header->byte_rate = SAMPLE_RATE * header->block_align;

	int data_size = SAMPLE_RATE * duration * header->block_align;
	header->data_size = data_size;
	header->file_size = data_size + sizeof(WavHeader) - 8;
	/*printf("DATA_SIZE:%d\nFILE_SIZE:%d\nDELTER:%d\n",
	 * data_size,header->file_size,header->file_size-data_size);*/
}

/* 
 * 生成固定声波
 * volume: 0% ~ 100%
 * 数学模型: f(x) = A*sin(ω*x)
 * */
double wave(double x, double volume, double f)
{
	const short max_vol = (unsigned short)(-1) / 2;
	return (volume * max_vol) * sin(f * (2*M_PI) * x);
}

/* 
 * note: C D E F G A B
 * type: n分音符
 * base: 以n分音符为一拍
 * speed: 速度，n拍/分钟
 * */
int *create_note_wave(const char *note, int type, int base, int speed)
{
	double slide = 0, slide_d = 0, slide_s = 0, amplitude = 0.5;
	if (note == NULL) return NULL;
	type = type <= 0 ? 4 : type;
	base = base <= 0 ? 4 : base;
	speed = speed <= 0 ? 60 : speed;

	for (const char *p = note; *p != '\0'; p++) {
		if (*p == '/')
			type *= 2;
		else if (*p == '*')
			type /= 2;
		else if (*p == '.')
			type *= (3/2);
		else if (*p == '~')
			slide = 1;
		else if (*p == '+')
			amplitude += amplitude + 0.1 <= 1 ? 0.1 : 0;
		else if (*p == '-')
			amplitude -= amplitude - 0.1 >= 0 ? 0.1 : 0;
	}
	const unsigned int buffer_frames = SAMPLE_RATE*((float)base/type)*(60/(float)speed);
	int *buffer = malloc(sizeof(short)*(buffer_frames * 2 + 2));    /* 双声道 */
	memset(buffer, 0, sizeof(short)*(buffer_frames * 2 + 2));
	buffer[0] = buffer_frames;
	for (int i = 0; i < buffer_frames; i++) {    /* 生成每个采样点的声波 */
		const double note_freq[256] = { 0,
			['c'] = 130.8, ['d'] = 146.8, ['e'] = 164.8, ['f'] = 174.6,
			['g'] = 196.0, ['a'] = 220.0, ['b'] = 246.9,
			['C'] = 261.6, ['D'] = 293.6, ['E'] = 329.6, ['F'] = 349.2,
			['G'] = 392.0, ['A'] = 440.0, ['B'] = 493.9,
			['1'] = 523.2, ['2'] = 587.3, ['3'] = 659.2, ['4'] = 698.5,
			['5'] = 784.0, ['6'] = 880.0, ['7'] = 987.8};
		short result = 0, result2 = 0;
		short *num = &result;
		for (const char *p = note; *p != '\0'; p++) {
			if (*p == 'r')
				num = &result2;
			else if (*p == ',')
				break;
			if (slide == 1) slide = note_freq[(int)*p];
			else if (slide && !slide_s) slide_s = (note_freq[(int)*p] - slide) / (buffer_frames - i);
			if (slide_s) {
				slide_d += slide_s;
				*num = (short)wave((float)i / SAMPLE_RATE, amplitude, slide+slide_d);
			} else
				*num += (short)wave((float)i / SAMPLE_RATE, amplitude, note_freq[(int)*p]);
		}
		((short*)buffer)[i * 2 + 2] = result;    /* 左声道 */
		((short*)buffer)[i * 2 + 3] = result2 ? result2 : result;    /* 右声道 */
	}
	return buffer;
}

int play_wav(snd_pcm_t *pcm_handle, short *wave, int size) {
	if (!pcm_handle)
		return 1;
	// 写入音频数据
	int frames_written = snd_pcm_writei(pcm_handle, wave, size);
	if (frames_written < 0) frames_written = snd_pcm_recover(pcm_handle, frames_written, 0);
	if (frames_written < 0) {
		fprintf(stderr, "写入错误: %s\n", snd_strerror(frames_written));
		return 1;
	}
	return 0;
}


int melody(int id, char *filename, int stat, int speed)
{
	id %= 5;
	const char *note[5][514] = {
		{
			"C","C","G","G","A","A","G*", "F","F","E","E","D","D","C*",    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
			"G","G","F","F","E","E","D*", "G","G","F","F","E","E","D*",    /* 挂在天上放光明 */ /* 好像许多小眼睛 */
			"C","C","G","G","A","A","G*", "F","F","E","E","D","D","C*",    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
		}, {
			"C/","C/","C",     "g/","a/","g",     "E/.","D//","C/","a/", "D*",
			"E/","E/","E",     "G/","G/","G",     "A/.","G//","C/","E/", "D*",
			"E*",              "G*",              "E/.","D//","C/","D/", "a*",
			"A/","A1~","1/",   "G/.","GE~","E//", "C","E",               "G*",
			"A","G",           "E","G",           "1.++","A/",       "G","E",
			"G/","/","D/","/", "C","* ",
			"C/","C/","C",     "g/","a/","g",     "E/.","D//","C/","a/", "D*",
			"E/","E/","E",     "G/","G/","G",     "A/.","G//","C/","E/", "D*",
			"E*",              "G*",              "E/.","D//","C/","D/", "a*",
			"A/","A1~","1/",   "G/.","GE~","E//", "C","E",               "G*",
			"A","G",           "E","G",           "1.++","A/",       "G","E",
			"C/","/","G/","/", "1"," "
		}, {
			"C","C","C","C","C","C","C","C","C","C",
			"C","C","C","C","C","C","C","C","C","C",    /* 10s音频测试 */
		}, {
			"c","d","e","f","g","a","b",
			"C","D","E","F","G","A","B",
			"1","2","3","4","5","6","7",    /* 10.5s音频测试 */
		}, {
			"3/.","2//","1/","A/", "G/","A//","G//","E/","G/", "1/","1//","1//","1/","1/", "1","C//","D//","E//","F//",
			"G","E/.","D//", "C/","E/","G/","1/", "3","2/.","3//", "1*", "2","2/.","3//", "2/","1/","B/","A/",
			"G","A/.","1//", "G*", "E/","E//","E//","D/","E/", "G/","E//","G//","A/","G/", "1","2", "3*",
			"2/.","1//","B/","A/", "G/","A//","G/","E/","G/", "1","1/","1/", "1","G/.","A//", "E", "G", "1/","1/","2/","1/",
			"G*", "G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*", "3","3/","5/", "4.","3/", "2/","B/","A/","G//","G//",
			"3.","2/", "1","B/","A/", "G.","E/", "G/","G//","A//","B/","1/", "2*", "2","G/.","A//", "E","G",
			"1/","1/","2/","1/", "G*", "G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*", "3","3/","5/", "4.","3/",
			"2/", "B/","A/","G//","G//", "3.","2//", "1","B/","A/", "G.","E/", "G/","A/","B/","2/",
			"1*", "1","C//","D//","E//","F//",    /* Specifit */
			/* REPEAT NEXT */
			"G","E/.","D//", "C/","E/","G/","1/", "3","2/.","3//", "1*", "2","2/.","3//", "2/","1/","B/","A/",
			"G","A/.","1//", "G*", "E/","E//","E//","D/","E/", "G/","E//","G//","A/","G/", "1","2", "3*",
			"2/.","1//","B/","A/", "G/","A//","G/","E/","G/", "1","1/","1/", "1","G/.","A//", "E", "G", "1/","1/","2/","1/",
			"G*", "G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*", "3","3/","5/", "4.","3/", "2/","B/","A/","G//","G//",
			"3.","2/", "1","B/","A/", "G.","E/", "G/","G//","A//","B/","1/", "2*", "2","G/.","A//", "E","G",
			"1/","1/","2/","1/", "G*", "G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*", "3","3/","5/", "4.","3/",
			"2/", "B/","A/","G//","G//", "3.","2//", "1","B/","A/", "G.","E/", "G/","A/","B/","2/",
			"1*", "1",    /* 运动员进行曲 */
		},
	};
	/*
	 * 1 2 3 4 5 6 7
	 * C D E F G A B
	 * */

	FILE *wav_file;
	WavHeader wav_header;
	double size = 0;
	if (stat & (1 << 1)) {
		wav_file =  fopen(filename, "wb");
		 /* 创建并写入WAV文件头 */
		if (!wav_file)
			fprintf(stderr, "打开文件不成: %s\n", filename);
		create_wav_header(&wav_header, size);
		fwrite(&wav_header, 1, sizeof(wav_header), wav_file);
	}

	snd_pcm_t *pcm_handle = NULL;
	if (stat & 1) {
		// ALSA PCM设备配置
		pcm_handle = init();
		if (! pcm_handle) return 1;
	}

	for (int i = 0; note[id][i] != NULL; i++) {
		int *wave = create_note_wave(note[id][i], 4, 4, speed);
		if (!wave || wave[0] <= 10) {
			printf("波形为空,Code:[%d]\n", wave ? wave[0] : -1);
			continue;
		}
		size+=(float)wave[0]/SAMPLE_RATE;
		if (stat & (1 << 1)) fwrite(wave+1, sizeof(int), *wave, wav_file);
		if (stat & 1) play_wav(pcm_handle, (short*)wave+1, *wave);
		free(wave);
	}
	if (stat & (1 << 1)) {
		fseek(wav_file, 0, SEEK_SET);
		create_wav_header(&wav_header, size);	// 重新生成准确的头信息
		fwrite(&wav_header, 1, sizeof(wav_header), wav_file);
		fclose(wav_file);
	}

	if (stat & 1) {
		// 等待播放完成并关闭设备
		snd_pcm_drain(pcm_handle);
		snd_pcm_close(pcm_handle);
	}
	return 0;
}

int read_play_wave(char *filename)
{
	if (!filename) return -1;
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "打开文件失败:%s\n", filename);
		return 1;
	}

	char *content = NULL, *p = NULL;
	char RIFF[5] = {0}, WAVE[5] = {0}, data[5] = {0};
	fseek(fp, 0L, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	content = malloc(sizeof(int)*size);
	fread(content, size, 1, fp);
	fclose(fp);

	WavHeader header;
	memcpy(&header, content, sizeof(WavHeader));
	memcpy(RIFF, header.RIFF, 4);
	memcpy(WAVE, header.WAVE, 4);
	memcpy(data, header.data, 4);
	if (strcmp(RIFF, "RIFF") || strcmp(WAVE, "WAVE")) {
		fprintf(stderr, "这看起来不像是一个wav文件[思考]\n");
		return 3;
	}
	SAMPLE_RATE = header.sample_rate;

	/* 寻找data段 */
	p = content+sizeof(WavHeader) - 8;
	while((strcmp(data, "data") || *(p-1) != 0) && p != content+size) {
		p++;
		memcpy(data, p, 4);
	}
	memcpy(&header.data_size, p+4, 4);

	printf("channels:%d\nsample_rate:%d\nbyte_rate:%d\nblock_align:%d\n"
	       "bits_per_sample:%d\ndata_size:%d\nfile_size:%d\n",
	       header.channels, header.sample_rate, header.byte_rate, header.block_align,
	       header.bits_per_sample, header.data_size, header.file_size);

	snd_pcm_t *pcm_handle = init();
	if (! pcm_handle) return 2;

	play_wav(pcm_handle, (short*)(p+8), header.data_size);

	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);

	free(content);
	return 0;
}

int main(int argc, char *argv[])
{
	int ch = 0, id = 0, stat = 0, speed = 120;
	char filename[125] = "output.wav";
	while ((ch = getopt(argc, argv, "hi:pso:S:r:")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: ALSA [-hips] [-o <file>]\n"
			       "Option:\n"
			       "    -i <NUM>  选择曲子\n"
			       "    -p        播放曲子\n"
			       "    -s        保存曲子(单设-o无用)\n"
			       "    -o <FILE> 输出文件(output.wav)\n"
			       "    -S <NUM>  设置曲速(120)\n"
			       "    -r <FILE> 输入文件（启用后其他选项无效）\n"
			       "    -h        显示帮助\n"
			       "  NUM: 0: 小星星\n"
			       "       1: 中国人民志愿军战歌\n"
			       "       2: 20s音频测试\n"
			       "       3: 10.5升调音频测试\n"
			       "       4: 运动员进行曲\n"
			       );
			return ch == '?' ? -1 : 0;
			break;
		case 'i':
			id = strtod(optarg, NULL);
			break;
		case 'p':
			stat |= 1;
			break;
		case 's':
			stat |= 1<<1;
			break;
		case 'o':
			strcpy(filename, optarg);
			break;
		case 'S':
			speed = strtod(optarg, NULL);
			break;
		case 'r':
			strcpy(filename, optarg);
			return read_play_wave(filename);
			break;
		default:
			break;
		}
	}
	speed = speed <= 2 ? 120 : speed;
	melody(id, filename, stat, speed);
	return 0;
}
