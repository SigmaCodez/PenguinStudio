// SPDX-License-Identifier: GPL-2.0-or-later
// LD_PRELOAD test helper. Run only with a new, isolated configuration directory.
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QFile>
#include <QImage>
#include <QListView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTimer>
#include <QWidget>
#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <graphics/vec4.h>
#include <cstring>
#include <deque>
#include <functional>
#include "../frontend/utility/PenguinCameraFilters.hpp"

namespace {
obs_source_t *source = nullptr, *filter = nullptr;
obs_scene_t *scene = nullptr;
QJsonObject report;
QString directory;
QWidget *mainWindow = nullptr;

// Each wait starts after the preceding step completes. Independent coarse Qt
// timers can overlap (notably Undo/Redo/capture) on a loaded CI runner.
struct TestStep {
	int delay;
	std::function<void()> run;
};
std::deque<TestStep> steps;
int previousDeadline = 0;
void NextStep()
{
	if (steps.empty()) {
		return;
	}
	auto step = std::move(steps.front());
	steps.pop_front();
	QTimer::singleShot(step.delay, Qt::PreciseTimer, qApp, [run = std::move(step.run)] {
		run();
		NextStep();
	});
}
void At(int deadline, std::function<void()> run)
{
	steps.push_back({deadline - previousDeadline, std::move(run)});
	previousDeadline = deadline;
}

void Fail(int code)
{
	report["failure_code"] = code;
	QFile result(directory + "/result.json");
	if (result.open(QIODevice::WriteOnly)) {
		result.write(QJsonDocument(report).toJson());
	}
	obs_source_release(filter);
	filter = nullptr;
	obs_source_release(source);
	source = nullptr;
	obs_scene_release(scene);
	scene = nullptr;
	QCoreApplication::exit(code);
}

void Capture(void *data)
{
	obs_enter_graphics();
	const char *name = static_cast<const char *>(data);
	const uint32_t width = obs_source_get_width(source), height = obs_source_get_height(source);
	auto *texture = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	auto *stage = gs_stagesurface_create(width, height, GS_RGBA);
	if (gs_texrender_begin(texture, width, height)) {
		vec4 clear{};
		gs_clear(GS_CLEAR_COLOR, &clear, 0, 0);
		gs_ortho(0, float(width), 0, float(height), -100, 100);
		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
		obs_source_video_render(source);
		gs_blend_state_pop();
		gs_texrender_end(texture);
		gs_stage_texture(stage, gs_texrender_get_texture(texture));
		uint8_t *pixels;
		uint32_t stride;
		if (gs_stagesurface_map(stage, &pixels, &stride)) {
			QFile file(directory + '/' + name + ".rgba");
			if (file.open(QIODevice::WriteOnly)) {
				for (uint32_t row = 0; row < height; ++row) {
					file.write(reinterpret_cast<char *>(pixels + row * stride), width * 4);
				}
				report[name] = QString("%1x%2").arg(width).arg(height);
			}
			gs_stagesurface_unmap(stage);
		}
	}
	gs_stagesurface_destroy(stage);
	gs_texrender_destroy(texture);
	obs_leave_graphics();
}
void Snap(const char *name)
{
	obs_queue_task(OBS_TASK_GRAPHICS, Capture, const_cast<char *>(name), true);
}

void FirstMattingFrame(void *)
{
	// Create and render within one graphics task: no tick can prepare a mask
	// between these operations. A fresh filter must not flash the raw background.
	auto *previous = source;
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "file", (directory + "/green.png").toUtf8().constData());
	source = obs_source_create("image_source", "First matting frame fixture", settings, nullptr);
	if (source && PenguinCamera::Apply(source, "Camera appearance", 0, 1, 25, 15)) {
		Capture(const_cast<char *>("automatic-first-frame"));
	}
	obs_source_release(source);
	source = previous;
}
void Mode(int shape, int background)
{
	auto *shapes = mainWindow->findChild<QComboBox *>("cameraShape");
	auto *backgrounds = mainWindow->findChild<QComboBox *>("cameraBackground");
	if (shapes && backgrounds) {
		report["controls_enabled"] = shapes->isEnabled() && backgrounds->isEnabled();
		report["controls_visible"] = shapes->isVisible();
		auto *sources = mainWindow->findChild<QListView *>("sources");
		report["selected_rows"] = sources ? sources->selectionModel()->selectedRows().size() : -1;
		report["initial_shape"] = shapes->currentIndex();
		shapes->setCurrentIndex(shape);
		backgrounds->setCurrentIndex(background);
		report["inspector_used"] = true;
	} else {
		auto *settings = obs_source_get_settings(filter);
		obs_data_set_int(settings, "shape", shape);
		obs_data_set_int(settings, "background", background);
		obs_source_update(filter, settings);
		obs_data_release(settings);
	}
}
void Image(const QString &path)
{
	auto *settings = obs_source_get_settings(source);
	obs_data_set_string(settings, "file", path.toUtf8().constData());
	obs_source_update(source, settings);
	obs_data_release(settings);
}
void Start()
{
	directory = qEnvironmentVariable("PENGUIN_CAMERA_RESULTS");
	if (directory.isEmpty()) {
		return;
	}
	At(3000, [] {
		for (auto *widget : QApplication::topLevelWidgets()) {
			if (!std::strcmp(widget->metaObject()->className(), "OBSBasic")) {
				mainWindow = widget;
			}
		}
		if (!mainWindow) {
			Fail(42);
			return;
		}
		if (!obs_source_get_display_name("penguin_camera_filter")) {
			obs_module_t *module = nullptr;
			const auto path = qgetenv("PENGUIN_CAMERA_MODULE");
			const auto data = qgetenv("PENGUIN_CAMERA_DATA");
			if (obs_open_module(&module, path.constData(), data.constData()) != MODULE_SUCCESS ||
			    !obs_init_module(module)) {
				Fail(43);
				return;
			}
		}
		for (const auto &name : {QString("green"), QString("blue")}) {
			QImage image(640, 360, QImage::Format_RGBA8888);
			image.fill(name == "green" ? Qt::green : Qt::blue);
			QPainter painter(&image);
			painter.fillRect(240, 100, 161, 201, Qt::red);
			painter.end();
			if (!image.save(directory + '/' + name + ".png")) {
				Fail(44);
				return;
			}
		}
		auto *settings = obs_data_create();
		obs_data_set_string(settings, "file", (directory + "/green.png").toUtf8().constData());
		source = obs_source_create("image_source", "Camera fixture", settings, nullptr);
		obs_data_release(settings);
		filter = obs_source_create("penguin_camera_filter", "Camera appearance", nullptr, nullptr);
		if (!source || !filter) {
			Fail(45);
			return;
		}
		obs_source_filter_add(source, filter);
		scene = obs_scene_create("Camera fixture scene");
		auto *item = obs_scene_add(scene, source);
		obs_frontend_set_current_scene(obs_scene_get_source(scene));
		obs_sceneitem_select(item, true);
	});
	At(4000, [] {
		// Scene changes reach the source tree asynchronously. Select its row
		// after that update, just as a user selecting the source would do.
		auto *sources = mainWindow->findChild<QListView *>("sources");
		if (sources && sources->model()->rowCount() == 1) {
			const auto index = sources->model()->index(0, 0);
			sources->setCurrentIndex(index);
			sources->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect |
									 QItemSelectionModel::Rows);
		}
	});
	At(4500, [] {
		obs_queue_task(OBS_TASK_GRAPHICS, FirstMattingFrame, nullptr, true);
		Snap("rectangle");
		Mode(1, 0);
	});
	At(5000, [] {
		if (!report["inspector_used"].toBool()) {
			return;
		}
		auto *undo = mainWindow->findChild<QAction *>("actionMainUndo");
		if (!undo || !undo->isEnabled()) {
			Fail(48);
			return;
		}
		undo->trigger();
		auto *settings = obs_source_get_settings(filter);
		report["default_undo_shape"] = int(obs_data_get_int(settings, "shape"));
		report["default_undo_radius"] = obs_data_get_double(settings, "radius");
		obs_data_release(settings);
	});
	At(5250, [] {
		if (!report["inspector_used"].toBool()) {
			return;
		}
		auto *redo = mainWindow->findChild<QAction *>("actionMainRedo");
		if (!redo || !redo->isEnabled()) {
			Fail(49);
			return;
		}
		redo->trigger();
		auto *settings = obs_source_get_settings(filter);
		report["default_redo_shape"] = int(obs_data_get_int(settings, "shape"));
		obs_data_release(settings);
	});
	At(5500, [] {
		Snap("circle");
		Mode(2, 0);
	});
	At(6500, [] {
		Snap("oval");
		Mode(3, 0);
	});
	At(7500, [] {
		Snap("rounded");
		Mode(0, 2);
	});
	At(8500, [] {
		Snap("green-key");
		Image(directory + "/blue.png");
		Mode(0, 3);
	});
	At(9500, [] {
		Snap("blue-key");
		const QString portrait = qEnvironmentVariable("PENGUIN_CAMERA_PORTRAIT");
		Image(portrait.isEmpty() ? directory + "/green.png" : portrait);
		Mode(0, 1);
	});
	At(10500, [] {
		if (!report["inspector_used"].toBool()) {
			return;
		}
		mainWindow->findChild<QAction *>("actionMainUndo")->trigger();
		report["automatic_undo_removed_matting"] = !PenguinCamera::CameraMatting(source);
		mainWindow->findChild<QAction *>("actionMainRedo")->trigger();
		report["automatic_redo_restored_matting"] = bool(PenguinCamera::CameraMatting(source));
	});
	At(14500, [] {
		Snap("automatic");
		calldata_t parameters;
		calldata_init(&parameters);
		auto matting = PenguinCamera::CameraMatting(source);
		proc_handler_call(obs_source_get_proc_handler(matting ? matting.Get() : filter), "camera_status",
				  &parameters);
		report["rvm_backend"] = bool(matting);
		report["ai_status"] = int(calldata_int(&parameters, "status"));
		calldata_free(&parameters);
		Mode(1, 1);
	});
	At(16000, [] {
		Snap("automatic-circle");
		Mode(0, 0);
	});
	At(17500, [] {
		Snap("disabled");
		Mode(3, 2);
		auto *saved = obs_save_source(source);
		QFile settings(directory + "/source.json");
		if (!settings.open(QIODevice::WriteOnly)) {
			obs_data_release(saved);
			Fail(46);
			return;
		}
		settings.write(obs_data_get_json(saved));
		obs_data_release(saved);
		// Reproduce rapid pair creation/removal while the graphics thread renders.
		const auto before = PenguinCamera::SaveCamera(source);
		OBSDataAutoRelease unrelatedSettings = obs_data_create();
		OBSSourceAutoRelease unrelated =
			obs_source_create("color_filter", "Unrelated camera test filter", unrelatedSettings, nullptr);
		obs_source_set_enabled(unrelated, false);
		obs_source_filter_add(source, unrelated);
		bool pairsOk = true;
		for (int i = 0; i < 5; ++i) {
			pairsOk &= PenguinCamera::Apply(source, "Camera appearance", i % 4, 1, 25, 15);
			const auto automatic = PenguinCamera::SaveCamera(source);
			// Keep the removed instance alive to exercise UUID reuse reliably.
			auto previousMatting = PenguinCamera::CameraMatting(source);
			PenguinCamera::RestoreCamera(before);
			pairsOk &= !PenguinCamera::CameraMatting(source);
			PenguinCamera::RestoreCamera(automatic);
			auto matting = PenguinCamera::CameraMatting(source);
			pairsOk &= matting && previousMatting &&
				   std::strcmp(obs_source_get_uuid(matting), obs_source_get_uuid(previousMatting)) != 0;
			pairsOk &= matting && obs_source_filter_get_index(source, matting) ==
						      obs_source_filter_get_index(source, filter) + 1;
		}
		PenguinCamera::RestoreCamera(before);
		report["pair_lifecycle"] = pairsOk;
		report["unrelated_filter_preserved"] = obs_source_filter_get_index(source, unrelated) >= 0 &&
						       !obs_source_enabled(unrelated);
		report["platform"] = QGuiApplication::platformName();
		report["visible"] = mainWindow->isVisible();
		QFile result(directory + "/result.json");
		if (!result.open(QIODevice::WriteOnly)) {
			Fail(47);
			return;
		}
		result.write(QJsonDocument(report).toJson());
		obs_source_release(filter);
		obs_source_release(source);
		obs_scene_release(scene);
		mainWindow->close();
	});
	NextStep();
}
} // namespace
Q_COREAPP_STARTUP_FUNCTION(Start)
