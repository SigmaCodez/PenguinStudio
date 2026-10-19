// SPDX-License-Identifier: GPL-2.0-or-later
// Optional LD_PRELOAD integration helper. Run only with an isolated test profile.
// X11 screenshots include native video surfaces. Wayland captures are explicitly
// labeled Qt widget renders; decoded recordings validate native video content.
#include <obs.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/config-file.h>

#include <QApplication>
#include <QAbstractButton>
#include <QAction>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QKeyEvent>
#include <QTabBar>
#include <QStyleOptionTab>
#include <QListView>
#include <QListWidget>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QMessageBox>
#include <QPointer>
#include <QScreen>
#include <QScrollArea>
#include <QSet>
#include <QSpinBox>
#include <QTimer>
#include <QWindow>
#include <QWizard>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#undef KeyPress
#undef KeyRelease
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <utility>

namespace {
QPointer<QMainWindow> mainWindow;
int ticks = 0;
QJsonArray checks;
QJsonArray tabMeasurements;
QJsonArray capturedLayouts;
obs_sceneitem_t *fixtureItem = nullptr;
obs_sceneitem_t *backgroundItem = nullptr;
QString fixtureSceneUuid;
obs_scene_t *alternateScene = nullptr;
std::vector<obs_sceneitem_t *> audioFixtures;

void Check(bool passed, const QString &name)
{
	checks.append(QJsonObject{{QStringLiteral("name"), name}, {QStringLiteral("passed"), passed}});
	qInfo("Workspace check: %s: %s", qPrintable(name), passed ? "PASS" : "FAIL");
	QFile progress(QDir(QString::fromLocal8Bit(qgetenv("PENGUIN_SMOKE_DIR"))).filePath(QStringLiteral("checks-progress.json")));
	if (progress.open(QIODevice::WriteOnly))
		progress.write(QJsonDocument(checks).toJson());
}

QDir Results()
{
	return QDir(QString::fromLocal8Bit(qgetenv("PENGUIN_SMOKE_DIR")));
}

void SaveReport()
{
	QJsonArray widgets;
	for (auto *widget : mainWindow->findChildren<QWidget *>()) {
		if (widget->objectName().isEmpty())
			continue;
		const QRect bounds(widget->mapTo(mainWindow, QPoint()), widget->size());
		widgets.append(QJsonObject{{QStringLiteral("name"), widget->objectName()},
				   {QStringLiteral("class"), QString::fromLatin1(widget->metaObject()->className())},
				   {QStringLiteral("visible"), widget->isVisible()},
				   {QStringLiteral("x"), bounds.x()}, {QStringLiteral("y"), bounds.y()},
				   {QStringLiteral("width"), bounds.width()},
				   {QStringLiteral("height"), bounds.height()}});
	}
	QFile report(Results().filePath(QStringLiteral("workspace-checks.json")));
	const QRect available = mainWindow->screen()->availableGeometry();
	config_t *config = obs_frontend_get_user_config();
	if (report.open(QIODevice::WriteOnly))
		report.write(QJsonDocument(QJsonObject{{QStringLiteral("platform"), QGuiApplication::platformName()},
						     {QStringLiteral("windowWidth"), mainWindow->width()},
						     {QStringLiteral("windowHeight"), mainWindow->height()},
						     {QStringLiteral("screenWidth"), available.width()},
						     {QStringLiteral("screenHeight"), available.height()},
						     {QStringLiteral("devicePixelRatio"), mainWindow->devicePixelRatioF()},
						     {QStringLiteral("workspaceVersion"), int(config_get_int(config, "BasicWindow", "PenguinWorkspaceVersion"))},
						     {QStringLiteral("previousDockStateSaved"), config_has_user_value(config, "BasicWindow", "PenguinPreviousDockState")},
						     {QStringLiteral("captureMethod"), QGuiApplication::platformName() == QStringLiteral("xcb")
							      ? QStringLiteral("native-x11-window") : QStringLiteral("qt-widget-render")},
						     {QStringLiteral("checks"), checks},
						     {QStringLiteral("sidebarTabs"), tabMeasurements},
						     {QStringLiteral("capturedLayouts"), capturedLayouts},
						     {QStringLiteral("widgets"), widgets}})
				     .toJson());
}

void Capture(const QString &filename)
{
	capturedLayouts.append(QJsonObject{{"image", filename}, {"width", mainWindow->width()},
					   {"height", mainWindow->height()}, {"minimumWidth", mainWindow->minimumWidth()},
					   {"minimumHeight", mainWindow->minimumHeight()},
					   {"minimumHintWidth", mainWindow->minimumSizeHint().width()},
					   {"minimumHintHeight", mainWindow->minimumSizeHint().height()}});
	if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
		const QString widgetFilename = QFileInfo(filename).completeBaseName() + QStringLiteral("-widgets.png");
		Check(mainWindow->grab().save(Results().filePath(widgetFilename)),
		      QStringLiteral("native-wayland-widget-render"));
		return;
	}
	mainWindow->raise();
	Display *display = XOpenDisplay(nullptr);
	if (!display) {
		Check(false, QStringLiteral("native-window-capture"));
		return;
	}
	const Pixmap pixmap = XCompositeNameWindowPixmap(display, mainWindow->winId());
	XSync(display, False);
	Window root{};
	int x{}, y{};
	unsigned int width{}, height{}, border{}, depth{};
	XGetGeometry(display, pixmap, &root, &x, &y, &width, &height, &border, &depth);
	QFile geometry(Results().filePath(filename + QStringLiteral(".geometry.json")));
	if (geometry.open(QIODevice::WriteOnly))
		geometry.write(QJsonDocument(QJsonObject{{"qtWidth", mainWindow->width()}, {"qtHeight", mainWindow->height()},
			{"pixmapWidth", int(width)}, {"pixmapHeight", int(height)}, {"screenWidth", mainWindow->screen()->availableGeometry().width()},
			{"screenHeight", mainWindow->screen()->availableGeometry().height()}, {"devicePixelRatio", mainWindow->devicePixelRatioF()}}).toJson());
	XImage *pixels = XGetImage(display, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
	bool saved = false;
	if (pixels) {
		QImage image(reinterpret_cast<const uchar *>(pixels->data), pixels->width, pixels->height,
			     pixels->bytes_per_line, QImage::Format_RGB32);
		QSet<QRgb> colors;
		for (int row = 0; row < image.height(); row += 16)
			for (int column = 0; column < image.width(); column += 16)
				colors.insert(image.pixel(column, row));
		saved = colors.size() > 8 && image.save(Results().filePath(filename));
		XDestroyImage(pixels);
	}
	XFreePixmap(display, pixmap);
	XCloseDisplay(display);
	Check(saved, QStringLiteral("native-window-capture"));
}

void AddFixture()
{
	obs_source_t *current = obs_frontend_get_current_scene();
	obs_scene_t *scene = obs_scene_from_source(current);
	Check(scene != nullptr, QStringLiteral("current-scene-available"));
	if (!scene) {
		obs_source_release(current);
		return;
	}
	obs_video_info video{};
	obs_get_video_info(&video);
	obs_data_t *settings = obs_data_create();
	obs_data_set_int(settings, "width", video.base_width);
	obs_data_set_int(settings, "height", video.base_height);
	obs_data_set_int(settings, "color", 0xff30221b);
	obs_source_t *background = obs_source_create("color_source_v3", "Validation background", settings, nullptr);
	backgroundItem = background ? obs_scene_add(scene, background) : nullptr;
	Check(backgroundItem != nullptr, QStringLiteral("real-obs-background-created"));
	obs_source_release(background);
	const QByteArray browserFile = qgetenv("PENGUIN_SMOKE_BROWSER");
	if (!browserFile.isEmpty()) {
		obs_data_t *browserSettings = obs_data_create();
		obs_data_set_bool(browserSettings, "is_local_file", true);
		obs_data_set_string(browserSettings, "local_file", browserFile.constData());
		obs_data_set_int(browserSettings, "width", video.base_width);
		obs_data_set_int(browserSettings, "height", video.base_height);
		obs_source_t *browser =
			obs_source_create("browser_source", "Local browser fixture", browserSettings, nullptr);
		obs_data_release(browserSettings);
		Check(browser && obs_scene_add(scene, browser), QStringLiteral("browser-source-created"));
		obs_source_release(browser);
	}
	for (uint32_t channel = 1; channel <= 6; ++channel)
		obs_set_output_source(channel, nullptr);
	for (const char *name : {"Microphone test tone", "Desktop test tone"}) {
		// Sources retain their settings; do not mutate another fixture's object.
		obs_data_release(settings);
		settings = obs_data_create();
		const auto file = Results().filePath(QString::fromUtf8(name) + QStringLiteral(".wav")).toUtf8();
		obs_data_set_bool(settings, "is_local_file", true);
		obs_data_set_bool(settings, "looping", true);
		obs_data_set_string(settings, "local_file", file.constData());
		obs_source_t *tone = obs_source_create("ffmpeg_source", name, settings, nullptr);
		auto *audioItem = tone ? obs_scene_add(scene, tone) : nullptr;
		Check(audioItem != nullptr, QStringLiteral("real-audio-fixture-%1").arg(QString::fromUtf8(name)));
		if (audioItem)
			audioFixtures.push_back(audioItem);
		obs_source_release(tone);
	}
	obs_data_release(settings);
	settings = obs_data_create();
	const QByteArray image = qgetenv("PENGUIN_SMOKE_IMAGE");
	obs_data_set_string(settings, "file", image.constData());
	obs_source_t *logo = obs_source_create("image_source", "Penguin Studio logo", settings, nullptr);
	obs_sceneitem_t *item = logo ? obs_scene_add(scene, logo) : nullptr;
	fixtureItem = item;
	fixtureSceneUuid = QString::fromUtf8(obs_source_get_uuid(current));
	Check(item != nullptr, QStringLiteral("real-obs-image-created"));
	if (item) {
		vec2 scale{0.5f, 0.5f};
		vec2 position{float(video.base_width) / 2.0f - 313.5f, float(video.base_height) / 2.0f - 313.5f};
		obs_sceneitem_set_scale(item, &scale);
		obs_sceneitem_set_pos(item, &position);
		obs_sceneitem_select(item, true);
	}
	obs_source_release(logo);
	obs_data_release(settings);
	obs_source_release(current);
}

template<typename T> T *Control(const char *name)
{
	auto *control = mainWindow->findChild<T *>(QString::fromLatin1(name));
	Check(control != nullptr, QStringLiteral("control-%1").arg(QString::fromLatin1(name)));
	return control;
}

QRect BoundsInWindow(QWidget *widget)
{
	return {widget->mapTo(mainWindow, QPoint()), widget->size()};
}

void RaiseInspector()
{
	if (auto *dock = mainWindow->findChild<QDockWidget *>(QStringLiteral("penguinInspectorDock")))
		dock->raise();
}

void CheckWorkspaceLayout(const QString &size, int requestedWidth, int requestedHeight)
{
	auto *layers = Control<QDockWidget>("sourcesDock");
	auto *scenes = Control<QDockWidget>("scenesDock");
	auto *mixer = Control<QDockWidget>("mixerDock");
	auto *inspector = Control<QDockWidget>("penguinInspectorDock");
	auto *topbar = Control<QWidget>("penguinTopBar");
	auto *canvas = Control<QWidget>("centralwidget");
	if (!layers || !scenes || !mixer || !inspector || !topbar || !canvas)
		return;
	Check(mainWindow->width() <= requestedWidth && mainWindow->height() <= requestedHeight,
	      QStringLiteral("workspace-fits-requested-size-%1").arg(size));
	Check(mainWindow->dockWidgetArea(layers) == Qt::LeftDockWidgetArea &&
	      mainWindow->dockWidgetArea(scenes) == Qt::LeftDockWidgetArea &&
	      BoundsInWindow(layers).bottom() < BoundsInWindow(scenes).top(),
	      QStringLiteral("layers-above-scenes-%1").arg(size));
	Check(mainWindow->dockWidgetArea(mixer) == Qt::BottomDockWidgetArea &&
	      BoundsInWindow(mixer).top() > BoundsInWindow(canvas).top() && mixer->height() >= 90,
	      QStringLiteral("mixer-below-canvas-%1").arg(size));
	Check(mainWindow->dockWidgetArea(inspector) == Qt::RightDockWidgetArea &&
	      BoundsInWindow(canvas).right() < BoundsInWindow(inspector).left(),
	      QStringLiteral("inspector-right-of-canvas-%1").arg(size));
	Check(canvas->width() >= 320 && canvas->height() >= 240,
	      QStringLiteral("usable-preview-space-%1").arg(size));
	for (const char *name : {"penguinBrand", "penguinLiveButton", "penguinRecordButton", "penguinToolbarButton"}) {
		auto *button = Control<QAbstractButton>(name);
		Check(button && button->isVisible() && BoundsInWindow(topbar).contains(BoundsInWindow(button)),
		      QStringLiteral("toolbar-control-fits-%1-%2").arg(size, QString::fromLatin1(name)));
	}
	for (const char *name : {"penguinLayerSearch", "penguinSceneSearch"}) {
		auto *search = Control<QLineEdit>(name);
		if (!search)
			continue;
		QStyleOptionFrame option;
		option.initFrom(search);
		const QRect content = search->style()->subElementRect(QStyle::SE_LineEditContents, &option, search);
		Check(search->isVisible() && !search->placeholderText().isEmpty() &&
		      !search->accessibleName().isEmpty() && content.height() >= search->fontMetrics().height(),
		      QStringLiteral("search-field-readable-%1-%2").arg(size, QString::fromLatin1(name)));
	}
}

void TestSearch(QListView *view, QLineEdit *search, const QString &query, int nameRole, const QString &label)
{
	if (!view || !search)
		return;
	const int originalRows = view->model()->rowCount();
	const QModelIndexList selected = view->selectionModel()->selectedIndexes();
	search->setText(query.toUpper());
	Check(!view->dragEnabled() && !view->acceptDrops(), QStringLiteral("%1-filter-prevents-ambiguous-reorder").arg(label));
	int matches = 0;
	bool correctRows = originalRows > 0;
	for (int row = 0; row < originalRows; ++row) {
		const QString name = view->model()->index(row, 0).data(nameRole).toString();
		const bool match = name.contains(query, Qt::CaseInsensitive);
		matches += match;
		correctRows = correctRows && view->isRowHidden(row) != match;
	}
	Check(correctRows && matches > 0, QStringLiteral("%1-search-matches-case-insensitively").arg(label));
	search->setText(QStringLiteral("Penguin nonexistent fixture 58c30c"));
	bool noMatches = true;
	for (int row = 0; row < originalRows; ++row)
		noMatches = noMatches && view->isRowHidden(row);
	Check(noMatches && view->model()->rowCount() == originalRows,
	      QStringLiteral("%1-search-hides-without-removing-items").arg(label));
	search->clear();
	bool restored = view->model()->rowCount() == originalRows;
	for (int row = 0; row < originalRows; ++row)
		restored = restored && !view->isRowHidden(row);
	Check(restored && selected == view->selectionModel()->selectedIndexes(),
	      QStringLiteral("%1-search-clear-preserves-items-and-selection").arg(label));
	Check(view->dragEnabled() && view->acceptDrops(), QStringLiteral("%1-search-clear-restores-reorder").arg(label));
}

void TestWorkspaceSearch()
{
	TestSearch(Control<QListView>("sources"), Control<QLineEdit>("penguinLayerSearch"),
		   QStringLiteral("Penguin Studio logo"), Qt::AccessibleTextRole, QStringLiteral("layers"));
	auto *scenes = Control<QListWidget>("scenes");
	if (scenes && scenes->currentItem())
		TestSearch(scenes, Control<QLineEdit>("penguinSceneSearch"), scenes->currentItem()->text(),
			   Qt::DisplayRole, QStringLiteral("scenes"));
	Check(fixtureItem && obs_sceneitem_visible(fixtureItem) && obs_sceneitem_selected(fixtureItem),
	      QStringLiteral("layer-filter-does-not-change-rendered-scene"));
}

void TestSceneRename()
{
	auto *scenes = Control<QListWidget>("scenes");
	if (!scenes || !scenes->currentItem())
		return;
	const QString original = scenes->currentItem()->text();
	auto edit = [scenes](const QString &text, int key, const QString &label) {
		// Use the actual Rename action slot: ordinary scene items deliberately
		// omit ItemIsEditable until this route temporarily enables editing.
		Check(QMetaObject::invokeMethod(mainWindow, "EditSceneName", Qt::DirectConnection),
		      QStringLiteral("scene-rename-action-%1").arg(label));
		QCoreApplication::processEvents();
		QLineEdit *editor = nullptr;
		for (auto *candidate : scenes->findChildren<QLineEdit *>())
			if (candidate->isVisible())
				editor = candidate;
		Check(editor != nullptr, QStringLiteral("scene-rename-editor-%1").arg(label));
		if (!editor)
			return;
		editor->selectAll();
		editor->insert(text);
		QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
		QCoreApplication::sendEvent(editor, &event);
		QCoreApplication::processEvents();
	};
	edit(QStringLiteral("Penguin cancelled rename fixture"), Qt::Key_Escape, QStringLiteral("cancel"));
	Check(scenes->currentItem()->text() == original, QStringLiteral("scene-rename-escape-preserves-name"));
	const QString renamed = QStringLiteral("Penguin renamed scene fixture");
	edit(renamed, Qt::Key_Return, QStringLiteral("commit"));
	obs_source_t *scene = obs_get_source_by_uuid(fixtureSceneUuid.toUtf8().constData());
	Check(scene && scenes->currentItem()->text() == renamed &&
	      QString::fromUtf8(obs_source_get_name(scene)) == renamed,
	      QStringLiteral("scene-rename-editor-updates-obs-source"));
	edit(original, Qt::Key_Return, QStringLiteral("restore"));
	Check(scene && scenes->currentItem()->text() == original &&
	      QString::fromUtf8(obs_source_get_name(scene)) == original,
	      QStringLiteral("scene-rename-editor-restores-name"));
	obs_source_release(scene);
}

void CheckSidebarTabText(const QString &size)
{
	auto *inspector = mainWindow->findChild<QDockWidget *>(QStringLiteral("penguinInspectorDock"));
	if (!inspector)
		return;
	bool found = false;
	for (auto *tabs : mainWindow->findChildren<QTabBar *>()) {
		bool inspectorTab = false;
		for (int index = 0; index < tabs->count(); ++index)
			inspectorTab = inspectorTab || tabs->tabText(index) == inspector->windowTitle();
		if (!inspectorTab || !tabs->isVisible())
			continue;
		found = true;
		for (int index = 0; index < tabs->count(); ++index) {
			QStyleOptionTab option;
			option.initFrom(tabs);
			option.rect = tabs->tabRect(index);
			option.text = tabs->tabText(index);
			option.shape = tabs->shape();
			const QRect text = tabs->style()->subElementRect(QStyle::SE_TabBarTabText, &option, tabs);
			const int textWidth = tabs->fontMetrics().horizontalAdvance(option.text);
			tabMeasurements.append(QJsonObject{{"size", size}, {"index", index}, {"text", option.text},
						  {"textWidth", textWidth}, {"availableWidth", text.width()},
						  {"tabWidth", option.rect.width()}, {"barWidth", tabs->width()}});
			Check(textWidth <= text.width(),
			      QStringLiteral("sidebar-tab-label-fits-%1-%2").arg(size).arg(index));
		}
	}
	Check(found, QStringLiteral("sidebar-tab-bar-visible-%1").arg(size));
}

void TestRightSidebar()
{
	auto *inspector = Control<QDockWidget>("penguinInspectorDock");
	if (!inspector)
		return;
	for (const auto &entry : {std::pair{"penguin-broadcast-destinations", "penguinDestinationsButton"},
				  std::pair{"penguin-stream-details", "penguinDetailsButton"}}) {
		const char *name = entry.first;
		auto *dock = Control<QDockWidget>(name);
		if (!dock)
			continue;
		Check(!dock->isFloating() && mainWindow->dockWidgetArea(dock) == Qt::RightDockWidgetArea &&
		      mainWindow->tabifiedDockWidgets(inspector).contains(dock),
		      QStringLiteral("integrated-sidebar-tab-%1").arg(QString::fromLatin1(name)));
		if (auto *button = Control<QAbstractButton>(entry.second))
			button->click();
		QCoreApplication::processEvents();
		Check(!dock->visibleRegion().isEmpty(), QStringLiteral("sidebar-tab-can-open-%1").arg(QString::fromLatin1(name)));
	}
	if (auto *button = Control<QAbstractButton>("penguinInspectorButton"))
		button->click();
	QCoreApplication::processEvents();
	Check(!inspector->visibleRegion().isEmpty(), QStringLiteral("inspector-tab-can-restore"));
}

bool CloseEnough(float actual, float expected)
{
	return std::abs(actual - expected) < 0.1f;
}

void Trigger(const char *name)
{
	if (auto *action = Control<QAction>(name))
		action->trigger();
}

void Click(const char *name)
{
	if (auto *button = Control<QAbstractButton>(name))
		button->click();
}

void TestInspector()
{
	if (!fixtureItem)
		return;
	obs_transform_info original{};
	obs_sceneitem_get_info2(fixtureItem, &original);
	auto *x = Control<QDoubleSpinBox>("inspectorX");
	if (!x)
		return;
	Check(x->isEnabled() && x->isVisible(), QStringLiteral("single-selection-editable"));
	x->setValue(123);
	obs_transform_info changed{};
	obs_sceneitem_get_info2(fixtureItem, &changed);
	Check(CloseEnough(changed.pos.x, 123), QStringLiteral("position-edit-reaches-obs"));
	if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
		mainWindow->activateWindow();
		mainWindow->setFocus();
		Display *display = XOpenDisplay(nullptr);
		XEvent event{};
		event.xkey.display = display;
		event.xkey.window = mainWindow->winId();
		event.xkey.root = DefaultRootWindow(display);
		event.xkey.same_screen = True;
		event.xkey.keycode = XKeysymToKeycode(display, XStringToKeysym("z"));
		event.xkey.state = ControlMask;
		event.xkey.type = 2; // X11 KeyPress (macro conflicts with Qt).
		XSendEvent(display, mainWindow->winId(), True, KeyPressMask, &event);
		event.xkey.type = 3; // X11 KeyRelease.
		XSendEvent(display, mainWindow->winId(), True, KeyReleaseMask, &event);
		XFlush(display);
		QElapsedTimer elapsed;
		elapsed.start();
		while (elapsed.elapsed() < 100) {
			QCoreApplication::processEvents();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		XCloseDisplay(display);
	} else {
		Trigger("actionMainUndo");
	}
	obs_sceneitem_get_info2(fixtureItem, &changed);
	Check(CloseEnough(changed.pos.x, original.pos.x), QStringLiteral("position-undo"));
	Trigger("actionMainRedo");
	obs_sceneitem_get_info2(fixtureItem, &changed);
	Check(CloseEnough(changed.pos.x, 123), QStringLiteral("position-redo"));
	if (auto *rotation = Control<QDoubleSpinBox>("inspectorRotation")) {
		rotation->setValue(15);
		obs_sceneitem_get_info2(fixtureItem, &changed);
		Check(CloseEnough(changed.rot, 15), QStringLiteral("rotation-edit-reaches-obs"));
	}
	if (auto *width = Control<QDoubleSpinBox>("inspectorWidth")) {
		width->setValue(400);
		obs_sceneitem_get_info2(fixtureItem, &changed);
		Check(CloseEnough(std::abs(changed.scale.x) * obs_source_get_width(obs_sceneitem_get_source(fixtureItem)), 400),
		      QStringLiteral("width-edit-reaches-obs"));
		Check(CloseEnough(changed.scale.x / original.scale.x, changed.scale.y / original.scale.y),
		      QStringLiteral("aspect-ratio-preserved"));
	}
	auto *cropToggle = Control<QAbstractButton>("inspectorCropToggle");
	auto *cropFields = Control<QWidget>("inspectorCropFields");
	if (cropToggle && cropFields) {
		Check(cropToggle->isCheckable() && !cropToggle->isChecked() && !cropFields->isVisible(),
		      QStringLiteral("crop-section-collapsed-by-default"));
		cropToggle->click();
		Check(cropToggle->isChecked() && cropFields->isVisible(), QStringLiteral("crop-section-opens"));
	}
	if (auto *crop = Control<QSpinBox>("inspectorCropLeft")) {
		crop->setValue(10);
		obs_sceneitem_crop value{};
		obs_sceneitem_get_crop(fixtureItem, &value);
		Check(value.left == 10, QStringLiteral("crop-edit-reaches-obs"));
		Trigger("actionMainUndo");
		obs_sceneitem_get_crop(fixtureItem, &value);
		Check(value.left == 0, QStringLiteral("crop-undo"));
		Trigger("actionMainRedo");
		obs_sceneitem_get_crop(fixtureItem, &value);
		Check(value.left == 10, QStringLiteral("crop-redo"));
	}
	Click("inspectorVisible");
	Check(!obs_sceneitem_visible(fixtureItem), QStringLiteral("visibility-edit"));
	Trigger("actionMainUndo");
	Check(obs_sceneitem_visible(fixtureItem), QStringLiteral("visibility-undo"));
	Click("inspectorLocked");
	Check(obs_sceneitem_locked(fixtureItem) && !x->isEnabled(), QStringLiteral("lock-disables-transform"));
	Trigger("actionMainUndo");
	Check(!obs_sceneitem_locked(fixtureItem), QStringLiteral("lock-undo"));
	// Allow the inspector's normal refresh to enable controls before later phases.
}


void RefreshInspector()
{
	if (auto *inspector = mainWindow->findChild<QWidget *>(QStringLiteral("penguinInspector")))
		QMetaObject::invokeMethod(inspector, "RefreshSelection", Qt::DirectConnection);
}

void CheckInspectorWidth(const QString &size)
{
	auto *scroll = Control<QScrollArea>("inspectorScroll");
	if (!scroll)
		return;
	for (const char *name : {"inspectorX", "inspectorY", "inspectorWidth", "inspectorHeight", "inspectorRotation",
		"inspectorCropLeft", "inspectorCropRight", "inspectorCropTop", "inspectorCropBottom", "inspectorVisible",
		"inspectorLocked", "inspectorAspectLock", "inspectorFit", "inspectorCenter", "inspectorFlipHorizontal",
		"inspectorFlipVertical", "inspectorProperties", "inspectorFilters", "inspectorAdvanced", "inspectorReset"}) {
		auto *control = mainWindow->findChild<QWidget *>(QString::fromLatin1(name));
		// A collapsed section has no layout allocation until it is expanded.
		// Its controls are covered again by the compact run after expansion.
		if (control && !control->isVisible())
			continue;
		const int left = control ? control->mapTo(scroll->viewport(), QPoint()).x() : -1;
		Check(control && left >= 0 && left + control->width() <= scroll->viewport()->width(),
		      QStringLiteral("inspector-horizontal-fit-%1-%2").arg(size, QString::fromLatin1(name)));
	}
}

obs_sceneitem_t *edgeGroup = nullptr;
obs_transform_info originalGroupChild{};
obs_transform_info originalGroup{};

void PrepareInspectorEdgeCases()
{
	if (!fixtureItem)
		return;
	auto *width = Control<QDoubleSpinBox>("inspectorWidth");
	auto *height = Control<QDoubleSpinBox>("inspectorHeight");
	if (!width || !height)
		return;
	obs_transform_info info{};
	obs_sceneitem_get_info2(fixtureItem, &info);
	info.scale = {-0.5f, 0.5f};
	obs_sceneitem_set_info2(fixtureItem, &info);
	obs_sceneitem_crop crop{};
	crop.left = 40;
	crop.right = 50;
	crop.top = 20;
	crop.bottom = 30;
	obs_sceneitem_set_crop(fixtureItem, &crop);
	RefreshInspector();
	auto *source = obs_sceneitem_get_source(fixtureItem);
	const float croppedWidth = float(obs_source_get_width(source)) - crop.left - crop.right;
	const float croppedHeight = float(obs_source_get_height(source)) - crop.top - crop.bottom;
	Check(CloseEnough(width->value(), croppedWidth * 0.5f) && CloseEnough(height->value(), croppedHeight * 0.5f),
	      QStringLiteral("negative-cropped-dimensions-display"));
	width->setValue(200);
	obs_sceneitem_get_info2(fixtureItem, &info);
	Check(info.scale.x < 0 && CloseEnough(std::abs(info.scale.x) * croppedWidth, 200),
	      QStringLiteral("negative-cropped-width-edit"));
	Check(CloseEnough(std::abs(info.scale.x), info.scale.y), QStringLiteral("negative-scale-aspect-link"));
	info.bounds_type = OBS_BOUNDS_SCALE_INNER;
	info.bounds = {400, 300};
	obs_sceneitem_set_info2(fixtureItem, &info);
	RefreshInspector();
	Check(CloseEnough(width->value(), 400) && CloseEnough(height->value(), 300), QStringLiteral("bounded-dimensions-display"));
	width->setValue(600);
	obs_sceneitem_get_info2(fixtureItem, &info);
	Check(CloseEnough(info.bounds.x, 600) && CloseEnough(info.bounds.y, 450), QStringLiteral("bounded-width-edit-aspect-link"));
	Trigger("actionMainUndo");
	obs_sceneitem_get_info2(fixtureItem, &info);
	Check(CloseEnough(info.bounds.x, 400) && CloseEnough(info.bounds.y, 300), QStringLiteral("bounded-width-undo"));
	info.bounds_type = OBS_BOUNDS_NONE;
	info.pos = {200, 100};
	info.scale = {0.5f, 0.5f};
	info.rot = 0;
	obs_sceneitem_set_info2(fixtureItem, &info);
	crop = {};
	obs_sceneitem_set_crop(fixtureItem, &crop);
	edgeGroup = obs_scene_insert_group2(obs_sceneitem_get_scene(fixtureItem), "Validation group", &fixtureItem, 1, true);
	Check(edgeGroup != nullptr, QStringLiteral("group-fixture-created"));
	if (edgeGroup) {
		obs_sceneitem_select(edgeGroup, false);
		obs_sceneitem_select(fixtureItem, true);
	}
}

void CheckGroupTransform(int stage)
{
	if (!edgeGroup)
		return;
	obs_transform_info child{}, group{};
	obs_sceneitem_get_info2(fixtureItem, &child);
	obs_sceneitem_get_info2(edgeGroup, &group);
	if (stage == 92) {
		RefreshInspector();
		originalGroupChild = child;
		originalGroup = group;
		if (auto *x = Control<QDoubleSpinBox>("inspectorX")) {
			Check(x->isVisible() && x->isEnabled(), QStringLiteral("group-child-inspector-editable"));
			x->setValue(child.pos.x + 100);
		}
	} else if (stage == 96) {
		Check(CloseEnough(group.pos.x, originalGroup.pos.x + 100 * originalGroup.scale.x),
		      QStringLiteral("group-child-edit-moves-world-position"));
		Trigger("actionMainUndo");
	} else if (stage == 100) {
		Check(CloseEnough(group.pos.x, originalGroup.pos.x) && CloseEnough(child.pos.x, originalGroupChild.pos.x),
		      QStringLiteral("group-child-undo-restores-world-position"));
		Trigger("actionMainRedo");
	} else if (stage == 104) {
		Check(CloseEnough(group.pos.x, originalGroup.pos.x + 100 * originalGroup.scale.x),
		      QStringLiteral("group-child-redo-restores-world-position"));
	}
}

void Tick()
{
	++ticks;
	if (!mainWindow) {
		for (QWidget *widget : QApplication::topLevelWidgets()) {
			if (qobject_cast<QMainWindow *>(widget) &&
			    QByteArray(widget->metaObject()->className()) == "OBSBasic")
				mainWindow = qobject_cast<QMainWindow *>(widget);
		}
	}
	if (!mainWindow)
		return;
	const QByteArray mode = qgetenv("PENGUIN_SMOKE_MODE");
	const bool closeWizard = qgetenv("PENGUIN_SMOKE_MODE") == "wizard-close";
	if (ticks < 24) {
		for (QWidget *widget : QApplication::topLevelWidgets()) {
			auto *dialog = qobject_cast<QDialog *>(widget);
			if (dialog && dialog->isVisible()) {
				if (closeWizard && qobject_cast<QWizard *>(dialog) &&
				    QByteArray(dialog->metaObject()->className()) == "QWizard") {
					Check(true, QStringLiteral("close-with-active-setup-wizard"));
					SaveReport();
					mainWindow->close();
					return;
				}
				dialog->reject();
			}
		}
	}
	if (ticks == 24 && (mode == "layout-save" || mode == "layout-reopen")) {
		auto *dock = Control<QDockWidget>("penguinInspectorDock");
		if (dock) {
			if (mode == "layout-save") {
				dock->setFloating(false);
				mainWindow->removeDockWidget(dock);
				mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dock);
				dock->show();
			}
			Check(mainWindow->dockWidgetArea(dock) == Qt::LeftDockWidgetArea && dock->isVisible(),
			      QString::fromLatin1(mode));
		}
		SaveReport();
		mainWindow->close();
		return;
	}
	if (ticks == 24) {
		mainWindow->showNormal();
		mainWindow->resize(1440, 900);
		AddFixture();
	}
	if (ticks == 28)
		mainWindow->resize(1440, 900);
	if (ticks == 32)
		Trigger("actionScaleWindow");
	if (ticks == 36) {
		Capture(QStringLiteral("workspace-1440.png"));
		CheckWorkspaceLayout(QStringLiteral("large"), 1440, 900);
		CheckInspectorWidth(QStringLiteral("large"));
		TestWorkspaceSearch();
		TestSceneRename();
		TestRightSidebar();
		CheckSidebarTabText(QStringLiteral("large"));
	}
	if (ticks == 37 && qEnvironmentVariableIsSet("PENGUIN_SMOKE_EXPECT_REDESIGN"))
		TestInspector();
	if (ticks == 40)
		mainWindow->resize(1280, 800);
	if (ticks == 48) {
		Capture(QStringLiteral("workspace-1280.png"));
		CheckWorkspaceLayout(QStringLiteral("compact"), 1280, 800);
		CheckInspectorWidth(QStringLiteral("compact"));
		CheckSidebarTabText(QStringLiteral("compact"));
	}
	if (ticks == 49 && fixtureItem && qEnvironmentVariableIsSet("PENGUIN_SMOKE_EXPECT_REDESIGN")) {
		Click("inspectorFlipHorizontal");
		obs_transform_info info{};
		obs_sceneitem_get_info2(fixtureItem, &info);
		Check(info.scale.x < 0, QStringLiteral("horizontal-flip"));
		Trigger("actionMainUndo");
		obs_sceneitem_get_info2(fixtureItem, &info);
		Check(info.scale.x > 0, QStringLiteral("horizontal-flip-undo"));
		if (backgroundItem)
			obs_sceneitem_select(backgroundItem, true);
	}
	if (ticks == 52 && fixtureItem && qEnvironmentVariableIsSet("PENGUIN_SMOKE_EXPECT_REDESIGN")) {
		if (auto *x = Control<QDoubleSpinBox>("inspectorX"))
			Check(!x->isVisible(), QStringLiteral("multiple-selection-inspector-disabled"));
		if (backgroundItem)
			obs_sceneitem_select(backgroundItem, false);
		alternateScene = obs_scene_create("Validation alternate scene");
		if (alternateScene)
			obs_frontend_set_current_scene(obs_scene_get_source(alternateScene));
	}
	if (ticks == 56 && alternateScene) {
		if (auto *x = Control<QDoubleSpinBox>("inspectorX"))
			Check(!x->isVisible(), QStringLiteral("empty-scene-clears-inspector"));
		obs_source_t *source = obs_get_source_by_uuid(fixtureSceneUuid.toUtf8().constData());
		obs_frontend_set_current_scene(source);
		obs_source_release(source);
	}
	if (ticks == 60) {
		if (alternateScene) {
			if (auto *x = Control<QDoubleSpinBox>("inspectorX"))
				Check(x->isVisible(), QStringLiteral("scene-switch-restores-inspector"));
			obs_scene_release(alternateScene);
			alternateScene = nullptr;
		}
		Click("penguinDestinationsButton");
		if (auto *dock = Control<QDockWidget>("penguin-broadcast-destinations")) {
			Check(dock->isVisible(), QStringLiteral("destinations-toolbar-opens-panel"));
			Check(!dock->isFloating() && !dock->visibleRegion().isEmpty(),
			      QStringLiteral("destinations-toolbar-selects-integrated-tab"));
			RaiseInspector();
		}
		Click("penguinCanvasMode");
	}
	if (ticks == 63) {
		Check(obs_frontend_preview_program_mode_active(), QStringLiteral("studio-mode-toolbar-enabled"));
		Click("penguinCanvasMode");
		if (auto *panel = Control<QWidget>("penguinOutputPanel")) {
			Check(panel->property("penguinDestinationCount").toInt() == 0, QStringLiteral("no-external-destinations-configured"));
			if (panel->property("penguinDestinationCount").toInt() == 0)
				Click("penguinLiveButton");
		}
	}
	if (ticks == 67) {
		bool found = false;
		for (auto *widget : QApplication::topLevelWidgets()) {
			if (auto *wizard = qobject_cast<QWizard *>(widget); wizard && wizard->isVisible()) {
				found = true;
				wizard->reject();
			}
		}
		Check(found, QStringLiteral("go-live-opens-real-connection-wizard"));
		RaiseInspector();
		Check(!obs_frontend_preview_program_mode_active(), QStringLiteral("studio-mode-toolbar-disabled"));
		Results().mkpath(QStringLiteral("recordings"));
		const auto path = Results().filePath(QStringLiteral("recordings")).toUtf8();
		auto *config = obs_frontend_get_profile_config();
		config_set_string(config, "Output", "Mode", "Simple");
		config_set_string(config, "SimpleOutput", "FilePath", path.constData());
		config_set_string(config, "SimpleOutput", "RecFormat2", "mkv");
		config_set_string(config, "SimpleOutput", "RecQuality", "Small");
		config_set_string(config, "SimpleOutput", "RecEncoder", "x264");
		QTimer::singleShot(1000, qApp, [] {
			for (auto *widget : QApplication::topLevelWidgets()) {
				if (auto *dialog = qobject_cast<QMessageBox *>(widget); dialog && dialog->isVisible()) {
					Check(false, QStringLiteral("recording-modal: %1: %2").arg(dialog->windowTitle(), dialog->text()));
					dialog->reject();
				}
			}
		});
		Click("penguinRecordButton");
	}
	if (ticks == 72)
		Check(obs_frontend_recording_active(), QStringLiteral("record-toolbar-starts-real-recording"));
	if (ticks == 80 && obs_frontend_recording_active())
		Click("penguinRecordButton");
	if (ticks == 88) {
		Check(!obs_frontend_recording_active(), QStringLiteral("record-toolbar-stops-recording"));
		Check(!Results().entryList({QStringLiteral("recordings/*.mkv")}, QDir::Files).isEmpty() ||
		      !QDir(Results().filePath(QStringLiteral("recordings"))).entryList({QStringLiteral("*.mkv")}, QDir::Files).isEmpty(),
		      QStringLiteral("local-recording-file-created"));
		if (closeWizard)
			Check(false, QStringLiteral("expected-setup-wizard-was-not-seen"));
		PrepareInspectorEdgeCases();
	}
	if (ticks == 92 || ticks == 96 || ticks == 100 || ticks == 104)
		CheckGroupTransform(ticks);
	if (ticks == 108) {
		SaveReport();
		for (auto *item : audioFixtures)
			obs_sceneitem_remove(item);
		audioFixtures.clear();
		mainWindow->close();
	}
}

void Start()
{
	if (qgetenv("PENGUIN_SMOKE_DIR").isEmpty())
		return;
	Results().mkpath(QStringLiteral("."));
	QTimer::singleShot(1000, qApp, [] {
		auto *timer = new QTimer(qApp);
		QObject::connect(timer, &QTimer::timeout, Tick);
		timer->start(250);
	});
}
Q_COREAPP_STARTUP_FUNCTION(Start)
} // namespace
