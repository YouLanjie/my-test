#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdcountof.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include "../include/target_list.h"

// #define EXPERIMENT_FFMPEG_AI_CODE

#ifndef EXPERIMENT_FFMPEG_AI_CODE
#include <fcntl.h>
#include <stdlib.h>
// #include <errno.h>
#else
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>


/* 选像素格式：优先 YUV420P，否则取编码器支持的第一个 */
static enum AVPixelFormat pick_pix_fmt(AVCodecContext *ctx)
{
    const enum AVPixelFormat *fmts = NULL;
    if (avcodec_get_supported_config(ctx, NULL,
                                     AV_CODEC_CONFIG_PIX_FORMAT,
                                     0, (const void **)&fmts, NULL) < 0 || !fmts)
        return AV_PIX_FMT_YUV420P;

    for (const enum AVPixelFormat *p = fmts; *p != AV_PIX_FMT_NONE; p++)
        if (*p == AV_PIX_FMT_YUV420P) return *p;

    return fmts[0];
}

/* 选采样格式：优先 S16，否则取编码器支持的第一个 */
static enum AVSampleFormat pick_sample_fmt(AVCodecContext *ctx)
{
    const enum AVSampleFormat *fmts = NULL;
    if (avcodec_get_supported_config(ctx, NULL,
                                     AV_CODEC_CONFIG_SAMPLE_FORMAT,
                                     0, (const void **)&fmts, NULL) < 0 || !fmts)
        return AV_SAMPLE_FMT_S16;

    for (const enum AVSampleFormat *p = fmts; *p != AV_SAMPLE_FMT_NONE; p++)
        if (*p == AV_SAMPLE_FMT_S16) return *p;

    return fmts[0];
}

// av_err2str: ???
#define fferr(err) av_make_error_string(errmsg_buf, sizeof(errmsg_buf), err)
static bool transcode_av(const char *infile, const char *outfile)
{
	bool ok = false;

	AVFormatContext *ifmt = NULL, *ofmt = NULL;
	AVCodecContext  *vdec = NULL, *adec = NULL;
	AVCodecContext  *venc = NULL, *aenc = NULL;
	struct SwsContext *sws = NULL;
	struct SwrContext *swr = NULL;
	AVFrame  *vfrm = NULL, *vfrm_out = NULL;
	AVFrame  *afrm = NULL, *afrm_out = NULL;
	AVPacket *pkt = NULL;

	int vidx_in = -1, aidx_in = -1;
	int vidx_out = -1, aidx_out = -1;
	int ret;

	static char errmsg_buf[AV_ERROR_MAX_STRING_SIZE];

	/* ---- 1. 打开输入 ---- */
	if ((ret = avformat_open_input(&ifmt, infile, NULL, NULL)) < 0) {
		fprintf(stderr, "avformat_open_input: %s\n", fferr(ret));
		goto done;
	}
	if ((ret = avformat_find_stream_info(ifmt, NULL)) < 0) {
		fprintf(stderr, "avformat_find_stream_info: %s\n", fferr(ret));
		goto done;
	}

	vidx_in = av_find_best_stream(ifmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	aidx_in = av_find_best_stream(ifmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (vidx_in < 0 && aidx_in < 0) {
		fprintf(stderr, "no audio/video stream in '%s'\n", infile);
		goto done;
	}

	/* ---- 2. 输出上下文（让 libavformat 根据文件名猜容器） ---- */
	if ((ret = avformat_alloc_output_context2(&ofmt, NULL, NULL, outfile)) < 0
	    || !ofmt) {
		fprintf(stderr, "avformat_alloc_output_context2: %s\n", fferr(ret));
		goto done;
	}

	/* 3gp 的专用参数（仅在需要重编码时使用） */
	bool is_3gp = strstr(ofmt->oformat->name, "3gp") != NULL;

	/* ---- 3. 判断每个流是 copy 还是转码 ---- */
	bool copy_v = false, copy_a = false;

	if (vidx_in >= 0) {
		enum AVCodecID cid = ifmt->streams[vidx_in]->codecpar->codec_id;
		copy_v = avformat_query_codec(ofmt->oformat, cid,
		                              FF_COMPLIANCE_NORMAL) > 0;
	}
	if (aidx_in >= 0) {
		enum AVCodecID cid = ifmt->streams[aidx_in]->codecpar->codec_id;
		copy_a = avformat_query_codec(ofmt->oformat, cid,
		                              FF_COMPLIANCE_NORMAL) > 0;
	}

	fprintf(stderr, "plan: video %s, audio %s\n",
	        vidx_in < 0 ? "none" : (copy_v ? "copy" : "transcode"),
	        aidx_in < 0 ? "none" : (copy_a ? "copy" : "transcode"));

	/* ---- 4. 打开解码器（只对需要转码的流） ---- */
	if (vidx_in >= 0 && !copy_v) {
		AVStream *st = ifmt->streams[vidx_in];
		const AVCodec *c = avcodec_find_decoder(st->codecpar->codec_id);
		if (!c) { fprintf(stderr, "video decoder not found\n"); goto done; }
		vdec = avcodec_alloc_context3(c);
		avcodec_parameters_to_context(vdec, st->codecpar);
		if ((ret = avcodec_open2(vdec, c, NULL)) < 0) {
			fprintf(stderr, "open video decoder: %s\n", fferr(ret));
			goto done;
		}
	}
	if (aidx_in >= 0 && !copy_a) {
		AVStream *st = ifmt->streams[aidx_in];
		const AVCodec *c = avcodec_find_decoder(st->codecpar->codec_id);
		if (!c) { fprintf(stderr, "audio decoder not found\n"); goto done; }
		adec = avcodec_alloc_context3(c);
		avcodec_parameters_to_context(adec, st->codecpar);
		if ((ret = avcodec_open2(adec, c, NULL)) < 0) {
			fprintf(stderr, "open audio decoder: %s\n", fferr(ret));
			goto done;
		}
	}

	/* ---- 5. 视频输出流：copy 或 建立编码器 ---- */
	if (vidx_in >= 0) {
		if (copy_v) {
			AVStream *ost = avformat_new_stream(ofmt, NULL);
			if (!ost) goto done;
			avcodec_parameters_copy(ost->codecpar,
			                        ifmt->streams[vidx_in]->codecpar);
			ost->codecpar->codec_tag = 0;   /* 让 muxer 决定 */
			ost->time_base           = ifmt->streams[vidx_in]->time_base;
			vidx_out = ost->index;
		} else {
			enum AVCodecID vcodec_hint = ofmt->oformat->video_codec;
			if (vcodec_hint == AV_CODEC_ID_NONE) {
				enum AVCodecID ic =
					ifmt->streams[vidx_in]->codecpar->codec_id;
				if (avcodec_find_encoder(ic)) vcodec_hint = ic;
			}
			if (vcodec_hint == AV_CODEC_ID_NONE) vcodec_hint = AV_CODEC_ID_H264;

			const AVCodec *c = avcodec_find_encoder(vcodec_hint);
			if (!c) {
				fprintf(stderr, "video encoder %d not found\n", vcodec_hint);
				goto done;
			}
			venc = avcodec_alloc_context3(c);
			if (!venc) {
				fprintf(stderr, "video encoder %d context faild to create\n", vcodec_hint);
				goto done;
			}

			if (is_3gp) {
				venc->bit_rate  = 400000;
				venc->width     = 352;
				venc->height    = 288;
				venc->time_base = (AVRational){ 1, 12 };
				venc->framerate = (AVRational){ 12, 1 };
				venc->gop_size  = 12;
			} else {
				venc->bit_rate  = vdec->bit_rate > 0 ? vdec->bit_rate : 800000;
				venc->width     = vdec->width;
				venc->height    = vdec->height;
				venc->time_base = (AVRational){ 1, 25 };
				venc->framerate = (AVRational){ 25, 1 };
				venc->gop_size  = 12;
			}
			venc->pix_fmt             = pick_pix_fmt(venc);
			venc->sample_aspect_ratio = vdec->sample_aspect_ratio;

			if (ofmt->oformat->flags & AVFMT_GLOBALHEADER)
				venc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if ((ret = avcodec_open2(venc, c, NULL)) < 0) {
				fprintf(stderr, "open video encoder: %s\n", fferr(ret));
				goto done;
			}

			AVStream *ost = avformat_new_stream(ofmt, NULL);
			if (!ost) goto done;
			avcodec_parameters_from_context(ost->codecpar, venc);
			ost->time_base = venc->time_base;
			vidx_out = ost->index;
		}
	}

	/* ---- 6. 音频输出流：copy 或 建立编码器 ---- */
	if (aidx_in >= 0) {
		if (copy_a) {
			AVStream *ost = avformat_new_stream(ofmt, NULL);
			if (!ost) goto done;
			avcodec_parameters_copy(ost->codecpar,
			                        ifmt->streams[aidx_in]->codecpar);
			ost->codecpar->codec_tag = 0;
			ost->time_base           = ifmt->streams[aidx_in]->time_base;
			aidx_out = ost->index;
		} else {
			enum AVCodecID acodec_hint = ofmt->oformat->audio_codec;
			if (acodec_hint == AV_CODEC_ID_NONE) {
				enum AVCodecID ic =
					ifmt->streams[aidx_in]->codecpar->codec_id;
				if (avcodec_find_encoder(ic)) acodec_hint = ic;
			}
			if (acodec_hint == AV_CODEC_ID_NONE) acodec_hint = AV_CODEC_ID_AAC;

			const AVCodec *c = avcodec_find_encoder(acodec_hint);
			if (!c) {
				fprintf(stderr, "audio encoder %d not found\n", acodec_hint);
				goto done;
			}
			aenc = avcodec_alloc_context3(c);
			if (!aenc) {
				fprintf(stderr, "audio encoder %d context faild to create\n", acodec_hint);
				goto done;
			}

			if (is_3gp) {
				aenc->bit_rate    = 12200;
				aenc->sample_rate = 8000;
				av_channel_layout_default(&aenc->ch_layout, 1);
			} else {
				aenc->bit_rate    = adec->bit_rate > 0 ? adec->bit_rate : 128000;
				aenc->sample_rate = adec->sample_rate;
				if (adec->ch_layout.nb_channels > 0)
					av_channel_layout_copy(&aenc->ch_layout, &adec->ch_layout);
				else
					av_channel_layout_default(&aenc->ch_layout, 2);
			}
			aenc->sample_fmt = pick_sample_fmt(aenc);
			aenc->time_base  = (AVRational){ 1, aenc->sample_rate };

			if (ofmt->oformat->flags & AVFMT_GLOBALHEADER)
				aenc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if ((ret = avcodec_open2(aenc, c, NULL)) < 0) {
				fprintf(stderr, "open audio encoder: %s\n", fferr(ret));
				goto done;
			}

			AVStream *ost = avformat_new_stream(ofmt, NULL);
			if (!ost) goto done;
			avcodec_parameters_from_context(ost->codecpar, aenc);
			ost->time_base = aenc->time_base;
			aidx_out = ost->index;
		}
	}

	/* ---- 7. 打开输出、写头 ---- */
	if (!(ofmt->oformat->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&ofmt->pb, outfile, AVIO_FLAG_WRITE)) < 0) {
			fprintf(stderr, "avio_open: %s\n", fferr(ret));
			goto done;
		}
	}
	if ((ret = avformat_write_header(ofmt, NULL)) < 0) {
		fprintf(stderr, "avformat_write_header: %s\n", fferr(ret));
		goto done;
	}

	/* ---- 8. 准备缓冲 ---- */
	pkt = av_packet_alloc();
	if (!pkt) goto done;
	if (!copy_v) {
		vfrm     = av_frame_alloc();
		vfrm_out = av_frame_alloc();
		if (!vfrm || !vfrm_out) goto done;
	}
	if (!copy_a) {
		afrm     = av_frame_alloc();
		afrm_out = av_frame_alloc();
		if (!afrm || !afrm_out) goto done;
	}

	int64_t vpts_out = 0, apts_out = 0;

	/* ---- 9. 主循环 ---- */
	while ((ret = av_read_frame(ifmt, pkt)) >= 0) {

		/* ============ 视频 ============ */
		if (pkt->stream_index == vidx_in && venc) {
			if (copy_v) {
				pkt->stream_index = vidx_out;
				av_packet_rescale_ts(pkt,
					ifmt->streams[vidx_in]->time_base,
					ofmt->streams[vidx_out]->time_base);
				pkt->pos = -1;
				if (av_interleaved_write_frame(ofmt, pkt) < 0) {
					av_packet_unref(pkt);
					goto done;
				}
				av_packet_unref(pkt);
			} else {
				if (avcodec_send_packet(vdec, pkt) < 0) {
					av_packet_unref(pkt);
					continue;
				}
				av_packet_unref(pkt);

				while ((ret = avcodec_receive_frame(vdec, vfrm)) == 0) {
					if (!sws) {
						sws = sws_getContext(
							vfrm->width, vfrm->height, vfrm->format,
							venc->width, venc->height, venc->pix_fmt,
							SWS_BILINEAR, NULL, NULL, NULL);
						if (!sws) {
							fprintf(stderr, "sws_getContext failed\n");
							goto done;
						}
						vfrm_out->format = venc->pix_fmt;
						vfrm_out->width  = venc->width;
						vfrm_out->height = venc->height;
						if (av_frame_get_buffer(vfrm_out, 32) < 0) goto done;
					}
					if (av_frame_make_writable(vfrm_out) < 0) goto done;
					sws_scale(sws,
						(const uint8_t * const *)vfrm->data,
						vfrm->linesize,
						0, vfrm->height,
						vfrm_out->data, vfrm_out->linesize);

					vfrm_out->pts = vpts_out++;
					if (avcodec_send_frame(venc, vfrm_out) < 0) goto done;
					while (avcodec_receive_packet(venc, pkt) == 0) {
						pkt->stream_index = vidx_out;
						av_packet_rescale_ts(pkt,
							venc->time_base,
							ofmt->streams[vidx_out]->time_base);
						if (av_interleaved_write_frame(ofmt, pkt) < 0) {
							av_packet_unref(pkt);
							goto done;
						}
						av_packet_unref(pkt);
					}
					av_frame_unref(vfrm);
				}
			}
		}
		/* ============ 音频 ============ */
		else if (pkt->stream_index == aidx_in && aenc) {
			if (copy_a) {
				pkt->stream_index = aidx_out;
				av_packet_rescale_ts(pkt,
					ifmt->streams[aidx_in]->time_base,
					ofmt->streams[aidx_out]->time_base);
				pkt->pos = -1;
				if (av_interleaved_write_frame(ofmt, pkt) < 0) {
					av_packet_unref(pkt);
					goto done;
				}
				av_packet_unref(pkt);
			} else {
				if (avcodec_send_packet(adec, pkt) < 0) {
					av_packet_unref(pkt);
					continue;
				}
				av_packet_unref(pkt);

				while ((ret = avcodec_receive_frame(adec, afrm)) == 0) {
					if (!swr) {
						if (swr_alloc_set_opts2(&swr,
							&aenc->ch_layout, aenc->sample_fmt, aenc->sample_rate,
							&afrm->ch_layout, afrm->format, afrm->sample_rate,
							0, NULL) < 0 || !swr || swr_init(swr) < 0) {
							fprintf(stderr, "swr init failed\n");
							goto done;
						}
						afrm_out->format      = aenc->sample_fmt;
						afrm_out->sample_rate = aenc->sample_rate;
						av_channel_layout_copy(&afrm_out->ch_layout,
						                       &aenc->ch_layout);
						afrm_out->nb_samples  = aenc->frame_size > 0
							? aenc->frame_size : afrm->nb_samples;
						if (av_frame_get_buffer(afrm_out, 0) < 0) goto done;
					}

					int out_max = (int)av_rescale_rnd(
						swr_get_delay(swr, afrm->sample_rate) + afrm->nb_samples,
						aenc->sample_rate, afrm->sample_rate, AV_ROUND_UP);

					if (out_max > afrm_out->nb_samples) {
						av_frame_unref(afrm_out);
						afrm_out->format      = aenc->sample_fmt;
						afrm_out->sample_rate = aenc->sample_rate;
						av_channel_layout_copy(&afrm_out->ch_layout,
						                       &aenc->ch_layout);
						afrm_out->nb_samples  = out_max;
						if (av_frame_get_buffer(afrm_out, 0) < 0) goto done;
					}
					if (av_frame_make_writable(afrm_out) < 0) goto done;

					int got = swr_convert(swr,
						afrm_out->data, out_max,
						(const uint8_t **)afrm->data, afrm->nb_samples);
					if (got <= 0) { av_frame_unref(afrm); continue; }

					afrm_out->nb_samples = got;
					afrm_out->pts        = apts_out;
					apts_out            += got;

					if (avcodec_send_frame(aenc, afrm_out) < 0) goto done;
					while (avcodec_receive_packet(aenc, pkt) == 0) {
						pkt->stream_index = aidx_out;
						av_packet_rescale_ts(pkt,
							aenc->time_base,
							ofmt->streams[aidx_out]->time_base);
						if (av_interleaved_write_frame(ofmt, pkt) < 0) {
							av_packet_unref(pkt);
							goto done;
						}
						av_packet_unref(pkt);
					}
					av_frame_unref(afrm);
				}
			}
		}
		else {
			av_packet_unref(pkt);
		}
	}

	/* ---- 10. flush 编码器（copy 流无需 flush） ---- */
	if (venc) {
		avcodec_send_frame(venc, NULL);
		while (avcodec_receive_packet(venc, pkt) == 0) {
			pkt->stream_index = vidx_out;
			av_packet_rescale_ts(pkt, venc->time_base,
				ofmt->streams[vidx_out]->time_base);
			av_interleaved_write_frame(ofmt, pkt);
			av_packet_unref(pkt);
		}
	}
	if (aenc) {
		avcodec_send_frame(aenc, NULL);
		while (avcodec_receive_packet(aenc, pkt) == 0) {
			pkt->stream_index = aidx_out;
			av_packet_rescale_ts(pkt, aenc->time_base,
				ofmt->streams[aidx_out]->time_base);
			av_interleaved_write_frame(ofmt, pkt);
			av_packet_unref(pkt);
		}
	}

	av_write_trailer(ofmt);
	ok = true;

done:
	av_packet_free(&pkt);
	av_frame_free(&vfrm);
	av_frame_free(&afrm);
	av_frame_free(&vfrm_out);
	av_frame_free(&afrm_out);
	sws_freeContext(sws);
	swr_free(&swr);
	avcodec_free_context(&vdec);
	avcodec_free_context(&adec);
	avcodec_free_context(&venc);
	avcodec_free_context(&aenc);
	if (ofmt) {
		if (!(ofmt->oformat->flags & AVFMT_NOFILE) && ofmt->pb)
			avio_closep(&ofmt->pb);
		avformat_free_context(ofmt);
	}
	if (ifmt) avformat_close_input(&ifmt);
	return ok;
}
#undef fferr
#endif


static int read_ffmpeg_output(Target_t *target, int pid, int subout, int suberr)
{
	if (!target || subout < 0 || suberr < 0) return -1;
	struct pollfd fds[2] = {
		{ .fd = subout, .events = POLLIN, },
		{ .fd = suberr, .events = POLLIN, },
	};
	char buffer[2*PATH_MAX];
	/* 单位：微秒μs */
	uint64_t duration_total = 0,
		 duration_now = 0;
	bool active = true;
	int ret = 0;
	while (active && (waitpid(pid, &ret, WNOHANG) > 0 ? !WIFEXITED(ret) : true)
	       && poll(fds, countof(fds), 1e3) != -1) {
		for (size_t i = 0; i < countof(fds); i++) {
			if (!(fds[i].revents & POLLIN)) continue;
			ssize_t size = read(fds[i].fd, buffer, sizeof(buffer));
			if (i == 1) {
				/* STDERR转日志 */
				sva_append(&target->log, (SV_t){.p=buffer,.len=size});
				continue;
			}
			if ((size_t)size >= sizeof(buffer)) buffer[--size] = 0;
			SV_t line = {}, left = (SV_t){.p=buffer,.len=size};
			char c = 0;
			while (sv_forline(&line, &left)) {
				if (sscanf(line.p, "progress=en%c", &c) == 1 && c == 'd') {
					active = false;
					break;
				}
				if (sscanf(line.p, "out_time_us=%lu", &duration_now) < 1) continue;
				target->progress = (double)duration_now/duration_total;
			}
		}
		if (duration_total) continue;
		SV_t line = {}, left = sv_from_sva(&target->log);
		int h = 0, m = 0, s = 0, cs = 0;
		for (int i = 0; i < 50 && sv_forline(&line, &left); i++) {
			if (sscanf(line.p, " Duration: %d:%2d:%2d.%2d,", &h, &m, &s, &cs) < 4) continue;
			duration_total = (h*3600+m*60+s)*1000000L+cs*10000L;
			break;
		}
	}
	return WIFEXITED(ret) ? WEXITSTATUS(ret) : -2;
}

static SVA_t *argv2str(SVA_t *dest, char *argv[])
{
	if (!dest || !argv) return NULL;
	SVA_t tmp = {};
	for (size_t idx = 0; argv[idx]; idx++) {
		if (idx) sva_append(dest, sv_from_lstr(" "));
		if (!strpbrk(argv[idx], " \t\r\n\\\"'`!?#$%^&|(){}[]<>~") && strlen(argv[idx]) != 0) {
			sva_sprintfcat(dest, "%s", argv[idx]);
			continue;
		}
		sva_from_cstr(&tmp, argv[idx]);
		sva_replace(&tmp, sv_from_lstr("'"), sv_from_lstr("'\\''"));
		sva_sprintfcat(dest, "'%.*s'", (int)tmp.len, tmp.p);
	}
	sva_free(&tmp);
	return dest;
}

static bool build_ffmpeg(Target_t *target)
{
	if (!target || !target->depend_len
	    || !target->dependencies || !target->dependencies[0]) return false;
#ifdef EXPERIMENT_FFMPEG_AI_CODE
	return transcode_av(target->dependencies[0]->name.p, target->name.p);
#else
	SV_t name = sv_from_sva(&target->name);

	char *argv1[] = {
		"ffmpeg", "-hide_banner", "-y", "-progress", "pipe:1", "-nostats",
		"-i", target->dependencies[0]->name.p,
		target->name.p,
		NULL,
	};
	/* 3gp参数 */
	char *argv2[] = {
		"ffmpeg", "-hide_banner", "-y", "-progress", "pipe:1", "-nostats",
		"-i", target->dependencies[0]->name.p,
		"-r", "12", "-b:v", "400k", "-s", "352x288",
		"-ab", "12.2k", "-ac", "1", "-ar", "8000",
		target->name.p,
		NULL,
	};
	char **argv = sv_case_end_with(name, sv_from_lstr(".3gp"))
		? argv2 : argv1;

	/* STDOUT 0:读端 1:写端 | STDERR 2:读端 3:写端 */
	int pipefd[4] = {-1, -1, -1, -1};
	int ret = false;
	if (pipe(pipefd) == -1) {
		perror("pipe#1");
		goto EXIT_THREAD_AND_CLEANUP;
	}
	if (pipe(pipefd+2) == -1) {
		perror("pipe#2");
		goto EXIT_THREAD_AND_CLEANUP;
	}

	SVA_t command = {};
	argv2str(&command, argv);
	printf("[\e[32mRUN\e[0m] %s\n", command.p);

	pid_t pid = fork();
	if (!pid) {
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[3], STDERR_FILENO);
		for (size_t i = 0; i < countof(pipefd); i++) close(pipefd[i]);
		execvp("ffmpeg", argv);
		perror("execvp");
		exit(1);
	}
	close(pipefd[1]);
	pipefd[1] = -1;
	close(pipefd[3]);
	pipefd[3] = -1;

	ret = read_ffmpeg_output(target, pid, pipefd[0], pipefd[2]);
	// waitpid(pid, &ret, 0);
	// ret = WIFEXITED(ret) && WEXITSTATUS(ret) == 0;
	if (ret != 0) {
		sva_sprintfcat(&target->log, "\n[INFO] 退出状态：code %d\n", ret);
		sva_sprintfcat(&target->log, "[COMMAND] %.*s\n", (int)command.len, command.p);
		printf("[\e[31mFAILD\e[0m] %s\n", target->name.p);
		remove(target->name.p);
		ret = false;
	} else ret = true;
	sva_free(&command);

EXIT_THREAD_AND_CLEANUP:
	for (size_t i = 0; i < countof(pipefd); i++) {
		if (pipefd[i] >= 0) close(pipefd[i]);
	}
	return ret;
#endif
}


static bool rule_fordir(SV_t d_name, uint8_t d_type)
{
	if (!d_name.p || !d_name.len) return false;
	/* 跳过特殊路径 */
	if (sv_cmp(d_name, sv_from_cstr("..")) == 0 ||
	    sv_cmp(d_name, sv_from_cstr(".")) == 0)
		return false;
	/* 跳过文件夹 */
	if (d_type == DT_DIR) return false;
	/* 跳过非文件 */
	if (d_type != DT_REG && d_type != DT_LNK)
		return false;
	if (!path_suffixname(d_name).len) return false;
	return true;
}

static SV_t output_type = {};
static Path_t output_dir = {};

static Target_t *action_file(Target_t *list, SV_t full_path)
{
	if (!output_type.len || !output_dir.len) return list;
	Target_t *target_src, *target_output;

	Path_t tmp = {};
	sva_from_sv(&tmp, full_path);
	path_normalize(&tmp);
	if (!path_get_st_follow(sv_from_sva(&tmp)).isfile) {
		sva_free(&tmp);
		return NULL;
	}
	target_src = target_get_or_create(list, sv_from_sva(&tmp));
	if (target_src) target_src->type = TY_DEP;
	if (!list) list = target_src;

	SV_t stem = path_stemname(full_path);
	if (!stem.len) stem = path_basename(full_path);
	sva_sprintfcat(path_join(sva_from_sva(&tmp, &output_dir), stem), ".%.*s", (int)output_type.len, output_type.p);
	target_output = target_get_or_create(list, sv_from_sva(&tmp));
	if (target_output) target_output->build = build_ffmpeg;
	target_depend_append(target_output, target_src);
	sva_free(&tmp);
	return list;
}

int main(int argc, char *argv[])
{
	SV_t exe_name = path_basename(sv_from_cstr(argv[0]));
	SV_t inputdir = {};
	int opt;
	bool print_list = false;
	int proc_limit = sysconf(_SC_NPROCESSORS_ONLN) / 2;
	while ((opt = getopt(argc, argv, "ht:d:l:p")) != -1) {
		switch (opt) {
		case 't':
			output_type = path_basename(sv_from_cstr(optarg));
			break;
		case 'd':
			inputdir = sv_from_cstr(optarg);
			break;
		case 'l':
			sscanf(optarg, "%d", &proc_limit);
			if (proc_limit <= 0) proc_limit = 1;
			break;
		case 'p':
			print_list = true;
			break;
		case '?':
		case 'h':
		default:
			printf("本程序基于ffmpeg，转换格式时需要安装ffmpeg\n"
			       "Usage: %.*s [OPTIONS]\n"
			       "OPTIONS:\n"
			       "    -t <FMT>  指定目标格式\n"
			       "    -d <PATH> 输入文件夹\n"
			       "    -l <NUM>  设置任务并行上限\n"
			       "    -p        结束后打印任务列表\n"
			       "    -h        打印帮助\n",
			       (int)exe_name.len, exe_name.p);
			return 0;
			break;
		}
	}
	if (!inputdir.len || !output_type.len) {
		printf("[!] 未指定输入文件夹或输出文件类型\n");
		return 1;
	}
	path_join(path_from_sv(&output_dir, inputdir), sv_from_lstr("out/"));
	path_mkdir(sv_from_sva(&output_dir), 0777);
	Target_t *list = NULL;
	list = target_fordir(list, NULL, inputdir, rule_fordir, action_file);

	printf("[INFO] 并行上限: %d\n", proc_limit);
	if (proc_limit > 1) target_buildlist_for_pthread(list, proc_limit, true);
	else target_buildlist(list);

	int completed = 0, faild = 0;
	for (Target_t *p = list; p; p = p->next) {
		if (p->type != TY_NORM) continue;
		if (p->status == TS_SUCCESS) completed++;
		else if (p->status == TS_FAILD) faild++;
	}
	printf("所有任务执行完成，%d 成功，%d 失败\n", completed, faild);
	if (print_list) target_printlist(list, 0b110);
	else target_printlist(list, 0);
	target_freelist(list);
	sva_free(&output_dir);
	return 0;
}

