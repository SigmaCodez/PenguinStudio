// SPDX-License-Identifier: GPL-2.0-or-later
#include "../frontend/utility/StartupDialog.hpp"
#include <QEventLoop>
#include <cassert>
#include <cstdio>

static void Drain()
{
	for (int i = 0; i < 10; ++i) {
		QCoreApplication::processEvents();
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	}
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	QWidget window;
	int calls = 0;
	new StartupDialog(&window, [&]() {
		assert(window.isActiveWindow());
		assert(!window.property("penguinStartupDialogPending").toBool());
		++calls;
	});
	Drain();
	assert(calls == 0); // A hidden or background main window must not open a wizard.
	window.show();
	window.activateWindow();
	Drain();
	assert(calls == 1);
	window.hide();
	window.show();
	window.activateWindow();
	Drain();
	assert(calls == 1); // Repeated activation cannot open another first-run wizard.
	{
		auto *closing = new QWidget;
		new StartupDialog(closing, [&]() { ++calls; });
		delete closing;
	}
	Drain();
	assert(calls == 1); // Shutdown cancels the deferred callback.
	std::puts("PASS: startup activation, one-shot scheduling, and parent shutdown");
}
