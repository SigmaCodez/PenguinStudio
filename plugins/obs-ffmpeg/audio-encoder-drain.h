/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <libavcodec/avcodec.h>

/* Outputs have detached before encoder destruction. Finish delayed encoding
 * and release its packets without sending data to an already stopped output. */
static inline int ffmpeg_audio_encoder_drain(AVCodecContext *context)
{
	if (!context || !avcodec_is_open(context))
		return 0;

	AVPacket packet = {0};
	int ret = avcodec_send_frame(context, NULL);
	while (ret == AVERROR(EAGAIN)) {
		ret = avcodec_receive_packet(context, &packet);
		av_packet_unref(&packet);
		if (ret < 0)
			return ret;
		ret = avcodec_send_frame(context, NULL);
	}
	if (ret < 0 && ret != AVERROR_EOF)
		return ret;

	do {
		ret = avcodec_receive_packet(context, &packet);
		av_packet_unref(&packet);
	} while (ret == 0);

	return ret == AVERROR_EOF ? 0 : ret;
}
