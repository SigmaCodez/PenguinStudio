// SPDX-License-Identifier: GPL-2.0-or-later
#include "PenguinWorkspace.hpp"

#include "OBSBasic.hpp"
#include "PenguinInspector.hpp"
#include "AudioMixer.hpp"
#include <utility/PenguinIcons.hpp>

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolBar>
#include <QToolButton>

#include <algorithm>
#include <functional>

using PenguinStudio::Icon;
using PenguinStudio::StudioIcon;

namespace {
QToolButton *Button(QWidget *parent, const char *name, const QString &text, Icon icon, bool label = true)
{
	auto *button = new QToolButton(parent);
	button->setObjectName(QString::fromUtf8(name));
	button->setText(text);
	button->setToolTip(text);
	button->setAccessibleName(text);
	button->setIcon(StudioIcon(icon));
	button->setIconSize(QSize(18, 18));
	button->setToolButtonStyle(label ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
	button->setCursor(Qt::PointingHandCursor);
	return button;
}

QLabel *Label(QWidget *parent, const char *name, const QString &text = {})
{
	auto *label = new QLabel(text, parent);
	label->setObjectName(QString::fromUtf8(name));
	label->setTextFormat(Qt::PlainText);
	return label;
}

void Mirror(QToolButton *proxy, QAbstractButton *original)
{
	if (!original) {
		proxy->setEnabled(false);
		return;
	}
	proxy->setEnabled(original->isEnabled());
	proxy->setToolTip(original->text());
	proxy->setAccessibleName(original->text());
	proxy->setCheckable(original->isCheckable());
	proxy->setChecked(original->isChecked());
}
} // namespace

PenguinWorkspace::PenguinWorkspace(OBSBasic *window) : QObject(window), main(window), refreshTimer(this)
{
	main->setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
	inspectorDock = new QDockWidget(QTStr("Penguin.Inspector.Title"), main);
	inspectorDock->setObjectName(QStringLiteral("penguinInspectorDock"));
	inspectorDock->setMinimumWidth(246);
	inspector = new PenguinInspector(main, inspectorDock);
	inspectorDock->setWidget(inspector);
	main->AddDockWidget(inspectorDock, Qt::RightDockWidgetArea);
	CreateToolbar();
	CreateCanvasBar();
	CreatePanelHeaders();
	connect(main->ui->sources->selectionModel(), &QItemSelectionModel::selectionChanged, inspector,
		&PenguinInspector::RefreshSelection);
	connect(main->ui->scenes, &QListWidget::currentItemChanged, inspector, &PenguinInspector::RefreshSelection);
	connect(&refreshTimer, &QTimer::timeout, this, [this]() { Refresh(); });
}

void PenguinWorkspace::CreateToolbar()
{
	toolbar = new QToolBar(main);
	toolbar->setObjectName(QStringLiteral("penguinTopBar"));
	toolbar->setMovable(false);
	toolbar->setFloatable(false);
	toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
	auto *bar = new QWidget(toolbar);
	auto *row = new QHBoxLayout(bar);
	row->setContentsMargins(12, 8, 12, 8);
	row->setSpacing(8);
	bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	auto *brand = Button(bar, "penguinBrand", QStringLiteral("Penguin Studio"), Icon::Chevron);
	brand->setIcon(main->windowIcon());
	brand->setIconSize(QSize(24, 24));
	brand->setPopupMode(QToolButton::InstantPopup);
	auto *appMenu = new QMenu(brand);
	for (QAction *action : main->menuBar()->actions()) {
		if (action->menu())
			appMenu->addMenu(action->menu());
	}
	brand->setMenu(appMenu);
	row->addWidget(brand);
	RegisterMenuShortcuts();
#ifndef __APPLE__
	// The complete native menu tree stays available from the application button.
	main->menuBar()->hide();
#endif
	workspaceLabel = Label(bar, "penguinWorkspaceLabel", QTStr("Penguin.Workspace.Label"));
	row->addWidget(workspaceLabel);
	row->addStretch(1);
	metrics = Label(bar, "penguinMetric");
	row->addWidget(metrics);

	destinationsButton = Button(bar, "penguinDestinationsButton", QTStr("Penguin.Workspace.Destinations"), Icon::Live);
	connect(destinationsButton, &QToolButton::clicked, this, [this]() { ShowDock("penguin-broadcast-destinations"); });
	row->addWidget(destinationsButton);
	detailsButton = Button(bar, "penguinDetailsButton", QTStr("Penguin.Workspace.StreamDetails"), Icon::Inspect, false);
	connect(detailsButton, &QToolButton::clicked, this, [this]() { ShowDock("penguin-stream-details"); });
	row->addWidget(detailsButton);
	liveButton = Button(bar, "penguinLiveButton", QTStr("Penguin.Workspace.GoLive"), Icon::Live);
	liveButton->setPopupMode(QToolButton::MenuButtonPopup);
	auto *liveMenu = new QMenu(liveButton);
	liveMenu->addAction(QTStr("Penguin.Workspace.Destinations"), this,
			    [this]() { ShowDock("penguin-broadcast-destinations"); });
	liveMenu->addAction(QTStr("Penguin.Workspace.StreamDetails"), this,
			    [this]() { ShowDock("penguin-stream-details"); });
	liveMenu->addSeparator();
	controlActions.emplace_back(liveMenu->addAction(QTStr("Penguin.Workspace.PrimaryStream"), this,
		[this]() { ClickControl(QStringLiteral("streamButton")); }), QStringLiteral("streamButton"));
	liveButton->setMenu(liveMenu);
	connect(liveButton, &QToolButton::clicked, this, [this]() { ToggleLive(); });
	row->addWidget(liveButton);

	recordButton = Button(bar, "penguinRecordButton", QTStr("Penguin.Workspace.Record"), Icon::Record);
	connect(recordButton, &QToolButton::clicked, this, [this]() {
		ClickControl(QStringLiteral("recordButton"));
	});
	recordButton->setPopupMode(QToolButton::MenuButtonPopup);
	auto *recordMenu = new QMenu(recordButton);
	controlActions.emplace_back(recordMenu->addAction(QTStr("Penguin.Workspace.PauseRecording"), this,
		[this]() { ClickControl(QStringLiteral("pauseRecordButton")); }), QStringLiteral("pauseRecordButton"));
	recordButton->setMenu(recordMenu);
	row->addWidget(recordButton);

	clipButton = Button(bar, "penguinClipButton", QTStr("Penguin.Workspace.Clip"), Icon::Clip);
	connect(clipButton, &QToolButton::clicked, this, [this]() {
		ClickControl(QStringLiteral("saveReplayButton"));
	});
	row->addWidget(clipButton);
	auto *settings = Button(bar, "penguinSettingsButton", QTStr("Settings"), Icon::Settings, false);
	connect(settings, &QToolButton::clicked, this, [this]() { ClickControl(QStringLiteral("settingsButton")); });
	row->addWidget(settings);
	auto *more = Button(bar, "penguinToolbarButton", QTStr("Penguin.Workspace.More"), Icon::More, false);
	more->setPopupMode(QToolButton::InstantPopup);
	auto *moreMenu = new QMenu(more);
	for (const auto &entry : {std::pair{"replayBufferButton", "Penguin.Workspace.ReplayBuffer"},
				 std::pair{"virtualCamButton", "Penguin.Workspace.VirtualCamera"},
				 std::pair{"settingsButton", "Settings"}}) {
		const QString buttonName = QString::fromUtf8(entry.first);
		controlActions.emplace_back(moreMenu->addAction(QTStr(entry.second), this,
			[this, buttonName]() { ClickControl(buttonName); }), buttonName);
	}
	moreMenu->addSeparator();
	moreMenu->addAction(main->controlsDock->toggleViewAction());
	moreMenu->addAction(main->ui->transitionsDock->toggleViewAction());
	moreMenu->addAction(QTStr("Penguin.Workspace.Reset"), this, [this]() { main->on_resetDocks_triggered(); });
	more->setMenu(moreMenu);
	row->addWidget(more);
	toolbar->addWidget(bar);
	main->addToolBar(Qt::TopToolBarArea, toolbar);
}

void PenguinWorkspace::RegisterMenuShortcuts()
{
	// A hidden menu bar must not be the only widget associated with an action's
	// window shortcut. Reuse the same QAction, preserving enabled state and undo.
	std::function<void(QMenu *)> attach = [this, &attach](QMenu *menu) {
		for (QAction *action : menu->actions()) {
			if (action->menu())
				attach(action->menu());
			else if (!action->shortcuts().isEmpty() && !main->actions().contains(action))
				main->addAction(action);
		}
	};
	for (QAction *action : main->menuBar()->actions())
		if (action->menu())
			attach(action->menu());
}

void PenguinWorkspace::ClickControl(const QString &name)
{
	if (!ready || stopped || main->isClosing())
		return;
	auto *button = main->controlsDock->findChild<QAbstractButton *>(name);
	// Controls dock itself is intentionally hidden; test the button's own state.
	if (button && button->isEnabled() && !button->isHidden())
		button->click();
}

void PenguinWorkspace::CreateCanvasBar()
{
	auto *bar = new QWidget(main->ui->centralwidget);
	bar->setObjectName(QStringLiteral("penguinCanvasBar"));
	auto *row = new QHBoxLayout(bar);
	row->setContentsMargins(14, 8, 14, 8);
	row->setSpacing(10);
	auto *icon = Label(bar, "penguinCanvasIcon");
	icon->setPixmap(StudioIcon(Icon::Monitor).pixmap(18, 18));
	row->addWidget(icon);
	sceneName = Label(bar, "penguinCanvasTitle", QTStr("Penguin.Workspace.Canvas"));
	sceneName->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	row->addWidget(sceneName, 1);
	canvasSize = Label(bar, "penguinCanvasBadge");
	row->addWidget(canvasSize);
	canvasStatus = Label(bar, "penguinCanvasStatus", QTStr("Penguin.Workspace.Preview"));
	row->addWidget(canvasStatus);
	studioMode = Button(bar, "penguinCanvasMode", QTStr("Basic.TogglePreviewProgramMode"), Icon::Layers);
	studioMode->setCheckable(true);
	connect(studioMode, &QToolButton::clicked, main, &OBSBasic::TogglePreviewProgramMode);
	row->addWidget(studioMode);
	auto *inspect = Button(bar, "penguinInspectorButton", QTStr("Penguin.Inspector.Title"), Icon::Inspect, false);
	connect(inspect, &QToolButton::clicked, this, [this]() { ShowDock("penguinInspectorDock"); });
	row->addWidget(inspect);
	main->ui->verticalLayout->insertWidget(0, bar);
}

void PenguinWorkspace::CreatePanelHeaders()
{
	auto header = [this](QDockWidget *dock, const QString &title, Icon glyph, QToolBar *actions) {
		auto *bar = new QWidget(dock);
		bar->setObjectName(QStringLiteral("penguinPanelHeader"));
		auto *box = new QVBoxLayout(bar);
		box->setContentsMargins(0, 0, 0, 0);
		box->setSpacing(0);
		auto *row = new QHBoxLayout;
		box->addLayout(row);
		row->setContentsMargins(12, 10, 10, 8);
		row->setSpacing(8);
		auto *icon = Label(bar, "penguinPanelIcon");
		icon->setPixmap(StudioIcon(glyph).pixmap(18, 18));
		row->addWidget(icon);
		row->addWidget(Label(bar, "penguinPanelTitle", title), 1);
		if (dock == main->ui->scenesDock || dock == main->ui->sourcesDock) {
			auto *count = Label(bar, "penguinPanelCount");
			row->addWidget(count);
			if (dock == main->ui->scenesDock)
				sceneCount = count;
			else
				layerCount = count;
		}
		if (actions) {
			const auto entries = actions->actions();
			if (!entries.isEmpty()) {
				auto *add = Button(bar, "penguinPanelAction", entries.front()->text(), Icon::Plus, false);
				add->setDefaultAction(entries.front());
				add->setIcon(StudioIcon(Icon::Plus));
				row->addWidget(add);
			}
			auto *more = Button(bar, "penguinPanelAction", QTStr("Penguin.Workspace.More"), Icon::More, false);
			more->setPopupMode(QToolButton::InstantPopup);
			auto *menu = new QMenu(more);
			menu->addActions(entries);
			more->setMenu(menu);
			row->addWidget(more);
			actions->hide();
		}
		if (dock == main->ui->scenesDock || dock == main->ui->sourcesDock) {
			const bool scenes = dock == main->ui->scenesDock;
			auto *search = new QLineEdit(bar);
			search->setObjectName(scenes ? QStringLiteral("penguinSceneSearch") : QStringLiteral("penguinLayerSearch"));
			search->setPlaceholderText(QTStr(scenes ? "Penguin.Workspace.SearchScenes" : "Penguin.Workspace.SearchLayers"));
			search->setAccessibleName(search->placeholderText());
			search->setClearButtonEnabled(true);
			search->addAction(StudioIcon(Icon::Search), QLineEdit::LeadingPosition);
			auto *searchRow = new QHBoxLayout;
			searchRow->setContentsMargins(10, 0, 10, 10);
			searchRow->addWidget(search);
			box->addLayout(searchRow);
			(scenes ? sceneSearch : layerSearch) = search;
			connect(search, &QLineEdit::textChanged, this, [this]() { FilterLists(); });
		}
		dock->setTitleBarWidget(bar);
	};
	header(main->ui->scenesDock, QTStr("Basic.Main.Scenes"), Icon::Scene, main->ui->scenesToolbar);
	header(main->ui->sourcesDock, QTStr("Penguin.Workspace.Layers"), Icon::Layers, main->ui->sourcesToolbar);
	header(main->ui->mixerDock, QTStr("Penguin.Workspace.AudioMixer"), Icon::Audio, nullptr);
	header(inspectorDock, QTStr("Penguin.Inspector.Title"), Icon::Inspect, nullptr);
}

void PenguinWorkspace::Initialize()
{
	ready = true;
	if (auto *dock = main->findChild<QDockWidget *>(QStringLiteral("penguin-broadcast-destinations")))
		dock->setWindowTitle(QTStr("Penguin.Workspace.OutputTab"));
	if (auto *dock = main->findChild<QDockWidget *>(QStringLiteral("penguin-stream-details")))
		dock->setWindowTitle(QTStr("Penguin.Workspace.DetailsTab"));
	RegisterMenuShortcuts();
	config_t *config = App()->GetUserConfig();
	if (config_get_int(config, "BasicWindow", "PenguinWorkspaceVersion") < 3) {
		const QString previous = QString::fromUtf8(config_get_string(config, "BasicWindow", "DockState"));
		if (!previous.isEmpty()) {
			if (!config_has_user_value(config, "BasicWindow", "PenguinPreviousDockState"))
				config_set_string(config, "BasicWindow", "PenguinPreviousDockState", previous.toUtf8().constData());
			config_set_string(config, "BasicWindow", "PenguinPreV3DockState", previous.toUtf8().constData());
		}
		ResetLayout();
		// Persist the layout and its migration marker together. Otherwise a crash
		// before normal shutdown can retain the old layout with the new version.
		config_set_string(config, "BasicWindow", "DockState", main->saveState().toBase64().constData());
		config_set_int(config, "BasicWindow", "PenguinWorkspaceVersion", 3);
		config_save_safe(config, "tmp", nullptr);
	}
	toolbar->show();
	Refresh();
	refreshTimer.start(250);
}

void PenguinWorkspace::ResetLayout()
{
	main->ui->sideDocks->setChecked(true);
	main->setDockCornersVertical(true);
	for (auto *dock : {static_cast<QDockWidget *>(main->ui->scenesDock),
			   static_cast<QDockWidget *>(main->ui->sourcesDock),
			   static_cast<QDockWidget *>(main->ui->mixerDock), inspectorDock}) {
		dock->setFloating(false);
		main->removeDockWidget(dock);
	}
	main->addDockWidget(Qt::LeftDockWidgetArea, main->ui->sourcesDock);
	main->splitDockWidget(main->ui->sourcesDock, main->ui->scenesDock, Qt::Vertical);
	main->addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
	main->addDockWidget(Qt::BottomDockWidgetArea, main->ui->mixerDock);
	main->ui->scenesDock->show();
	main->ui->sourcesDock->show();
	main->ui->mixerDock->show();
	inspectorDock->show();
	main->controlsDock->hide();
	main->ui->transitionsDock->hide();
	main->statsDock->hide();
	// Source properties and filters are available in the Inspector. The optional
	// legacy toolbar duplicates them and imposes a wide minimum at large fonts.
	main->HideContextBar();
	main->setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
	for (const char *id : {"penguin-broadcast-destinations", "penguin-stream-details"})
		if (auto *dock = main->findChild<QDockWidget *>(QString::fromUtf8(id))) {
			dock->setFloating(false);
			main->removeDockWidget(dock);
			main->addDockWidget(Qt::RightDockWidgetArea, dock);
			main->tabifyDockWidget(inspectorDock, dock);
			dock->show();
		}
	inspectorDock->raise();
	main->resizeDocks({main->ui->sourcesDock, inspectorDock}, {230, 340}, Qt::Horizontal);
	main->resizeDocks({main->ui->scenesDock, main->ui->sourcesDock}, {240, 400}, Qt::Vertical);
	main->resizeDocks({main->ui->mixerDock}, {240}, Qt::Vertical);
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolumeControl", true);
	if (auto *mixer = main->ui->mixerDock->findChild<AudioMixer *>())
		mixer->setMixerLayoutVertical(true);
	sceneSearch->clear();
	layerSearch->clear();
	toolbar->show();
}

void PenguinWorkspace::FilterLists()
{
	if (stopped || !sceneSearch || !layerSearch)
		return;
	const QString sceneQuery = sceneSearch->text().trimmed();
	for (int row = 0; row < main->ui->scenes->count(); ++row) {
		auto *item = main->ui->scenes->item(row);
		item->setHidden(!item->text().contains(sceneQuery, Qt::CaseInsensitive));
	}
	// Reordering a filtered list would make the resulting source order ambiguous.
	main->ui->scenes->setDragEnabled(sceneQuery.isEmpty());
	main->ui->scenes->setAcceptDrops(sceneQuery.isEmpty());
	const QString layerQuery = layerSearch->text().trimmed();
	const int layers = main->ui->sources->model()->rowCount();
	for (int row = 0; row < layers; ++row) {
		const OBSSceneItem item = main->ui->sources->Get(row);
		const auto *source = item ? obs_sceneitem_get_source(item) : nullptr;
		const QString name = source ? QString::fromUtf8(obs_source_get_name(source)) : QString();
		main->ui->sources->setRowHidden(row, !name.contains(layerQuery, Qt::CaseInsensitive));
	}
	main->ui->sources->setDragEnabled(layerQuery.isEmpty());
	main->ui->sources->setAcceptDrops(layerQuery.isEmpty());
	sceneCount->setText(QString::number(main->ui->scenes->count()));
	layerCount->setText(QString::number(layers));
}

void PenguinWorkspace::ShowDock(const char *id)
{
	if (stopped || main->isClosing())
		return;
	if (auto *dock = main->findChild<QDockWidget *>(QString::fromUtf8(id))) {
		dock->show();
		dock->raise();
		dock->activateWindow();
	}
}

void PenguinWorkspace::ToggleLive()
{
	if (!ready || stopped || main->isClosing())
		return;
	// The toolbar delegates to the same validated output actions as the destination panel.
	auto *outputs = main->findChild<QWidget *>(QStringLiteral("penguinOutputPanel"));
	if (!outputs || outputs->property("penguinDestinationCount").toInt() == 0) {
		ShowDock("penguin-broadcast-destinations");
		if (outputs)
			if (auto *add = outputs->findChild<QPushButton *>(QStringLiteral("penguinAddDestination")))
				add->click();
		return;
	}
	const char *action = outputs->property("penguinActiveCount").toInt() > 0 ? "penguinStopAll" : "penguinStartAll";
	if (auto *button = outputs->findChild<QPushButton *>(QString::fromUtf8(action)))
		button->click();
	Refresh();
}

void PenguinWorkspace::Refresh()
{
	if (!ready || stopped)
		return;
	OBSScene scene = main->GetCurrentScene();
	const char *name = scene ? obs_source_get_name(obs_scene_get_source(scene)) : nullptr;
	sceneName->setText(name ? QString::fromUtf8(name) : QTStr("Penguin.Workspace.Canvas"));
	sceneName->setToolTip(sceneName->text());
	obs_video_info video{};
	if (obs_get_video_info(&video))
		canvasSize->setText(QStringLiteral("%1 × %2").arg(video.base_width).arg(video.base_height));
	metrics->setText(QStringLiteral("%1 FPS").arg(obs_get_active_fps(), 0, 'f', 0));
	metrics->setVisible(main->width() >= 1180);
	workspaceLabel->setVisible(main->width() >= 1440);
	detailsButton->setVisible(main->width() >= 1280);
	FilterLists();
	destinationsButton->setToolButtonStyle(main->width() >= 1180 ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
	Mirror(studioMode, main->controlsDock->findChild<QAbstractButton *>("modeSwitch"));
	studioMode->setCheckable(true);
	studioMode->setChecked(main->IsPreviewProgramMode());
	Mirror(recordButton, main->controlsDock->findChild<QAbstractButton *>("recordButton"));
	const bool recording = obs_frontend_recording_active();
	canvasStatus->setText(QTStr(recording ? "Penguin.Workspace.Recording" : "Penguin.Workspace.Preview"));
	recordButton->setText(recording ? QTStr("Penguin.Workspace.StopRecording") : QTStr("Penguin.Workspace.Record"));
	recordButton->setCheckable(true);
	recordButton->setChecked(recording);
	const auto *saveReplay = main->controlsDock->findChild<QAbstractButton *>("saveReplayButton");
	clipButton->setEnabled(obs_frontend_replay_buffer_active() && saveReplay && saveReplay->isEnabled() && !saveReplay->isHidden());
	clipButton->setToolTip(QTStr(obs_frontend_replay_buffer_active() ? "Penguin.Workspace.SaveClip"
								     : "Penguin.Workspace.EnableReplay"));
	auto *outputs = main->findChild<QWidget *>(QStringLiteral("penguinOutputPanel"));
	destinationsButton->setEnabled(outputs != nullptr);
	liveButton->setEnabled(outputs != nullptr);
	const bool live = outputs && outputs->property("penguinActiveCount").toInt() > 0;
	liveButton->setIcon(StudioIcon(Icon::Live, QColor("#FFFFFF")));
	liveButton->setText(QTStr(live ? "Penguin.Workspace.EndStream" : "Penguin.Workspace.GoLive"));
	liveButton->setToolTip(liveButton->text());
	liveButton->setAccessibleName(liveButton->text());
	liveButton->setCheckable(true);
	liveButton->setChecked(live);
	for (const auto &[action, name] : controlActions) {
		if (!action)
			continue;
		const auto *original = main->controlsDock->findChild<QAbstractButton *>(name);
		action->setEnabled(original && original->isEnabled() && !original->isHidden());
	}
}

void PenguinWorkspace::Shutdown()
{
	stopped = true;
	refreshTimer.stop();
	toolbar->setEnabled(false);
	studioMode->setEnabled(false);
	for (const auto &[action, name] : controlActions) {
		(void)name;
		if (action)
			action->setEnabled(false);
	}
	inspector->Shutdown();
}
