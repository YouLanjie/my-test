/*
 *   Copyright (C) 2026 u0_a221
 *
 *   文件名称：libav_test.c
 *   创 建 者：u0_a221
 *   创建日期：2026年02月20日
 *   描    述：
 *
 */


#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void process_audio_frame(AVFrame *frame)
{
	SwrContext *swr = swr_alloc();
	if (!swr) {
		return;
		// 处理错误
	}
	// 设置输入参数（从 AVFrame 获取）
	av_opt_set_chlayout(swr, "in_chlayout", &frame->ch_layout, 0);
	av_opt_set_int(swr, "in_sample_rate", frame->sample_rate, 0);
	av_opt_set_sample_fmt(swr, "in_sample_fmt", frame->format, 0);

	// 设置输出参数（目标格式）
	AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;	// 示例：立体声
	int out_sample_rate = frame->sample_rate;	// 保持原采样率，也可修改
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;

	av_opt_set_chlayout(swr, "out_chlayout", &out_ch_layout, 0);
	av_opt_set_int(swr, "out_sample_rate", out_sample_rate, 0);
	av_opt_set_sample_fmt(swr, "out_sample_fmt", out_sample_fmt, 0);

	// 初始化转换器
	if (swr_init(swr) < 0) {
		// 初始化失败
		return;
	}

	// 计算输出所需缓冲区大小（按最大可能样本数分配）
	int out_nb_samples = frame->nb_samples;	// 通常转换后样本数相同，但重采样后会变
	uint8_t *out_buffer = NULL;
	int out_linesize;
	av_samples_alloc(&out_buffer, &out_linesize,
			 out_ch_layout.nb_channels, out_nb_samples, out_sample_fmt, 0);

	// 执行转换
	int converted = swr_convert(swr, &out_buffer, out_nb_samples,
				    (const uint8_t **)frame->data, frame->nb_samples);
	if (converted < 0) {
		// 转换错误
		return;
	}
	// 现在 out_buffer 中存放着交错格式的 PCM 数据（假设目标为交错）
	// 可写入文件或进行其他处理

	/* audio data */
	int channels = frame->ch_layout.nb_channels;
	int nb_samples = frame->nb_samples;
	int16_t *data = (int16_t *) frame->data[0];	// 交错数据

	int16_t sample = 0;
	double total_phase_sq[16] = {0};
	// 遍历所有样本
	for (int i = 0; i < nb_samples; i++) {
		/*printf("SAMPLE[%d]: ", i);*/
		for (int ch = 0; ch < channels; ch++) {
			sample = data[i * channels + ch];
			if (ch >= 16) break;
			// 处理样本（例如写入文件、播放等）
			total_phase_sq[ch] += ((double)sample/INT16_MAX)*((double)sample/INT16_MAX);
			/*printf("[%d]:%7d", ch, sample);*/
		}
		/*printf("\n");*/
	}
	static long count = 0;
	printf("%ld", count);
	/*fprintf(stderr, "AVERAGE PHASE: ");*/
	for (int i = 0; i < 16 && i < channels; i++) {
		total_phase_sq[i] = sqrt(total_phase_sq[i]/nb_samples);
		/*fprintf(stderr, "[%d]", i);*/
		printf(",%-10lf", 20 * log10(total_phase_sq[i]));
	}
	printf("\n");
	count++;

	// 使用完毕后释放
	av_freep(&out_buffer);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
		return 1;
	}

	int ret;
	AVFormatContext *fmt_ctx = NULL;

	// 1. 打开文件
	if ((ret = avformat_open_input(&fmt_ctx, argv[1], NULL, NULL)) < 0) {
		fprintf(stderr, "Could not open input file: %s\n",
			av_err2str(ret));
		return 1;
	}
	// 2. 获取流信息
	if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
		fprintf(stderr, "Could not find stream info: %s\n",
			av_err2str(ret));
		avformat_close_input(&fmt_ctx);
		ret = 1;
		goto EXIT_CLOSE_INPUT; 
	}

	int audio_stream_idx = -1;	// 初始化为 -1，表示未找到
	for (int i = 0; i < fmt_ctx->nb_streams; i++) {
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_idx = i;
			break;		// 找到第一个视频流后退出循环
		}
	}
	if (audio_stream_idx == -1) {
		fprintf(stderr, "Could not find a video stream in the input file.\n");
		ret = 2;
		goto EXIT_CLOSE_INPUT; 
	}
	// 3. 打印文件信息（可选）
	av_dump_format(fmt_ctx, 0, argv[1], 0);
	printf("==================\n");

	AVStream *stream = fmt_ctx->streams[audio_stream_idx];
	const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!decoder) {
		fprintf(stderr, "Unsupported codec\n");
		ret = 3;
		goto EXIT_CLOSE_INPUT;
	}

	AVCodecContext *dec_ctx = avcodec_alloc_context3(decoder);
	avcodec_parameters_to_context(dec_ctx, stream->codecpar);
	if (avcodec_open2(dec_ctx, decoder, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		ret = 4;
		goto EXIT_FREE_CONTEXT;
	}

	AVFrame *frame = av_frame_alloc();
	AVPacket *pkt = av_packet_alloc();	// 分配数据包
	// 4. 循环读取数据包                   /*重要：释放包内数据引用*/
	for (;av_read_frame(fmt_ctx, pkt) >= 0; av_packet_unref(pkt)) {
		if (pkt->stream_index != audio_stream_idx) continue;
		// 获取当前包所属的流
		stream = fmt_ctx->streams[pkt->stream_index];
		/*printf("Packet: stream_index=%d, size=%d, pts=%ld, dts=%ld\n",*/
		       /*pkt->stream_index, pkt->size, pkt->pts, pkt->dts);*/
		ret = avcodec_send_packet(dec_ctx, pkt);
		if (ret < 0) {
			fprintf(stderr, "Error sending pkt for decoding\n");
			break;
		}

		// 将数据包发送给解码器
		ret = avcodec_send_packet(dec_ctx, pkt);
		if (ret < 0) {
			fprintf(stderr, "Error sending packet for decoding\n");
			break;
		}
		// 循环从解码器接收所有可能的帧
		while (ret >= 0) {
			ret = avcodec_receive_frame(dec_ctx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				fprintf(stderr, "Error during decoding: %s\n",
					av_err2str(ret));
				goto EXIT_PACKET_FREE;
			}
			// 此时 frame 包含一帧原始数据，可以处理（如保存或显示）
			/*printf("Decoded frame: %c\n", frame->data);*/
			process_audio_frame(frame);
			av_frame_unref(frame);
		}
	}
	// 最后刷新解码器（发送 NULL 数据包）
	avcodec_send_packet(dec_ctx, NULL);
	while (avcodec_receive_frame(dec_ctx, frame) == 0) {
		// 处理剩余帧
		av_frame_unref(frame);
	}

	printf("End of file reached.\n");

	ret = 0;
	// 5. 清理
EXIT_PACKET_FREE:
	av_packet_free(&pkt);
EXIT_FREE_CONTEXT:
	avcodec_free_context(&dec_ctx);
EXIT_CLOSE_INPUT:
	avformat_close_input(&fmt_ctx);
	return ret;
}
