#include <obs-module.h>
#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

void InitSceneSwitcher();
void FreeSceneSwitcher();

#if defined(_WIN32)
void InitCaptions();
void FreeCaptions();
#endif

void InitOutputTimer();
void FreeOutputTimer();

#if defined(ENABLE_SCRIPTING)
void InitScripts();
void FreeScripts();
static bool scripts_initialized;
#endif

bool obs_module_load(void)
{
#if defined(_WIN32)
	InitCaptions();
#endif
	InitSceneSwitcher();
	InitOutputTimer();
#if defined(ENABLE_SCRIPTING)
	if (obs_frontend_is_safe_mode_enabled()) {
		blog(LOG_WARNING, "Scripting is disabled in Safe Mode");
	} else {
		InitScripts();
		scripts_initialized = true;
	}
#endif
	return true;
}

void obs_module_unload(void)
{
#if defined(_WIN32)
	FreeCaptions();
#endif
	FreeSceneSwitcher();
	FreeOutputTimer();
#if defined(ENABLE_SCRIPTING)
	// Frontend callbacks may already be gone when core modules are unloaded.
	// Release only the scripting state this module actually initialized.
	if (scripts_initialized) {
		FreeScripts();
		scripts_initialized = false;
	}
#endif
}
