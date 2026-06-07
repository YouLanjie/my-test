/**
 * @file        music_synth.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       音频合成器导出应用（原ALSA.c）
 */

#include "core.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

const char *NOTELIST[] = {
	"C C G G A A G*  F F E E D D C*\n"   /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
	"G G F F E E D*  G G F F E E D*\n"   /* 挂在天上放光明 */ /* 好像许多小眼睛 */
	"C C G G A A G*  F F E E D D C*",    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */

	":beates=2;"
	"C/ C/ C   g/ a/ g   E/. D// C/ a/  D*"
	"E/ E/ E   G/ G/ G   A/. G// C/ E/  D*"
	"E*        G*        E/. D// C/ D/  a*"
	"As1       G.sE/     C E            G*"
	"A/ 0/     G/ 0/     E/ 0/          G/ 0/"
	"1.++ A/ G/ 0/ E/ 0/"	/* 打败美帝 */
	"G/ 0/ D/ 0/ C 0"	/* 野心狼 */
	"C/ C/ C   g/ a/ g   E/. D// C/ a/  D*"
	"E/ E/ E   G/ G/ G   A/. G// C/ E/  D*"
	"E*        G*        E/. D// C/ D/  a*"
	"As1       G.sE/     C E            G*"
	"A/ 0/     G/ 0/     E/ 0/          G/ 0/"
	"1.++ A/ G/ 0/ E/ 0/"	/* 打败美帝 */
	"1/ 0/ G/ 0/ 1 0",	/* 野心狼 */

	"C C C C C C C C C C "
	"C C C C C C C C C C",    /* 10s音频测试 */

	"c d e f g a b "
	"C D E F G A B "
	"1 2 3 4 5 6 7",    /* 10.5s音频测试 */

	":beates=2;"
	"3/. 2// 1/ A/   G/ A// G// E/ G/   1/ 1// 1// 1/ 1/"
	"1 C// D// E// F//"    /* 前奏 */
	"G E/. D//   C/ E/ G/ 1/   3 2/. 3//   1*"
	"2 2/. 3//   2/ 1/ B/ A/   G A/. 1//   G*"
	"E/ E// E// D/ E/   G/ E// G// A/ G/   1 2   3*"
	"2/. 1// B/ A/   G/ A// G// E/ G/   1 1/ 1/   1"
	"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
	"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
	"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
	"B/ A/   G. E/   G/ G// A// B/ 1/   2*   2"    /* 我们竞技在那运动场 */
	"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
	"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
	"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
	"B/ A/   G. E/   G/ A/ B/ 2/   1*"    /* 我们竞技在那运动场 */
	"1 C// D// E// F//"
	/* REPEAT NEXT */
	"G E/. D//   C/ E/ G/ 1/   3 2/. 3//   1*"
	"2 2/. 3//   2/ 1/ B/ A/   G A/. 1//   G*"
	"E/ E// E// D/ E/   G/ E// G// A/ G/   1 2   3*"
	"2/. 1// B/ A/   G/ A// G// E/ G/   1 1/ 1/   1"
	"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
	"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
	"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
	"B/ A/   G. E/   G/ G// A// B/ 1/   2*   2"    /* 我们竞技在那运动场 */
	"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
	"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
	"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
	"B/ A/   G. E/   G/ A/ B/ 2/   1*"    /* 我们竞技在那运动场 */
	"1   0",    /* 运动员进行曲 */

	":beates=6;"
	":notes=8;"
	":speed=120;"
	"1/ 2/ 3/ 2/ 1/ A/ B/ A./ E// G.~G."
	"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."
	"G/ F/ E/ D. b/ a/ g/ E.  F.  D  C/  C.~C."
	/* 前奏over */
	"G/~A/ G/ F/~E/  D/  C. g."	/* 我和~我~的~祖~国~ */
	"C/ E/ 1/ B/ A./ E// G.~G."	/* 一刻也不能分割 */
	"A/ B/ A/ G/~F/  E/  D. a."	/* 无论我走到哪里 */
	"b/ a/ g/ G/ C./ D// E.~E."	/* 都流出一首赞歌 */
	"G/ A/ G/ F/ E/  D/  C. g."	/* 我歌唱每一座高山 */
	"C/ E/ 1/ B/ 2./ 1// A.~A."	/* 我歌唱每一条河 */
	"1/ B/ A/ G."			/* 袅袅炊烟 */
	"A/ G/ F/ E."			/* 小小村落 */
	"b  a/ g/~g/ D/ C.~C."		/* 路下一道辙 */
	"1/ 2/~3/ 2/ 1/ A/ B/~A./~E// G.~G."	/* 我最亲爱的祖~国~ */
	"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."	/* 我永远紧依着你的心窝 */
	"G/ F/ E/ D."			/* 你用你那 */
	"b/ a/ g/ E."			/* 母亲的脉搏 */
	"F. D  C/ C.~C 0/"		/* 和我诉说 */
	/* 重复上一段 */
	"G/~A/ G/ F/~E/  D/  C. g."	/* 我的~祖~国~和~我~ */
	"C/ E/ 1/ B/ A./ E// G.~G."	/* 像海和浪花一朵 */
	"A/ B/ A/ G/~F/  E/  D. a."	/* 浪是那海的赤子 */
	"b/ a/ g/ G/ C./ D// E.~E."	/* 海是那浪的依托 */
	"G/ A/ G/ F/ E/  D/  C. g."	/* 每当大海在微笑 */
	"C/ E/ 1/ B/ 2./ 1// A.~A."	/* 我就是笑的酒窝 */
	"1/ B/ A/ G."			/* 我分担着 */
	"A/ G/ F/ E."			/* 海的忧愁 */
	"b  a/ g/~g/ D/ C.~C."		/* 分享海的欢乐 */
	"1/ 2/~3/ 2/ 1/ A/ B/~A./~E// G.~G."	/* 我最亲爱的祖~国~ */
	"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."	/* 你是大海永不干涸 */
	"G/ F/ E/ D."			/* 永远给我 */
	"b/ a/ g/ E."			/* 碧浪清波 */
	"F. D  C/ C.~C 0/"		/* 心中的歌 */
	/* 结束句 */
	"1/ 2/ 3/ 2/ 1/ A/ B/~A./~E// G.~G."	/* 我最亲爱的祖~国~ */
	"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."	/* 你是大海永不干涸 */
	"G/ F/ E/ D."			/* 永远给我 */
	"b/ a/ g/ E."			/* 碧浪清波 */
	"G. 2  1/ 1.~1. 0/",		/* 心中的歌 */
	/* 我和我的祖国 */

	":beates=2;"
	"C./ E// G/ G/ A G E./ C// G/ G// G//"
	"E C g/ g// g// g/ g// g// C 0/"
	"g/ C. C/ C./ C// g/ a// b// C C  0/"
	"E/ C/ D// E// G G E./ E// C./ E// G./ E// D D*"
	"A G D E G/ E/ 0/ G/ E/ D// E// C E 0"
	"g./ a// C/ C/ E./ E// G/ G/ D/ D// D// a D."
	"g/  C."
	"C/ E."
	"E/ G*"
	"C./ E// G/ G/ A G"
	"E./ C// G/ G// G// E/ 0/  C/ 0/"
	"g C E./ C// G/ G// G// E/ 0/ C/ 0/"
	"g C g C g C C 0",
	/* 义勇军进行曲 */
};
/*
 * c d e f g a b
 * C D E F G A B
 * 1 2 3 4 5 6 7
 * */

int streamer_file(void *p)
{
	if (!p) return EOF;
	return getc(p);
}

int streamer_str(void *vp)
{
	char **p = vp;
	if (!p) return EOF;
	if (!*p) return EOF;
	if (**p == 0) return EOF;
	return *(*p)++;
}

int main(int argc, char *argv[])
{
	int ch = 0, id = -1;
	char filename[PATH_MAX] = "",
	     input[PATH_MAX] = "";
	bool flg_print_formated_note = false,
	     flg_no_check = false,
	     flg_check_only = false;
	MusicCtx_t *ctx = music_ctx_create(SAMPLE_RATE);
	if (!ctx) {
		LOG("乐曲上下文ctx创建失败");
		return 1;
	}

	while ((ch = getopt(argc, argv, "hi:I:o:nmHPpNA:")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: music_synth [Option]\n"
			       "Option:\n"
			       "    -i <NUM>  选择曲子\n"
			       "    -I <FILE> 输入文件(曲谱)\n"
			       "    -o <FILE> 输出文件(为空时使用output.wav)\n"
			       "    -n        取消音符淡入淡出,可能产生杂音\n"
			       "    -m        平滑滑音，滑音频率匀速增长\n"
			       "    -H        取消泛音\n"
			       "    -P        打印音符(格式化)\n"
			       "    -p        仅打印格式化后的音符\n"
			       "    -N        不检查音符合规性\n"
			       "    -A <NUM>  音量系数(默认1.0)\n"
			       "    -h        显示帮助\n"
			       "  NUM: 0: 小星星\n"
			       "       1: 中国人民志愿军战歌\n"
			       "       2: 20s音频测试\n"
			       "       3: 10.5升调音频测试\n"
			       "       4: 运动员进行曲\n"
			       "       5: 我和我的祖国\n"
			       "       6: 义勇军进行曲\n"
			       );
			return ch == '?' ? -1 : 0;
			break;
		case 'A':
			ctx->amplitude = strtof(optarg, NULL);
			if (ctx->amplitude > 100 || ctx->amplitude < 0) ctx->amplitude = 1.0;
			break;
		case 'o':
			if (optarg && *optarg) strncpy(filename, optarg, sizeof(filename));
			else strncpy(filename, "output.wav", sizeof(filename));
			break;
		case 'i': id = (int)strtod(optarg, NULL) % ARRAY_LEN(NOTELIST); break;
		case 'I': strncpy(input, optarg, sizeof(input)); break;
		case 'n': ctx->flg_no_fade = true; break;
		case 'm': ctx->flg_smooth = true; break;
		case 'H': ctx->flg_no_har = true; break;
		case 'p': flg_check_only = true;
		case 'P': flg_print_formated_note = true; break;
		case 'N': flg_no_check = true; break;
		// case 'x': ctx->flg_print_debug_info = true; break;
		default:
			break;
		}
	}

	if (*input) {
		FILE *fp = fopen(input, "r");
		if (!fp) {
			music_ctx_free(ctx);
			LOG("输入文件打开失败: %s", input);
			return 1;
		}
		ctx->notes = note_parser(streamer_file, fp);
		fclose(fp);
	} else if (id >= 0) {
		const char *p = NOTELIST[id];
		ctx->notes = note_parser(streamer_str, &p);
	}
	else ctx->notes = note_parser(streamer_file, stdin);

	if (!ctx->notes) {
		LOG("曲谱为空");
		music_ctx_free(ctx);
		return 1;
	}

	if (!flg_no_check) check_notes(ctx->notes, flg_print_formated_note);
	if (flg_check_only) {
		music_ctx_free(ctx);
		return 0;
	}
	size_t total_size = music_ctx_stat(ctx);

	WavHeader_t header = create_wav_header(total_size);
	FILE *wav_file = NULL;
	if (filename[0]) wav_file = fopen(filename, "wb");
	if (!wav_file) {
		if (filename[0]) fprintf(stderr, "[ERROR] 打开文件 '%s' 时遇到问题: %s\n", filename, strerror(errno));
		else LOG("未设置输出文件名");
		music_ctx_free(ctx);
		return 1;
	}
	printf("[INFO] 结果输出到 '%s'\n", filename);
	fwrite(&header, sizeof(header), 1, wav_file);

	clock_t start = clock(), now;
	size_t sum_size = 0, size = 0;
	if (!total_size) total_size = 1;
	while ((size = music_ctx_gen_pcm(ctx))) {
		music_ctx_pcm_to_i16(ctx);
		fwrite(ctx->pcmf16_buffer, sizeof(int16_t)*2, size, wav_file);
		sum_size += size;
		/* fprintf(stderr, "TS: %.2lfs, CS: %lu, pos: %.2lfs \r",
		       (double)sum_size/SAMPLE_RATE, size,
		       (double)ctx->position/SAMPLE_RATE); */
		if (!sum_size) continue;
		now = clock();
		fprintf(stderr, "[%-35.*s] %4.1lf%% (%.1fs/%.1fs) real %.0lfs ETA %.1lfs \r",
			(int)(35*sum_size/total_size),
			"########################################",
			(double)sum_size/total_size*100.,
			(double)sum_size/SAMPLE_RATE,
			(double)total_size/SAMPLE_RATE,
			(double)(now-start)/CLOCKS_PER_SEC,
			(double)(now-start)/CLOCKS_PER_SEC*(total_size-sum_size)/sum_size);
	}
	fprintf(stderr, "\n");

	if (sum_size != total_size) {
		printf("[INFO] 更新wav文件时长\n");
		fseek(wav_file, 0L, SEEK_SET);
		header = create_wav_header(sum_size);
		fwrite(&header, sizeof(header), 1, wav_file);
	}
	fclose(wav_file);
	printf("[INFO] 曲谱总时长: %lf秒\n", (double)sum_size/SAMPLE_RATE);
	music_ctx_free(ctx);
	return 0;
}
