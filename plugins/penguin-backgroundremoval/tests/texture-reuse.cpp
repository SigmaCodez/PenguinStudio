// SPDX-License-Identifier: GPL-3.0-or-later
// Manual native Wayland/OpenGL comparison. See README.md in this directory.
#include <obs.h>
#include <obs-nix-platform.h>
#include <graphics/graphics.h>
#include <graphics/vec4.h>
#include <wayland-client.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

static void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

struct GraphicsContext {
	wl_display *display = nullptr;
	graphics_t *graphics = nullptr;
	bool entered = false;
	~GraphicsContext()
	{
		if (entered)
			gs_leave_context();
		if (graphics)
			gs_destroy(graphics);
		if (display)
			wl_display_disconnect(display);
	}
};
using Texture = std::unique_ptr<gs_texture_t, decltype(&gs_texture_destroy)>;
using Target = std::unique_ptr<gs_texrender_t, decltype(&gs_texrender_destroy)>;
using Surface = std::unique_ptr<gs_stagesurf_t, decltype(&gs_stagesurface_destroy)>;
using Effect = std::unique_ptr<gs_effect_t, decltype(&gs_effect_destroy)>;

static Texture texture(unsigned width, unsigned height, gs_color_format format, const uint8_t *data = nullptr,
		       uint32_t flags = 0)
{
	Texture result(gs_texture_create(width, height, format, 1, data ? &data : nullptr, flags), gs_texture_destroy);
	require(bool(result), "Texture creation failed");
	return result;
}

static Target target()
{
	Target result(gs_texrender_create(GS_BGRA, GS_ZS_NONE), gs_texrender_destroy);
	require(bool(result), "Render target creation failed");
	return result;
}

static std::vector<uint8_t> readPixels(gs_texture_t *input, unsigned width, unsigned height, gs_color_format format)
{
	Surface stage(gs_stagesurface_create(width, height, format), gs_stagesurface_destroy);
	require(bool(stage), "Staging surface creation failed");
	gs_stage_texture(stage.get(), input);
	uint8_t *mapped = nullptr;
	uint32_t pitch = 0;
	require(gs_stagesurface_map(stage.get(), &mapped, &pitch), "Staging surface map failed");
	// OBS's OpenGL staging allocation and glGetTexImage use four-byte alignment,
	// including R8 rows; some OBS versions report only the unaligned row width.
	pitch = (pitch + 3) & ~3u;
	const unsigned rowBytes = width * (format == GS_R8 ? 1 : 4);
	if (!mapped || pitch < rowBytes) {
		gs_stagesurface_unmap(stage.get());
		throw std::runtime_error("Invalid staging buffer");
	}
	std::vector<uint8_t> result(size_t(rowBytes) * height);
	for (unsigned y = 0; y < height; ++y)
		std::memcpy(result.data() + size_t(y) * rowBytes, mapped + size_t(y) * pitch, rowBytes);
	gs_stagesurface_unmap(stage.get());
	return result;
}

static void alphaUploadComparisons()
{
	unsigned cases = 0;
	for (auto [width, height] : std::vector<std::pair<unsigned, unsigned>>{{1, 1}, {7, 9}, {31, 17},
									  {320, 192}, {641, 359}}) {
		auto cached = texture(width, height, GS_R8, nullptr, GS_DYNAMIC);
		const unsigned aligned = (width + 3) & ~3u;
		for (unsigned frame = 0; frame < 20; ++frame) {
			std::vector<uint8_t> packed(width * height), padded((width + 7) * height, 42);
			std::vector<uint8_t> initial(aligned * height, 0);
			for (unsigned y = 0; y < height; ++y) {
				for (unsigned x = 0; x < width; ++x) {
					const auto value = uint8_t(x * 31 + y * 17 + frame * 53);
					packed[y * width + x] = padded[y * (width + 7) + x] = initial[y * aligned + x] = value;
				}
			}
			// Initial-data uploads also follow OpenGL's four-byte row alignment.
			auto original = texture(width, height, GS_R8, initial.data());
			uint8_t *mapped = nullptr;
			uint32_t pitch = 0;
			require(gs_texture_map(cached.get(), &mapped, &pitch), "Dynamic texture map failed");
			if (!mapped || pitch < width) {
				gs_texture_unmap(cached.get());
				throw std::runtime_error("Invalid dynamic texture buffer");
			}
			for (unsigned y = 0; y < height; ++y)
				std::memcpy(mapped + size_t(y) * pitch, padded.data() + size_t(y) * (width + 7), width);
			gs_texture_unmap(cached.get());
			const auto actual = readPixels(cached.get(), width, height, GS_R8);
			require(actual == packed, "Dynamic alpha upload changed the synthetic pattern");
			require(actual == readPixels(original.get(), width, height, GS_R8), "Alpha upload paths differ");
			++cases;
		}
	}
	std::printf("%u alpha-upload comparisons passed (repeated updates, odd widths, padded rows)\n", cases);
}

static void renderPass(gs_effect_t *effect, gs_texrender_t *destination, gs_texture_t *input, gs_texture_t *mask,
		       unsigned width, unsigned height, int iteration, int count, bool focal)
{
	gs_texrender_reset(destination);
	require(gs_texrender_begin(destination, width, height), "Blur render begin failed");
	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), input);
	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "focalmask"), mask);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "xOffset"), (float(iteration) + .5f) / width);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "yOffset"), (float(iteration) + .5f) / height);
	gs_effect_set_int(gs_effect_get_param_by_name(effect, "blurIter"), iteration);
	gs_effect_set_int(gs_effect_get_param_by_name(effect, "blurTotal"), count);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "blurFocusPoint"), .2f);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "blurFocusDepth"), .1f);
	vec4 clear{};
	gs_clear(GS_CLEAR_COLOR, &clear, 0, 0);
	gs_ortho(0, float(width), 0, float(height), -100, 100);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	while (gs_effect_loop(effect, focal ? "DrawFocalBlur" : "Draw"))
		gs_draw_sprite(input, 0, width, height);
	gs_blend_state_pop();
	gs_texrender_end(destination);
}

static void blurComparisons(gs_effect_t *effect)
{
	Target pingpong[2] = {target(), target()};
	unsigned cases = 0;
	for (auto [width, height] : std::vector<std::pair<unsigned, unsigned>>{{64, 48}, {320, 192}, {257, 129}}) {
		for (bool focal : {false, true}) {
			for (int count : {1, 2, 3, 8}) {
				for (unsigned frame = 0; frame < 3; ++frame) {
					const unsigned pitch = (width + 3) & ~3u;
					std::vector<uint8_t> image(width * height * 4), alpha(pitch * height, 0);
					for (unsigned y = 0; y < height; ++y) {
						for (unsigned x = 0; x < width; ++x) {
							for (unsigned c = 0; c < 4; ++c)
								image[(y * width + x) * 4 + c] = uint8_t(x * 31 + y * 17 + c * 53 + frame * 71);
							alpha[y * pitch + x] = uint8_t(x * 3 + y * 7 + frame * 19);
						}
					}
					auto input = texture(width, height, GS_BGRA, image.data());
					auto mask = texture(width, height, GS_R8, alpha.data());
					auto copied = texture(width, height, GS_BGRA);
					auto original = target();
					gs_copy_texture(copied.get(), input.get());
					for (int i = 0; i < count; ++i) {
						renderPass(effect, original.get(), copied.get(), mask.get(), width, height, i, count, focal);
						gs_copy_texture(copied.get(), gs_texrender_get_texture(original.get()));
					}
					const auto expected = readPixels(copied.get(), width, height, GS_BGRA);
					auto *current = input.get();
					for (int i = 0; i < count; ++i) {
						renderPass(effect, pingpong[i % 2].get(), current, mask.get(), width, height, i, count, focal);
						current = gs_texrender_get_texture(pingpong[i % 2].get());
					}
					if (readPixels(current, width, height, GS_BGRA) != expected) {
						std::fprintf(stderr, "Blur mismatch: %ux%u, focal=%d, passes=%d, frame=%u\n",
							     width, height, focal, count, frame);
						throw std::runtime_error("Copy-chain and ping-pong shader results differ");
					}
					++cases;
				}
			}
		}
	}
	std::printf("%u blur comparisons passed; byte difference=0 (normal/focal, resizing, repeated frames, 1/2/3/8 passes)\n", cases);
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		std::fprintf(stderr, "Usage: %s /path/to/libobs-opengl.so /path/to/kawase_blur.effect\n", argv[0]);
		return 2;
	}
	if (!std::getenv("WAYLAND_DISPLAY")) {
		std::fprintf(stderr, "SKIP: this manual GPU test requires an active Wayland session\n");
		return 77;
	}
	try {
		GraphicsContext context;
		context.display = wl_display_connect(nullptr);
		require(context.display, "Wayland connection failed");
		obs_set_nix_platform(OBS_NIX_PLATFORM_WAYLAND);
		obs_set_nix_platform_display(context.display);
		require(gs_create(&context.graphics, argv[1], 0) == GS_SUCCESS, "OpenGL graphics creation failed");
		gs_enter_context(context.graphics);
		context.entered = true;
		char *errors = nullptr;
		Effect effect(gs_effect_create_from_file(argv[2], &errors), gs_effect_destroy);
		if (errors) {
			std::fprintf(stderr, "%s\n", errors);
			bfree(errors);
		}
		require(bool(effect), "Kawase effect creation failed");
		alphaUploadComparisons();
		blurComparisons(effect.get());
	} catch (const std::exception &error) {
		std::fprintf(stderr, "%s\n", error.what());
		return 1;
	}
}
