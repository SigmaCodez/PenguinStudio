// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QApplication>
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include <QWindow>

#include <functional>
#include <utility>

// Wayland can deliver text-input enter before Qt processes the main window's
// initial activation. Opening a modal dialog in that interval races the two
// surfaces (QTBUG-141475). Wait for activation, then leave that event before
// opening the first-run dialog. No focus events or diagnostics are discarded.
class StartupDialog final : public QObject {
	QPointer<QWidget> window;
	std::function<void()> callback;
	bool queued = false;
	bool finished = false;

	void Queue()
	{
		if (queued || finished)
			return;
		queued = true;
		QTimer::singleShot(0, this, [this]() {
			queued = false;
			if (!window || !window->isVisible() || !window->isActiveWindow() || !window->windowHandle() ||
			    !window->windowHandle()->isActive() || QGuiApplication::focusWindow() != window->windowHandle())
				return;
			finished = true;
			window->setProperty("penguinStartupDialogPending", false);
			auto launch = std::move(callback);
			deleteLater();
			launch();
		});
	}

public:
	StartupDialog(QWidget *parent, std::function<void()> launch)
		: QObject(parent), window(parent), callback(std::move(launch))
	{
		window->setProperty("penguinStartupDialogPending", true);
		if (auto *handle = window->windowHandle())
			connect(handle, &QWindow::activeChanged, this, [this]() { Queue(); });
		connect(qApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *) { Queue(); });
		Queue();
	}

	~StartupDialog() override
	{
		if (window)
			window->setProperty("penguinStartupDialogPending", false);
	}
};
