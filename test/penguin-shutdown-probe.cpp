// SPDX-License-Identifier: GPL-2.0-or-later
// LD_PRELOAD observer for isolated native GUI runs. Calls through to the real
// frontend API unchanged; late unregisters remain visible in the application log.
#include <obs-frontend-api.h>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QMainWindow>
#include <QPointer>
#include <QTimer>
#include <QWizard>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

struct obs_frontend_callbacks;
namespace {
bool apiInitialized = false;
bool apiAlive = false;
bool apiDestroyed = false;
bool closeRequested = false;
bool wizardVisibleAtClose = false;
bool sentinelObserved = false;
bool statsOpened = false;
int unregisterCalls = 0;
int lateUnregisterCalls = 0;
int ticks = 0;
char resultPath[4096]{};
QPointer<QMainWindow> mainWindow;

void Tick()
{
	++ticks;
	const bool wizardMode = qEnvironmentVariable("PENGUIN_SHUTDOWN_MODE") == "wizard";
	for (auto *widget : QApplication::topLevelWidgets()) {
		if (QByteArray(widget->metaObject()->className()) == "OBSBasic")
			mainWindow = qobject_cast<QMainWindow *>(widget);
		if (ticks < 12) {
			if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible()) {
				if (!wizardMode || !qobject_cast<QWizard *>(dialog))
					dialog->reject();
			}
		}
	}
	if (!mainWindow)
		return;
	if (ticks == 12) {
		const QDir sentinel(QDir(qEnvironmentVariable("XDG_CONFIG_HOME"))
				    .filePath("PenguinStudio/.sentinel"));
		sentinelObserved = !sentinel.entryList({"run_*"}, QDir::Files).isEmpty();
	}
	if (qEnvironmentVariable("PENGUIN_SHUTDOWN_MODE") == "stats") {
		if (ticks == 14)
			if (auto *action = mainWindow->findChild<QAction *>("stats"))
				QTimer::singleShot(0, action, &QAction::trigger);
		if (ticks == 18)
			for (auto *widget : QApplication::topLevelWidgets())
				if (QByteArray(widget->metaObject()->className()) == "OBSBasicStats" && widget->isVisible()) {
					statsOpened = true;
					widget->close();
				}
	}
	if (ticks == 24) {
		for (auto *widget : QApplication::topLevelWidgets())
			if (qobject_cast<QWizard *>(widget) && widget->isVisible())
				wizardVisibleAtClose = true;
		closeRequested = true;
		if (qEnvironmentVariable("PENGUIN_SHUTDOWN_MODE") == "signal")
			std::raise(SIGTERM);
		else
			mainWindow->close();
	}
}

void Start()
{
	const QByteArray path = qgetenv("PENGUIN_SHUTDOWN_REPORT");
	if (path.isEmpty())
		return;
	std::snprintf(resultPath, sizeof(resultPath), "%s", path.constData());
	QTimer::singleShot(1000, qApp, [] {
		auto *timer = new QTimer(qApp);
		QObject::connect(timer, &QTimer::timeout, Tick);
		timer->start(250);
	});
}
Q_COREAPP_STARTUP_FUNCTION(Start)

// Save after module unload as well as Qt shutdown: saving at aboutToQuit would
// miss the late WebSocket destructor that caused the original diagnostic.
__attribute__((destructor)) void SaveReport()
{
	if (!resultPath[0])
		return;
	if (FILE *file = std::fopen(resultPath, "w")) {
		std::fprintf(file,
			"{\"apiInitialized\":%s,\"apiDestroyed\":%s,\"unregisterCalls\":%d,"
			"\"lateUnregisterCalls\":%d,\"closeRequested\":%s,\"sentinelObserved\":%s,"
			"\"wizardVisibleAtClose\":%s,\"statsOpened\":%s}\n",
			apiInitialized ? "true" : "false", apiDestroyed ? "true" : "false", unregisterCalls,
			lateUnregisterCalls, closeRequested ? "true" : "false", sentinelObserved ? "true" : "false",
			wizardVisibleAtClose ? "true" : "false", statsOpened ? "true" : "false");
		std::fclose(file);
	}
}
} // namespace

void obs_frontend_set_callbacks_internal(obs_frontend_callbacks *callbacks)
{
	using SetCallbacks = void (*)(obs_frontend_callbacks *);
	static auto real = reinterpret_cast<SetCallbacks>(
		dlsym(RTLD_NEXT, "_Z35obs_frontend_set_callbacks_internalP22obs_frontend_callbacks"));
	if (!real)
		std::abort();
	apiAlive = callbacks != nullptr;
	apiInitialized |= apiAlive;
	apiDestroyed |= !apiAlive && apiInitialized;
	real(callbacks);
}

extern "C" void obs_frontend_remove_event_callback(obs_frontend_event_cb callback, void *data)
{
	static auto real = reinterpret_cast<decltype(&obs_frontend_remove_event_callback)>(
		dlsym(RTLD_NEXT, "obs_frontend_remove_event_callback"));
	if (!real)
		std::abort();
	++unregisterCalls;
	if (!apiAlive)
		++lateUnregisterCalls;
	real(callback, data);
}
