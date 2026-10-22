/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "plugins/obs-ffmpeg/audio-encoder-drain.h"
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int warnings;
static void capture_log(void *context, int level, const char *format, va_list args)
{
	(void)context;
	if (level <= AV_LOG_WARNING) {
		warnings++;
		vfprintf(stderr, format, args);
	}
}

static void run(const char *name, int frames, bool leave_packet)
{
	const AVCodec *codec = avcodec_find_encoder_by_name(name);
	assert(codec);
	AVCodecContext *context = avcodec_alloc_context3(codec);
	assert(context);
	context->sample_rate = 48000;
	context->sample_fmt = strcmp(name, "aac") == 0 ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_S16;
	context->bit_rate = 128000;
	context->time_base = (AVRational){1, 48000};
	av_channel_layout_default(&context->ch_layout, 2);
	assert(avcodec_open2(context, codec, NULL) == 0);
	AVFrame *frame = av_frame_alloc();
	assert(frame);
	frame->format = context->sample_fmt;
	frame->sample_rate = context->sample_rate;
	frame->nb_samples = context->frame_size ? context->frame_size : 1024;
	assert(av_channel_layout_copy(&frame->ch_layout, &context->ch_layout) == 0);
	assert(av_frame_get_buffer(frame, 0) == 0);
	assert(av_samples_set_silence(frame->data, 0, frame->nb_samples, 2, context->sample_fmt) == 0);
	AVPacket packet = {0};
	for (int i = 0; i < frames; i++) {
		frame->pts = (int64_t)i * frame->nb_samples;
		int ret = avcodec_send_frame(context, frame);
		if (leave_packet && ret == AVERROR(EAGAIN))
			break;
		assert(ret == 0);
		if (!leave_packet) {
			while ((ret = avcodec_receive_packet(context, &packet)) == 0)
				av_packet_unref(&packet);
			assert(ret == AVERROR(EAGAIN));
		}
	}
	assert(ffmpeg_audio_encoder_drain(context) == 0);
	assert(avcodec_receive_packet(context, &packet) == AVERROR_EOF);
	assert(ffmpeg_audio_encoder_drain(context) == 0);
	av_frame_free(&frame);
	avcodec_free_context(&context);
}

int main(void)
{
	av_log_set_callback(capture_log);
	assert(ffmpeg_audio_encoder_drain(NULL) == 0);
	AVCodecContext *unopened = avcodec_alloc_context3(NULL);
	assert(unopened);
	assert(ffmpeg_audio_encoder_drain(unopened) == 0);
	avcodec_free_context(&unopened);
	for (int repeat = 0; repeat < 3; repeat++) {
		run("aac", 0, false);
		run("aac", 1, false);
		run("aac", 300, false);
		run("aac", 20, true);
		run("pcm_s16le", 3, false);
	}
	assert(warnings == 0);
	puts("Audio drain: empty, short, long, pending packets, repeated stop and PCM passed without warnings");
}
