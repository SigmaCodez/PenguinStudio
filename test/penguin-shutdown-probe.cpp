// SPDX-License-Identifier: GPL-2.0-or-later
// LD_PRELOAD observer for isolated native GUI runs. Calls through to the real
// frontend API unchanged; late unregisters remain visible in the application log.
#include <obs-frontend-api.h>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QLineEdit>
#include <QMainWindow>
#include <QPointer>
#include <QTimer>
#include <QWindow>
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
bool updaterOpened = false;
bool updaterFocused = false;
bool updaterClosed = false;
QPointer<QDialog> testedUpdater;
bool connectionWizardOpened = false;
bool autoConfigOpened = false;
bool startupAutoConfigObserved = false;
bool wizardInputFocused = false;
bool wizardInputFocusedAtClose = false;
bool wizardCancelRequested = false;
bool wizardClosedBeforeShutdown = false;
int unregisterCalls = 0;
int lateUnregisterCalls = 0;
int ticks = 0;
char resultPath[4096]{};
QPointer<QMainWindow> mainWindow;
QPointer<QWizard> testedWizard;
QPointer<QLineEdit> testedInput;

void Mark(const char *event)
{
	std::fprintf(stderr, "PENGUIN_SHUTDOWN_PROBE: %s\n", event);
	std::fflush(stderr);
}

void ExerciseWizard(const QString &mode)
{
	const bool connectionMode = mode == "wizard-cancel" || mode == "wizard-input";
	const bool autoConfigMode = mode == "auto-config";
	if (!connectionMode && !autoConfigMode)
		return;
	if (ticks == 14) {
		Mark("open wizard");
		if (autoConfigMode) {
			if (auto *action = mainWindow->findChild<QAction *>("autoConfigure"))
				QTimer::singleShot(0, action, &QAction::trigger);
		} else {
			// This is an isolated, empty profile. Never invoke Go Live with a configured destination.
			auto *outputs = mainWindow->findChild<QWidget *>("penguinOutputPanel");
			if (outputs && outputs->property("penguinDestinationCount").toInt() == 0)
				if (auto *button = mainWindow->findChild<QAbstractButton *>("penguinLiveButton"))
					QTimer::singleShot(0, button, &QAbstractButton::click);
		}
	}
	if (ticks == 16) {
		for (auto *widget : QApplication::topLevelWidgets()) {
			auto *wizard = qobject_cast<QWizard *>(widget);
			if (!wizard || !wizard->isVisible())
				continue;
			const bool isAutoConfig = QByteArray(wizard->metaObject()->className()) == "AutoConfig";
			if (isAutoConfig != autoConfigMode)
				continue;
			testedWizard = wizard;
			connectionWizardOpened = connectionMode;
			autoConfigOpened = autoConfigMode;
			wizard->activateWindow();
			if (connectionMode) {
				for (auto *input : wizard->findChildren<QLineEdit *>()) {
					if (input->isVisible() && input->isEnabled() &&
					    input->echoMode() == QLineEdit::Normal) {
						testedInput = input;
						input->setFocus(Qt::OtherFocusReason);
						break;
					}
				}
			}
			break;
		}
	}
	if (ticks == 18 && testedInput && testedWizard) {
		wizardInputFocused = testedInput->hasFocus() &&
				     QGuiApplication::focusWindow() == testedWizard->windowHandle();
		if (wizardInputFocused)
			Mark("wizard text input has native keyboard focus");
	}
	if (ticks == 20 && mode != "wizard-input" && testedWizard) {
		Mark("cancel wizard");
		wizardCancelRequested = true;
		testedWizard->reject();
	}
	if (ticks == 22 && wizardCancelRequested)
		wizardClosedBeforeShutdown = !testedWizard || !testedWizard->isVisible();
}

void Tick()
{
	++ticks;
	const QString mode = qEnvironmentVariable("PENGUIN_SHUTDOWN_MODE");
	const bool wizardMode = mode == "wizard";
	for (auto *widget : QApplication::topLevelWidgets()) {
		if (QByteArray(widget->metaObject()->className()) == "OBSBasic")
			mainWindow = qobject_cast<QMainWindow *>(widget);
		if (ticks < 12) {
			if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible()) {
				startupAutoConfigObserved |= QByteArray(dialog->metaObject()->className()) == "AutoConfig";
				if (!wizardMode || !qobject_cast<QWizard *>(dialog))
					dialog->reject();
			}
		}
	}
	if (!mainWindow)
		return;
	ExerciseWizard(mode);
	if (ticks == 12) {
		const QDir sentinel(QDir(qEnvironmentVariable("XDG_CONFIG_HOME"))
				    .filePath("PenguinStudio/.sentinel"));
		sentinelObserved = !sentinel.entryList({"run_*"}, QDir::Files).isEmpty();
	}
	if (mode == "updater") {
		if (ticks == 14)
			if (auto *action = mainWindow->findChild<QAction *>("actionCheckForUpdates"))
				QTimer::singleShot(0, action, &QAction::trigger);
		if (ticks == 16) {
			testedUpdater = mainWindow->findChild<QDialog *>("penguinLinuxUpdater");
			updaterOpened = testedUpdater && testedUpdater->isVisible();
			if (updaterOpened) {
				testedUpdater->activateWindow();
				if (auto *button = testedUpdater->findChild<QAbstractButton *>())
					button->setFocus(Qt::OtherFocusReason);
			}
		}
		if (ticks == 18 && testedUpdater)
			updaterFocused = testedUpdater->isActiveWindow() && QApplication::focusWidget() &&
					 testedUpdater->isAncestorOf(QApplication::focusWidget());
		if (ticks == 20 && testedUpdater)
			testedUpdater->reject();
		if (ticks == 22)
			updaterClosed = testedUpdater && !testedUpdater->isVisible();
	}
	if (mode == "stats") {
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
		wizardInputFocusedAtClose = testedInput && testedWizard && testedInput->hasFocus() &&
					    QGuiApplication::focusWindow() == testedWizard->windowHandle();
		closeRequested = true;
		Mark("close main window");
		if (mode == "signal")
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
			"\"wizardVisibleAtClose\":%s,\"statsOpened\":%s,\"connectionWizardOpened\":%s,"
			"\"autoConfigOpened\":%s,\"wizardInputFocused\":%s,\"wizardInputFocusedAtClose\":%s,"
			"\"wizardCancelRequested\":%s,\"wizardClosedBeforeShutdown\":%s,\"startupAutoConfigObserved\":%s,\"updaterOpened\":%s,\"updaterFocused\":%s,\"updaterClosed\":%s}\n",
			apiInitialized ? "true" : "false", apiDestroyed ? "true" : "false", unregisterCalls,
			lateUnregisterCalls, closeRequested ? "true" : "false", sentinelObserved ? "true" : "false",
			wizardVisibleAtClose ? "true" : "false", statsOpened ? "true" : "false",
			connectionWizardOpened ? "true" : "false", autoConfigOpened ? "true" : "false",
			wizardInputFocused ? "true" : "false", wizardInputFocusedAtClose ? "true" : "false",
			wizardCancelRequested ? "true" : "false", wizardClosedBeforeShutdown ? "true" : "false",
			startupAutoConfigObserved ? "true" : "false", updaterOpened ? "true" : "false",
			updaterFocused ? "true" : "false", updaterClosed ? "true" : "false");
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
