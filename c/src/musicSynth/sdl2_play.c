/**
 * @file        sdl2_play.c
 * @author      u0_a221
 * @date        2026-06-08
 * @brief       使用sdl2作为前端的播放器(ai生成)
 */

#include <SDL2/SDL.h>
#include "../../include/tools.h"
#include "core.h"

// 全局或静态的播放上下文（也可以在回调中通过 userdata 传递）
typedef struct {
	MusicCtx_t *music;      // 音乐生成器
	int paused;             // 1=暂停, 0=播放
	size_t total_frames;    // 总采样帧数（用于进度显示）
} PlaybackCtx;

// 音频回调函数
// SDL2 要求填充 stream 共 len 字节（len = frames * channels * sample_size）
void audio_callback(void *userdata, Uint8 *stream, int len) {
	PlaybackCtx *ctx = (PlaybackCtx*)userdata;
	int16_t *out = (int16_t*)stream;
	int frames_requested = len / (2 * sizeof(int16_t));  // 双声道, 16位
	int frames_done = 0;

	if (ctx->paused) {
		// 暂停时输出静音
		memset(stream, 0, len);
		return;
	}

	// 生成足够的数据填满请求的缓冲区
	while (frames_done < frames_requested) {
		size_t avail = music_ctx_gen_pcm(ctx->music);
		if (avail == 0) {
			// 播放结束，填充剩余静音
			memset(out + frames_done * 2, 0,
			   (frames_requested - frames_done) * 2 * sizeof(int16_t));
			return;
		}

		music_ctx_pcm_to_i16(ctx->music);   // 将内部 double 缓冲区转为 int16_t
		int16_t *src = ctx->music->pcmf16_buffer;
		size_t copy = (avail < (size_t)(frames_requested - frames_done)) ?
		      avail : (frames_requested - frames_done);

		// 应用音量并复制
		for (size_t i = 0; i < copy * 2; i++) {  // *2 因为双声道
			out[frames_done*2 + i] = src[i];
		}

		frames_done += copy;
		if (avail > copy) {
		// 理论上 music_ctx_gen_pcm 每次应生成固定大小的 buffer，
		// 如果一次生成多了，需要保留剩余部分。为简化示例，假设一次生成不超过请求量。
		// 实际可维护一个内部偏移量。
		}
	}
}

// 初始化 SDL2 音频设备
SDL_AudioDeviceID init_sdl_audio(PlaybackCtx *ctx) {
	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = SAMPLE_RATE;
	want.format = AUDIO_S16SYS;  // 系统端序的16位有符号
	want.channels = 2;
	want.samples = ctx->music->buffer_len;    // 缓冲区大小（帧数），影响延迟
	want.callback = audio_callback;
	want.userdata = ctx;

	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (dev == 0) {
		LOG("打开音频设备失败: %s\n", SDL_GetError());
		return 0;
	}

	// 可选：检查实际获得的参数是否与期望一致
	if (have.format != want.format || have.freq != want.freq || have.channels != want.channels) {
		LOG("警告：音频格式与请求不完全匹配\n");
	}
	return dev;
}

// 控制函数（线程安全）
void sdl_pause(SDL_AudioDeviceID dev, PlaybackCtx *ctx, int pause) {
	SDL_LockAudioDevice(dev);
	ctx->paused = pause;
	SDL_UnlockAudioDevice(dev);
}

void sdl_seek(SDL_AudioDeviceID dev, PlaybackCtx *ctx, size_t new_position) {
	SDL_LockAudioDevice(dev);
	ctx->music->position = new_position;
	music_ctx_tracks_reset(ctx->music);
	SDL_UnlockAudioDevice(dev);
}

void sdl_set_volume(SDL_AudioDeviceID dev, PlaybackCtx *ctx, double dvol) {
	SDL_LockAudioDevice(dev);
	double vol = ctx->music->amplitude + dvol;
	ctx->music->amplitude = (vol < 0.0f) ? 0.0f : vol;
	SDL_UnlockAudioDevice(dev);
}

int streamer_file(void *p)
{
	if (!p) return EOF;
	return getc(p);
}

int main(int argc, char *argv[]) {
	int ch = 0;
	char input[PATH_MAX] = "";
	bool flg_print_formated_note = false, flg_no_check = false;
	MusicCtx_t *music = music_ctx_create(SAMPLE_RATE/20);
	if (!music) {
		LOG("乐曲上下文ctx创建失败");
		return 1;
	}

	while ((ch = getopt(argc, argv, "hi:I:nmHPNA:")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: alsa_play [Option]\n"
			       "Option:\n"
			       "    -i <FILE>   输入文件(曲谱)\n"
			       "    -n          取消音符淡入淡出,可能产生杂音\n"
			       "    -m          平滑滑音，滑音频率匀速增长\n"
			       "    -H          取消泛音\n"
			       "    -P          打印音符(格式化)\n"
			       "    -N          不检查音符合规性\n"
			       "    -A <NUM>    音量系数(默认1.0)\n"
			       "    -h          显示帮助\n"
			       );
			return ch == '?' ? -1 : 0;
			break;
		case 'A':
			music->amplitude = strtof(optarg, NULL);
			if (music->amplitude > 100 || music->amplitude < 0) music->amplitude = 1.0;
			break;
		case 'I':
		case 'i': strncpy(input, optarg, sizeof(input)); break;
		case 'n': music->flg_no_fade = true; break;
		case 'm': music->flg_smooth = true; break;
		case 'H': music->flg_no_har = true; break;
		case 'P': flg_print_formated_note = true; break;
		case 'N': flg_no_check = true; break;
		}
	}
	if (!*input || !isatty(STDIN_FILENO)) {
		music_ctx_free(music);
		return 0;
	}

	FILE *fp = fopen(input, "r");
	if (!fp) {
		music_ctx_free(music);
		return 1;
	}
	music->notes = note_parser(streamer_file, fp);
	fclose(fp);

	if (!flg_no_check) check_notes(music->notes, flg_print_formated_note);
	PlaybackCtx ctx = {
		.music = music,
		.paused = 0,
		.total_frames = music_ctx_stat(music),
	};

	SDL_AudioDeviceID dev = init_sdl_audio(&ctx);
	if (dev == 0) {
		music_ctx_free(music);
		return 1;
	}

	// 开始播放
	SDL_PauseAudioDevice(dev, 0);  // 0 表示取消暂停，开始播放

	printf("[NOTICE] 该程序播放可能存在bug,请谨慎使用\n");
	// 控制循环（沿用原来的 kbhit + getchar 方式）
	int c = 0;
	int total_sec = (double)ctx.total_frames / SAMPLE_RATE;
	int now_sec = 0;
	do {
		c = kbhitGetchar();
		if (c == 'Q' || c == 'q') break;
		if (c == 0x1b && (c = kbhitGetchar()) == '[' && (c = kbhitGetchar()) >= 0 && c < 124) {
			c = (char[124]){ ['A']='k', ['B']='j', ['C']='l', ['D']='h'}[c];
		}
		if (c == ' ') {
			sdl_pause(dev, &ctx, !ctx.paused);
		} else if (c == 'j' || c == 'l' || c == 'k' || c == 'h' || c == '0') {
			size_t newpos = music->position;
			size_t step = (c == 'j' || c == 'k') ? SAMPLE_RATE*10 : SAMPLE_RATE;
			int8_t direction = (c == 'h' || c == 'k') ? -1 : 1;
			if (direction > 0 ? newpos+step <= ctx.total_frames : newpos >= step+0)
				newpos += step*direction;
			if (c == '0') newpos = 0;
			sdl_seek(dev, &ctx, newpos);
		} else if (c == '=') { sdl_set_volume(dev, &ctx, +0.005f);
		} else if (c == '-') { sdl_set_volume(dev, &ctx, -0.005f);
		} else if (c == '+') { sdl_set_volume(dev, &ctx, +0.1f);
		} else if (c == '_') { sdl_set_volume(dev, &ctx, -0.1f);
		} else if (c == '\\') {
			sdl_set_volume(dev, &ctx, 1-music->amplitude);
		} else if (c == 'i') {
			printf("\n[INFO] 当前位置参考:\n");
			print_note(music->tracks[0]);
		}

		// 显示进度
		now_sec = (double)music->position / SAMPLE_RATE;
		fprintf(stderr, "\r[%-40.*s] %4.1lf%% (%02d:%02d/%02d:%02d) Vol:%.1f%% \r",
			(int)(40 * music->position / ctx.total_frames),
			"##################################################",
			(double)music->position / ctx.total_frames * 100.,
			now_sec/60,
			now_sec % 60,
			total_sec/60,
			total_sec % 60,
			music->amplitude*100);

		usleep(1e6*music->buffer_len/SAMPLE_RATE/20);	// 避免 CPU 空转
	} while (music->position < ctx.total_frames);  // 或者判断回调是否已结束

	fprintf(stderr, "\n[Quit]\n");
	// 等待播放真正完成（回调中若返回 paComplete 语义，但 SDL2 不会自动停止设备）
	SDL_Delay(500);  // 简单等待，更严谨做法是检查音乐生成状态
	SDL_CloseAudioDevice(dev);
	SDL_Quit();

	music_ctx_free(music);
	return 0;
}
