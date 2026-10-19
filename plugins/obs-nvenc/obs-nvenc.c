#include <obs-module.h>

#include "obs-nvenc.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-nvenc", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "NVIDIA Encoder (NVENC) Plugin";
}

static bool encoders_initialized;

bool obs_module_load(void)
{
	if (!nvenc_supported()) {
		blog(LOG_INFO, "NVENC not supported; NVIDIA encoders are unavailable");
		return true;
	}

	obs_nvenc_load();
	obs_cuda_load();
	encoders_initialized = true;

	return true;
}

void obs_module_unload(void)
{
	if (!encoders_initialized)
		return;
	obs_cuda_unload();
	obs_nvenc_unload();
}
