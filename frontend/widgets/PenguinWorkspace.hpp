// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <utility>
#include <vector>

class OBSBasic;
class PenguinInspector;
class QDockWidget;
class QLabel;
class QToolButton;
class QToolBar;
class QAction;
class QLineEdit;

// Owns the workspace presentation. Output and source state remain owned by OBS.
class PenguinWorkspace final : public QObject {
	OBSBasic *main;
	PenguinInspector *inspector;
	QDockWidget *inspectorDock;
	QToolBar *toolbar;
	QLabel *sceneName;
	QLabel *canvasSize;
	QLabel *metrics;
	QLabel *workspaceLabel;
	QLabel *canvasStatus;
	QLabel *sceneCount = nullptr;
	QLabel *layerCount = nullptr;
	QLineEdit *sceneSearch = nullptr;
	QLineEdit *layerSearch = nullptr;
	QToolButton *studioMode;
	QToolButton *liveButton;
	QToolButton *recordButton;
	QToolButton *clipButton;
	QToolButton *destinationsButton;
	std::vector<std::pair<QPointer<QAction>, QString>> controlActions;
	QTimer refreshTimer;
	bool ready = false;
	bool stopped = false;

	void CreateToolbar();
	void CreateCanvasBar();
	void CreatePanelHeaders();
	void Refresh();
	void FilterLists();
	void ShowDock(const char *id);
	void ToggleLive();
	void ClickControl(const QString &name);
	void RegisterMenuShortcuts();

public:
	explicit PenguinWorkspace(OBSBasic *main);
	void Initialize();
	void ResetLayout();
	void Shutdown();
};
