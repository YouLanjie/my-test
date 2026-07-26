/**
 * @file        txt2midi.c
 * @author      Chglish
 * @date        2026-07-24
 * @brief       搞点midi写测试
 */

#include "../../include/string_view.h"
#include "lib/music_synth.h"
#include <endian.h>

typedef uint16_t u16be_t;
typedef uint32_t u32be_t;
typedef SVA_t Buf_t;

static uint16_t PPQN = 480;

Buf_t *buf_push8(Buf_t *buf, uint8_t var)
{
	if (!buf) return NULL;
	while (buf->len+1+sizeof(var) > buf->capacity)
		sva_double(buf);
	*(uint8_t*)(buf->p+buf->len) = var;
	buf->len += sizeof(var);
	return buf;
}

Buf_t *buf_push16(Buf_t *buf, uint16_t var)
{
	if (!buf) return NULL;
	while (buf->len+1+sizeof(var) > buf->capacity)
		sva_double(buf);
	*(uint16_t*)(buf->p+buf->len) = htobe16(var);
	buf->len += sizeof(var);
	return buf;
}

Buf_t *buf_push24(Buf_t *buf, uint32_t var)
{
	if (!buf) return NULL;
	while (buf->len+1+sizeof(var) > buf->capacity)
		sva_double(buf);
	buf_push8(buf, (var >> 16) & 0xFF);
	buf_push8(buf, (var >> 8) & 0xFF);
	buf_push8(buf, var & 0xFF);
	return buf;
}

Buf_t *buf_push32(Buf_t *buf, uint32_t var)
{
	if (!buf) return NULL;
	while (buf->len+1+sizeof(var) > buf->capacity)
		sva_double(buf);
	*(uint32_t*)(buf->p+buf->len) = htobe32(var);
	buf->len += sizeof(var);
	return buf;
}

Buf_t *buf_push_vlq(Buf_t *buf, uint32_t var)
{
	if (!buf) return NULL;
	while (buf->len+1+sizeof(var) > buf->capacity)
		sva_double(buf);
	uint8_t buffer[4] = {};
	size_t i = 0;
	if (var == 0) {
		buf_push8(buf, 0);
		return buf;
	}
	while (var > 0 && i < sizeof(buffer)) {
		buffer[i] = var & 0b01111111;
		var >>= 7;
		i++;
	}
	while (i > 0) {
		i--;
		if (i > 0) buffer[i] |= 0b10000000;
		buf_push8(buf, buffer[i]);
	}
	return buf;
}

Buf_t *buf_append(Buf_t *buf, Buf_t *from)
{
	if (!buf || !from) return NULL;
	if (buf->capacity < buf->len+from->len+1) {
		if (!sva_adjust_minimun(buf, buf->len+from->len+1))
			return NULL;
	}
	memcpy(buf->p+buf->len, from->p, from->len);
	buf->len += from->len;
	return buf;
}

Buf_t *buf_fwrite(Buf_t *buf, FILE *fp)
{
	if (!buf || !fp) return NULL;
	fwrite(buf->p, 1, buf->len, fp);
	return buf;
}

Buf_t *midi_write_header(Buf_t *buf, uint8_t track_num)
{
	if (!buf) return NULL;
	sva_sprintfcat(buf, "MThd");
	buf_push32(buf, 6);
	buf_push16(buf, 1);    /* 文件类型 */
	buf_push16(buf, track_num);    /* 轨道数 */
	buf_push16(buf, 480);
	return buf;
}

enum Events_t {
	EV_META_HEAD = 0xFF,
	EV_META_TEXT = 0x01,
	// EV_META_COPYRIGHT = 0x02,
	// EV_META_TRACKNAME = 0x03,
	// EV_META_INSTNAME = 0x04,
	// EV_META_LYRI = 0x05,
	EV_META_ENDTRACK0 = 0x2F,    /* 00, 结束轨道 */
	EV_META_SETSPEED3 = 0x51,    /* 设置速度(数据为3字节24位型微秒) */
	/* FF 58 04 nn dd cc bb 拍号
	 * (nn/dd, dd:0全1二分2四分3八分)
	 * (cc/bb, 照抄0x1808)*/
	EV_META_SETNOTE4 = 0x58,
	/* FF 59 02 sf mi 调号
	 * sf = -7: 7 个降号
	 * sf = -1: 1 个降号
	 * sf = 0: C 大调
	 * sf = 1: 1 个升号
	 * sf = 7: 7 个升号
	 *
	 * mi = 0: 大调
	 * mi = 1: 小调
	 * */
	EV_META_SETKEY2 = 0x59,
	EV_MIDI_SETVOL2 = 0xB0,
	EV_MIDI_OFF2 = 0x80,
	/* 力度，默认64 */
	EV_MIDI_ON2 = 0x90,
	EV_MIDI_SETINST1 = 0xC0,
};

void midi_meta_setspeed(Buf_t *ev, double bpm)
{
	if (!ev) return;
	buf_push_vlq(ev, 0);
	buf_push8(ev, EV_META_HEAD);
	buf_push8(ev, EV_META_SETSPEED3);
	buf_push8(ev, 3);
	buf_push24(ev, 60e6/bpm);    /* bpm转微秒 */
}

/* 每小节beates拍/以notes分音符为一拍 */
void midi_meta_setnote(Buf_t *ev, uint8_t beates, uint8_t notes)
{
	if (!ev) return;
	buf_push_vlq(ev, 0);
	buf_push8(ev, EV_META_HEAD);
	buf_push8(ev, EV_META_SETNOTE4);
	buf_push8(ev, 4);
	buf_push8(ev, beates);
	buf_push8(ev, round(log2(notes)));
	buf_push8(ev, 0x18);
	buf_push8(ev, 0x08);
}

void midi_meta_endtrack(Buf_t *ev)
{
	if (!ev) return;
	buf_push_vlq(ev, 0);
	buf_push8(ev, EV_META_HEAD);
	buf_push8(ev, EV_META_ENDTRACK0);
	buf_push8(ev, 0);
}

void midi_track_setinst(Buf_t *ev, uint8_t channel, uint8_t id)
{
	if (!ev) return;
	buf_push_vlq(ev, 0);
	buf_push8(ev, EV_MIDI_SETINST1|(channel&0x0F));
	buf_push8(ev, id);
}

void midi_track_setvolumn(Buf_t *ev, uint8_t channel, uint8_t vol)
{
	if (!ev) return;
	buf_push_vlq(ev, 0);
	buf_push8(ev, EV_MIDI_SETVOL2|(channel&0x0F));
	buf_push8(ev, 0x07);    /* 主音量 */
	buf_push8(ev, vol&0b01111111);
}

/* delay分音符的延迟 */
void midi_track_noteon(Buf_t *ev, uint8_t channel, double freq, double delay)
{
	if (!ev) return;
	buf_push_vlq(ev, delay>0?PPQN*(4/delay):0);
	buf_push8(ev, EV_MIDI_ON2|(channel&0x0F));
	uint8_t pitch = 69 + round(12 * log2(freq / 440.0));
	buf_push8(ev, pitch);
	buf_push8(ev, 100);    /* 力道 */
}

/* note: note分音符 */
void midi_track_noteoff(Buf_t *ev, uint8_t channel, double freq, double note)
{
	if (!ev) return;
	buf_push_vlq(ev, note>0?PPQN*(4/note):0);
	buf_push8(ev, EV_MIDI_OFF2|(channel&0x0F));
	uint8_t pitch = freq > 0 ? (69 + round(12 * log2(freq / 440.0))) : 0;
	buf_push8(ev, pitch);
	buf_push8(ev, 0);    /* 力道 */
}

/*  14B <HEAD>
 *    ? <TRACK>   4 "MTrk"
 *                4 LEN
 *             $LEN <EVENT>  VLQ Delta Time
 *                             1 <META(FF)|MIDI|SYS>
 *                             1 TYPE
 *                           VLQ DATA $LEN
 *                          $LEN DATA
 * */

int streamer_file(void *p)
{
	if (!p) return EOF;
	return getc(p);
}

int main(int argc, char *argv[])
{
	int ch = 0;
	char *inputf = "input.txt";
	char *outputf = "output.midi";

	while ((ch = getopt(argc, argv, "i:o:h")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: %s [Option]\n"
			       "Option:\n"
			       "    -i <FILE> 输入文件(txt曲谱)\n"
			       "    -o <FILE> 输出文件(midi文件)\n",
			       argv[0]);
			return ch == '?' ? -1 : 0;
			break;
		case 'i': inputf = optarg; break;
		case 'o': outputf = optarg; break;
		default:
			break;
		}
	}


	FILE *fp = fopen(inputf, "r");
	if (!fp) {
		printf("FILE `%s` could not open\n", inputf);
		return 1;
	}
	MusicCtx_t *music = music_ctx_create(SAMPLE_RATE/20);
	music->notes = note_parser(streamer_file, fp);
	music_ctx_stat(music);
	fclose(fp);

	Buf_t tracks = {}, event = {};
	sva_create(&tracks);
	sva_create(&event);

	Note_t *p;
	int flg_track_num = 0;
	bool flg_tracks[countof(music->tracks)] = {};
	for (p = music->notes; p; p=p->next) flg_tracks[p->track%countof(flg_tracks)] = true;

	for (size_t i = 0; i < countof(music->tracks); i++) {
		if (!flg_tracks[i]) continue;
		flg_track_num++;

		double bpm = 120;
		uint8_t beates = 4, notes = 4;
		if (p && p->pcm_data) {
			bpm = p->pcm_data->speed;
			beates = p->pcm_data->beates;
			notes = p->pcm_data->notes;
		}
		midi_meta_setspeed(&event, bpm);
		midi_meta_setnote(&event, beates, notes);
		midi_track_setvolumn(&event, 0, 100);
		// 假定只用单一乐器
		midi_track_setinst(&event, 0, 0);

		size_t base_position = 0,
		       track_offset = 0,
		       now_position = 0;
		for (p = music->notes; p; p=p->next) {
			if (!p->pcm_data) continue;
			if (p->pcm_data->beates != beates || p->pcm_data->notes != notes)
				midi_meta_setnote(&event, (beates = p->pcm_data->beates),
						  (notes = p->pcm_data->notes));
			if (p->pcm_data->speed != bpm)
				midi_meta_setspeed(&event, (bpm = p->pcm_data->speed));
			if (p->track == 0) {
				base_position += p->pcm_data->sample_num;
			}
			if (p->prev && p->prev->track != p->track) track_offset = 0;

			if (p->track != i) continue;
			if (p->track != 0) track_offset += p->pcm_data->sample_num;
			if (p->pcm_data->freq <= 0) continue;
			size_t d1 = base_position+track_offset;
			size_t d2 = now_position+p->pcm_data->sample_num;
			double n = (d1>d2) ? ((d1-d2)*1./SAMPLE_RATE*bpm/60) : 0;
			midi_track_noteon(&event, 0, p->pcm_data->freq, (n>0)?p->pcm_data->notes/n:0);
			midi_track_noteoff(&event, 0, p->pcm_data->freq, p->pcm_data->type);
			now_position = base_position+track_offset;
		}
		midi_meta_endtrack(&event);
		sva_sprintfcat(&tracks, "MTrk");
		buf_push32(&tracks, event.len);
		buf_append(&tracks, &event);
		sva_clear(&event);
	}
	Buf_t buf = {};
	sva_create(&buf);
	midi_write_header(&buf, flg_track_num);
	buf_append(&buf, &tracks);

	sva_free(&event);
	sva_free(&tracks);
	music_ctx_free(music);

	fp = fopen(outputf, "wb");
	if (!fp) {
		printf("FILE `%s` could not open\n", outputf);
		sva_free(&tracks);
		return 1;
	}
	buf_fwrite(&buf, fp);
	fclose(fp);
	sva_free(&buf);
	return 0;
}

