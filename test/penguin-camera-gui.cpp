// SPDX-License-Identifier: GPL-2.0-or-later
// LD_PRELOAD test helper. Run only with a new, isolated configuration directory.
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QFile>
#include <QImage>
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

namespace {
obs_source_t *source = nullptr, *filter = nullptr;
obs_scene_t *scene = nullptr;
QJsonObject report;
QString directory;
QWidget *mainWindow = nullptr;

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
void Mode(int shape, int background)
{
	auto *shapes = mainWindow->findChild<QComboBox *>("cameraShape");
	auto *backgrounds = mainWindow->findChild<QComboBox *>("cameraBackground");
	if (shapes && backgrounds) {
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
	QTimer::singleShot(3000, qApp, [] {
		for (auto *widget : QApplication::topLevelWidgets()) {
			if (!std::strcmp(widget->metaObject()->className(), "OBSBasic")) {
				mainWindow = widget;
			}
		}
		if (!mainWindow) {
			QCoreApplication::exit(42);
			return;
		}
		if (!obs_source_get_display_name("penguin_camera_filter")) {
			obs_module_t *module = nullptr;
			const auto path = qgetenv("PENGUIN_CAMERA_MODULE");
			const auto data = qgetenv("PENGUIN_CAMERA_DATA");
			if (obs_open_module(&module, path.constData(), data.constData()) != MODULE_SUCCESS ||
			    !obs_init_module(module)) {
				QCoreApplication::exit(43);
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
				QCoreApplication::exit(44);
				return;
			}
		}
		auto *settings = obs_data_create();
		obs_data_set_string(settings, "file", (directory + "/green.png").toUtf8().constData());
		source = obs_source_create("image_source", "Camera fixture", settings, nullptr);
		obs_data_release(settings);
		filter = obs_source_create("penguin_camera_filter", "Camera appearance", nullptr, nullptr);
		if (!source || !filter) {
			QCoreApplication::exit(45);
			return;
		}
		obs_source_filter_add(source, filter);
		scene = obs_scene_create("Camera fixture scene");
		auto *item = obs_scene_add(scene, source);
		obs_frontend_set_current_scene(obs_scene_get_source(scene));
		obs_sceneitem_select(item, true);
	});
	QTimer::singleShot(4500, qApp, [] {
		Snap("rectangle");
		Mode(1, 0);
	});
	QTimer::singleShot(5000, qApp, [] {
		if (!report["inspector_used"].toBool()) {
			return;
		}
		auto *undo = mainWindow->findChild<QAction *>("actionUndo");
		if (!undo || !undo->isEnabled()) {
			QCoreApplication::exit(48);
			return;
		}
		undo->trigger();
		auto *settings = obs_source_get_settings(filter);
		report["default_undo_shape"] = int(obs_data_get_int(settings, "shape"));
		report["default_undo_radius"] = obs_data_get_double(settings, "radius");
		obs_data_release(settings);
	});
	QTimer::singleShot(5250, qApp, [] {
		if (!report["inspector_used"].toBool()) {
			return;
		}
		auto *redo = mainWindow->findChild<QAction *>("actionRedo");
		if (!redo || !redo->isEnabled()) {
			QCoreApplication::exit(49);
			return;
		}
		redo->trigger();
		auto *settings = obs_source_get_settings(filter);
		report["default_redo_shape"] = int(obs_data_get_int(settings, "shape"));
		obs_data_release(settings);
	});
	QTimer::singleShot(5500, qApp, [] {
		Snap("circle");
		Mode(2, 0);
	});
	QTimer::singleShot(6500, qApp, [] {
		Snap("oval");
		Mode(3, 0);
	});
	QTimer::singleShot(7500, qApp, [] {
		Snap("rounded");
		Mode(0, 2);
	});
	QTimer::singleShot(8500, qApp, [] {
		Snap("green-key");
		Image(directory + "/blue.png");
		Mode(0, 3);
	});
	QTimer::singleShot(9500, qApp, [] {
		Snap("blue-key");
		const QString portrait = qEnvironmentVariable("PENGUIN_CAMERA_PORTRAIT");
		Image(portrait.isEmpty() ? directory + "/green.png" : portrait);
		Mode(0, 1);
	});
	QTimer::singleShot(14500, qApp, [] {
		Snap("automatic");
		calldata_t parameters;
		calldata_init(&parameters);
		proc_handler_call(obs_source_get_proc_handler(filter), "camera_status", &parameters);
		report["ai_status"] = int(calldata_int(&parameters, "status"));
		calldata_free(&parameters);
		Mode(1, 1);
	});
	QTimer::singleShot(16000, qApp, [] {
		Snap("automatic-circle");
		Mode(0, 0);
	});
	QTimer::singleShot(17500, qApp, [] {
		Snap("disabled");
		Mode(3, 2);
		auto *saved = obs_save_source(source);
		QFile settings(directory + "/source.json");
		if (!settings.open(QIODevice::WriteOnly)) {
			QCoreApplication::exit(46);
			return;
		}
		settings.write(obs_data_get_json(saved));
		obs_data_release(saved);
		report["platform"] = QGuiApplication::platformName();
		report["visible"] = mainWindow->isVisible();
		QFile result(directory + "/result.json");
		if (!result.open(QIODevice::WriteOnly)) {
			QCoreApplication::exit(47);
			return;
		}
		result.write(QJsonDocument(report).toJson());
		obs_source_release(filter);
		obs_source_release(source);
		obs_scene_release(scene);
		mainWindow->close();
	});
}
} // namespace
Q_COREAPP_STARTUP_FUNCTION(Start)
