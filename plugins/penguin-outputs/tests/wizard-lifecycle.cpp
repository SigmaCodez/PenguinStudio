// SPDX-License-Identifier: GPL-2.0-or-later
// Regression for closing the main window while its setup wizard is still open.
#include "../penguin-outputs.cpp"

#include <QElapsedTimer>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>

namespace {
QString configuration;
int callbackRemovals = 0;

void Require(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(EXIT_FAILURE);
	}
}

QPushButton *Add(OutputDock &dock)
{
	for (auto *button : dock.findChildren<QPushButton *>())
		if (button->text() == "Penguin.Add")
			return button;
	Require(false, "add destination button missing");
	return nullptr;
}

void DrainDeletes()
{
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	QCoreApplication::processEvents();
}
} // namespace

extern "C" {
bool text_lookup_getstr(lookup_t *, const char *key, const char **out)
{
	*out = key;
	return true;
}
char *obs_module_get_config_path(obs_module_t *, const char *)
{
	return bstrdup(configuration.toUtf8().constData());
}
bool obs_enum_encoder_types(size_t, const char **) { return false; }
void obs_frontend_remove_event_callback(obs_frontend_event_cb, void *) { ++callbackRemovals; }
}

int main(int argc, char **argv)
{
	QApplication application(argc, argv);
	application.setQuitOnLastWindowClosed(false);
	QTemporaryDir directory;
	Require(directory.isValid(), "temporary config directory missing");
	configuration = directory.filePath("destinations.json");
	{
		QPointer<OutputDock> dock = new OutputDock(nullptr);
		// With the old stack/exec implementation, this deletes a stack wizard
		// inside its nested event loop before click() can return.
		QTimer::singleShot(0, &application, [dock]() { delete dock.data(); });
		Add(*dock)->click();
		Require(dock, "opening a wizard must not start a nested event loop");
		QPointer<QWizard> wizard = dock->findChild<QWizard *>();
		Require(wizard && wizard->isVisible(), "wizard not shown");
		QCoreApplication::processEvents();
		Require(!dock && !wizard, "parent destruction must safely destroy its active wizard");
	}
	{
		QPointer<OutputDock> dock = new OutputDock(nullptr);
		Add(*dock)->click();
		QPointer<QWizard> wizard = dock->findChild<QWizard *>();
		Require(wizard, "validation wizard missing");
		QTimer::singleShot(0, &application, [dock]() { delete dock.data(); });
		Require(!wizard->validateCurrentPage(), "missing encoder should fail validation");
		Require(dock, "a validation warning must not start a nested event loop");
		QPointer<QMessageBox> warning = wizard->findChild<QMessageBox *>();
		Require(warning && warning->isVisible(), "validation warning not shown");
		QCoreApplication::processEvents();
		Require(!dock && !wizard && !warning, "parent destruction must release nested warnings safely");
	}
	{
		auto dock = std::make_unique<OutputDock>(nullptr);
		Add(*dock)->click();
		QPointer<QWizard> wizard = dock->findChild<QWizard *>();
		Require(wizard, "wizard missing");
		Add(*dock)->click();
		Require(dock->findChildren<QWizard *>().size() == 1, "duplicate wizard opened");
		dock->Event(OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN);
		DrainDeletes();
		Require(!wizard && !QFile::exists(configuration), "shutdown must cancel the wizard without saving changes");
		Add(*dock)->click();
		Require(dock->findChildren<QWizard *>().isEmpty(), "shutdown must prevent reopening a wizard");
	}
	{
		auto dock = std::make_unique<OutputDock>(nullptr);
		dock->Event(OBS_FRONTEND_EVENT_FINISHED_LOADING);
		QElapsedTimer clock;
		clock.start();
		while (!dock->findChild<QWizard *>() && clock.elapsed() < 2000)
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
		QPointer<QWizard> wizard = dock->findChild<QWizard *>();
		Require(wizard && wizard->pageIds().size() == 2, "first-run assistant did not open");
		wizard->reject();
		DrainDeletes();
		Require(!wizard, "dismissed wizard was not released");
		QFile file(configuration);
		Require(file.open(QIODevice::ReadOnly), "normal dismissal did not save setup state");
		Require(QJsonDocument::fromJson(file.readAll()).object().value("setupShown").toBool(),
			"first-run dismissal must be remembered");
	}
	FrontendEvent(OBS_FRONTEND_EVENT_EXIT, nullptr);
	obs_module_unload();
	Require(callbackRemovals == 0, "module unload must not call the destroyed frontend after EXIT");
	std::cout << "PASS: asynchronous wizard/warnings, parent destruction, shutdown cancellation, duplicate guard, "
		"first-run dismissal and frontend unload\n";
}
