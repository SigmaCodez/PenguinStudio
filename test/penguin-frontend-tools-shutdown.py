#!/usr/bin/env python3
"""Exercise the real frontend-tools lifecycle after frontend callback teardown."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="penguin-tools-shutdown-") as directory:
    path = Path(directory)
    (path / "obs-module.h").write_text("""#include <stdbool.h>
#define OBS_DECLARE_MODULE()
#define OBS_MODULE_USE_DEFAULT_LOCALE(a,b)
#define LOG_WARNING 1
void blog(int level, const char *message);
""")
    (path / "obs-frontend-api.h").write_text("#include <stdbool.h>\nbool obs_frontend_is_safe_mode_enabled(void);\n")
    (path / "probe.c").write_text("""#include <assert.h>
#include <stdbool.h>
static bool alive, safe;
static int queries, initialized, freed;
bool obs_frontend_is_safe_mode_enabled(void) { assert(alive); ++queries; return safe; }
void blog(int level, const char *message) { (void)level; (void)message; }
void InitSceneSwitcher(void) {} void FreeSceneSwitcher(void) {}
void InitOutputTimer(void) {} void FreeOutputTimer(void) {}
void InitScripts(void) { ++initialized; } void FreeScripts(void) { ++freed; }
bool obs_module_load(void); void obs_module_unload(void);
int main(void) {
    for (int mode = 0; mode < 2; ++mode) {
        safe = mode; alive = true;
        assert(obs_module_load());
        alive = false;
        obs_module_unload();
    }
#ifdef ENABLE_SCRIPTING
    assert(queries == 2 && initialized == 1 && freed == 1);
#else
    assert(queries == 0 && initialized == 0 && freed == 0);
#endif
    return 0;
}
""")
    for scripting in (False, True):
        binary = path / ("enabled" if scripting else "disabled")
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        *(["-DENABLE_SCRIPTING"] if scripting else []), f"-I{path}",
                        str(root / "plugins/frontend-tools/frontend-tools.c"),
                        str(path / "probe.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print("Frontend tools unload without frontend callbacks in normal and safe mode")
