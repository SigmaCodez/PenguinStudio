/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "obs-av1.h"
#include "obs-avc.h"
#include "obs-nal.h"
#include "util/bmem.h"
#include "amf.h"
#include "log.h"

static void av1(const uint8_t *data, size_t size)
{
	const uint8_t *expected = data + size;
	for (size_t i = 0; i + 2 < size; i++) {
		if (!data[i] && !data[i + 1] && data[i + 2] == 1) {
			expected = data + (i && !data[i - 1] ? i - 1 : i);
			break;
		}
	}
	assert(obs_nal_find_startcode(data, data + size) == expected);
	uint8_t *avc_header = NULL;
	(void)obs_parse_avc_header(&avc_header, data, size);
	bfree(avc_header);
	(void)obs_av1_keyframe(data, size);
	uint8_t *packet, *header;
	size_t packet_size, header_size;
	obs_extract_av1_headers(data, size, &packet, &packet_size, &header, &header_size);
	assert(packet_size <= size && header_size <= size);
	bfree(packet);
	bfree(header);
}
static void amf(const uint8_t *data, int size)
{
	for (int names = 0; names <= 1; names++) {
		AMFObject obj;
		AMFObjectProperty prop;
		AMF_Decode(&obj, (const char *)data, size, names);
		AMF_Reset(&obj);
		AMF3_Decode(&obj, (const char *)data, size, names);
		AMF_Reset(&obj);
		AMF_DecodeArray(&obj, (const char *)data, size, 3, names);
		AMF_Reset(&obj);
		AMFProp_Decode(&prop, (const char *)data, size, names);
		AMFProp_Reset(&prop);
		AMF3Prop_Decode(&prop, (const char *)data, size, names);
		AMFProp_Reset(&prop);
	}
}
int main(void)
{
	RTMP_LogSetLevel(RTMP_LOGCRIT);
	const uint8_t frame[] = {0x32, 1, 0};
	assert(obs_av1_keyframe(frame, sizeof(frame)));
	const uint8_t extension[] = {0x34, 0, 0};
	assert(obs_av1_keyframe(extension, sizeof(extension)));
	const uint8_t truncated[] = {0x36};
	assert(!obs_av1_keyframe(truncated, sizeof(truncated)));
	const uint8_t invalid[] = {0x32, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
	assert(!obs_av1_keyframe(invalid, sizeof(invalid)));
	uint8_t *out, *header;
	size_t out_size, header_size;
	const uint8_t sequence[] = {0x0a, 1, 0x42, 0x32, 1, 0};
	obs_extract_av1_headers(sequence, sizeof(sequence), &out, &out_size, &header, &header_size);
	assert(out_size == sizeof(sequence) && !memcmp(out, sequence, out_size));
	assert(header_size == 3 && !memcmp(header, sequence, header_size));
	bfree(out); bfree(header);
	const uint8_t string[] = {AMF_STRING, 0, 3, 'a', 'b', 'c'};
	AMFObjectProperty prop;
	assert(AMFProp_Decode(&prop, (const char *)string, sizeof(string), 0) == sizeof(string));
	assert(prop.p_vu.p_aval.av_len == 3);
	AMFProp_Reset(&prop);
	const uint8_t dynamic[] = {AMF3_OBJECT, 0x0b, 1, 3, 'x', AMF3_INTEGER, 42, 1};
	AMFObject obj;
	assert(AMF3_Decode(&obj, (const char *)dynamic, sizeof(dynamic), 1) == sizeof(dynamic));
	assert(obj.o_num == 1 && obj.o_props[0].p_vu.p_number == 42);
	AMF_Reset(&obj);
	/* Deep mixed inputs must fail without overflowing the native stack. */
	uint8_t nested[1024] = {0};
	for (size_t i = 0; i + 2 < sizeof(nested); i += 3) {
		nested[i] = AMF_OBJECT;
	}

	amf(nested, sizeof(nested));
	uint32_t rng = 0x12345678;
	for (int trial = 0; trial < 100000; trial++) {
		size_t size = trial % 257;
		uint8_t *data = malloc(size ? size : 1);
		for (size_t i = 0; i < size; i++) {
			rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
			data[i] = (uint8_t)rng;
		}
		av1(data, size);
		amf(data, (int)size);
		free(data);
	}
	return 0;
}
