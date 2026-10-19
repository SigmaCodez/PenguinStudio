// SPDX-License-Identifier: GPL-2.0-or-later
// Integration test: the production output controller streams only to loopback FFmpeg receivers.
#include "../penguin-outputs.cpp"

#include <obs-nix-platform.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QProcess>
#include <QThread>

#include <functional>
#include <iostream>
#include <stdexcept>
#include <X11/Xlib.h>
#ifdef ENABLE_WAYLAND
#include <wayland-client.h>
#endif

namespace {
void Check(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

bool Await(const std::function<bool()> &ready, int timeoutMs = 15000)
{
	QElapsedTimer elapsed;
	elapsed.start();
	while (!ready() && elapsed.elapsed() < timeoutMs) {
		QCoreApplication::processEvents();
		QThread::msleep(20);
	}
	return ready();
}

void Receiver(QProcess &process, int port, const QString &file)
{
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.start(QStringLiteral("ffmpeg"),
		      {QStringLiteral("-hide_banner"), QStringLiteral("-loglevel"), QStringLiteral("warning"),
		       QStringLiteral("-y"), QStringLiteral("-listen"), QStringLiteral("1"), QStringLiteral("-i"),
		       QStringLiteral("rtmp://127.0.0.1:%1/live/test").arg(port), QStringLiteral("-c"),
		       QStringLiteral("copy"), file});
	Check(process.waitForStarted(5000), "FFmpeg receiver did not start");
	QThread::msleep(350);
	Check(process.state() == QProcess::Running, "FFmpeg receiver exited before connecting");
}

void Start(RunningOutput &output, int port, Layout layout)
{
	Destination destination;
	destination.name = QStringLiteral("Local integration test");
	destination.server = QStringLiteral("rtmp://127.0.0.1:%1/live").arg(port);
	destination.key = QStringLiteral("test");
	destination.layout = layout;
	destination.bitrate = 2000;
	const QString error = output.Initialize(destination);
	if (!error.isEmpty())
		throw std::runtime_error(error.toStdString());
	obs_output_set_reconnect_settings(output.output, 0, 1);
	output.startRequested = obs_output_start(output.output);
	Check(output.startRequested, "libobs refused to start the local stream");
}

void CheckVideo(const QString &file, int width, int height)
{
	QProcess probe;
	probe.start(QStringLiteral("ffprobe"),
		    {QStringLiteral("-v"), QStringLiteral("error"), QStringLiteral("-show_streams"),
		     QStringLiteral("-of"), QStringLiteral("json"), file});
	Check(probe.waitForFinished(10000) && probe.exitCode() == 0, "ffprobe could not read recorded stream");
	const auto streams = QJsonDocument::fromJson(probe.readAllStandardOutput()).object()
				     .value(QStringLiteral("streams"))
				     .toArray();
	bool video = false;
	bool audio = false;
	for (const auto &value : streams) {
		const auto stream = value.toObject();
		if (stream.value(QStringLiteral("codec_type")) == QStringLiteral("video")) {
			Check(stream.value(QStringLiteral("width")).toInt() == width, "incorrect video width");
			Check(stream.value(QStringLiteral("height")).toInt() == height, "incorrect video height");
			Check(stream.value(QStringLiteral("codec_name")) == QStringLiteral("h264"), "video is not H.264");
			video = true;
		}
		if (stream.value(QStringLiteral("codec_type")) == QStringLiteral("audio")) {
			Check(stream.value(QStringLiteral("codec_name")) == QStringLiteral("aac"), "audio is not AAC");
			audio = true;
		}
	}
	Check(video && audio, "recorded stream needs both H.264 video and AAC audio");
}
} // namespace

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	if (argc != 3) {
		std::cerr << "Usage: penguin-local-stream <installation-prefix> <results-directory>\n";
		return 2;
	}
	const QDir prefix(QString::fromLocal8Bit(argv[1]));
	const QDir results(QString::fromLocal8Bit(argv[2]));
	Check(QDir().mkpath(results.absolutePath()), "cannot create results directory");
	Display *display = nullptr;
#ifdef ENABLE_WAYLAND
	wl_display *wayland = wl_display_connect(nullptr);
	if (wayland) {
		obs_set_nix_platform(OBS_NIX_PLATFORM_WAYLAND);
		obs_set_nix_platform_display(wayland);
		std::cout << "Rendering backend: native Wayland EGL\n";
	} else
#endif
	{
		display = XOpenDisplay(nullptr);
		if (display) {
			obs_set_nix_platform(OBS_NIX_PLATFORM_X11_EGL);
			obs_set_nix_platform_display(display);
			std::cout << "Rendering backend: X11 EGL\n";
		}
	}
	if (!obs_get_nix_platform_display()) {
		std::cerr << "This test needs an accessible Wayland or X11 display for libobs EGL.\n";
		return 2;
	}
	bool initialized = false;
	int result = 0;
	try {
		initialized = obs_startup("en-US", nullptr, nullptr);
		Check(initialized, "obs_startup failed");
		obs_video_info video{};
		const QByteArray graphics = prefix.filePath(QStringLiteral("lib/libobs-opengl.so")).toUtf8();
		video.graphics_module = graphics.constData();
		video.fps_num = 30;
		video.fps_den = 1;
		video.base_width = video.output_width = 1280;
		video.base_height = video.output_height = 720;
		video.output_format = VIDEO_FORMAT_NV12;
		video.colorspace = VIDEO_CS_709;
		video.range = VIDEO_RANGE_PARTIAL;
		video.scale_type = OBS_SCALE_BICUBIC;
		video.gpu_conversion = true;
		Check(obs_reset_video(&video) == OBS_VIDEO_SUCCESS, "video initialization failed");
		obs_audio_info audio{48000, SPEAKERS_STEREO};
		Check(obs_reset_audio(&audio), "audio initialization failed");
		for (const char *name : {"obs-x264", "obs-ffmpeg", "obs-outputs", "rtmp-services", "image-source"}) {
			obs_module_t *module = nullptr;
			const QByteArray binary = prefix.filePath(QStringLiteral("lib/obs-plugins/%1.so").arg(name)).toUtf8();
			const QByteArray data = prefix.filePath(QStringLiteral("share/obs/obs-plugins/%1").arg(name)).toUtf8();
			Check(obs_open_module(&module, binary.constData(), data.constData()) == MODULE_SUCCESS,
			      "could not open required OBS module");
			Check(obs_init_module(module), "could not initialize required OBS module");
		}
		const QByteArray locale =
			prefix.filePath(QStringLiteral("share/obs/obs-plugins/penguin-outputs/locale/en-US.ini")).toUtf8();
		obs_module_lookup = text_lookup_create(locale.constData());
		{
			ConnectionWizard preview(nullptr, true);
			preview.show();
			QCoreApplication::processEvents();
			Check(preview.grab().save(results.filePath(QStringLiteral("setup-welcome.png"))),
			      "could not capture welcome page");
			preview.next();
			QCoreApplication::processEvents();
			Check(preview.grab().save(results.filePath(QStringLiteral("setup-connection.png"))),
			      "could not capture connection page");
		}
		{
			OBSSceneAutoRelease scene = obs_scene_create_private("Integration test program");
			OBSDataAutoRelease settings = obs_data_create();
			obs_data_set_int(settings, "width", 1280);
			obs_data_set_int(settings, "height", 720);
			obs_data_set_int(settings, "color", 0xFF88CC33);
			OBSSourceAutoRelease color = obs_source_create_private("color_source_v3", "Test color", settings);
			Check(color && obs_scene_add(scene, color), "could not create test picture");
			struct ClearProgramOnExit {
				~ClearProgramOnExit() { obs_set_output_source(0, nullptr); }
			} clearProgram;
			obs_set_output_source(0, obs_scene_get_source(scene));
			QProcess landscapeReceiver;
			QProcess portraitReceiver;
			const QString landscapeFile = results.filePath(QStringLiteral("landscape.flv"));
			const QString portraitFile = results.filePath(QStringLiteral("portrait.flv"));
			Receiver(landscapeReceiver, 19350, landscapeFile);
			Receiver(portraitReceiver, 19351, portraitFile);
			{
				RunningOutput landscape;
				RunningOutput portrait;
				Start(landscape, 19350, Layout::Landscape);
				Start(portrait, 19351, Layout::PortraitCrop);
				Check(Await([&]() { return landscape.started.load() && portrait.started.load(); }),
				      "both local outputs did not become active");
				Check(Await([&]() {
					      return obs_output_get_total_frames(landscape.output) >= 90 &&
						     obs_output_get_total_frames(portrait.output) >= 90;
				      }),
				      "outputs did not produce enough frames");
				obs_output_stop(landscape.output);
				obs_output_stop(portrait.output);
				Check(Await([&]() { return landscape.stopped.load() && portrait.stopped.load(); }),
				      "outputs did not stop");
			}
			Check(landscapeReceiver.state() == QProcess::NotRunning || landscapeReceiver.waitForFinished(10000),
			      "landscape receiver did not finish");
			Check(portraitReceiver.state() == QProcess::NotRunning || portraitReceiver.waitForFinished(10000),
			      "portrait receiver did not finish");
			CheckVideo(landscapeFile, 1280, 720);
			CheckVideo(portraitFile, 1080, 1920);
			const QString shutdownLandscape = results.filePath(QStringLiteral("shutdown-landscape.flv"));
			const QString shutdownPortrait = results.filePath(QStringLiteral("shutdown-portrait.flv"));
			Receiver(landscapeReceiver, 19350, shutdownLandscape);
			Receiver(portraitReceiver, 19351, shutdownPortrait);
			{
				RunningOutput landscape;
				RunningOutput portrait;
				Start(landscape, 19350, Layout::Landscape);
				Start(portrait, 19351, Layout::PortraitFit);
				Check(Await([&]() {
					      return obs_output_get_total_frames(landscape.output) >= 30 &&
						     obs_output_get_total_frames(portrait.output) >= 30;
				      }),
				      "restarted outputs did not produce frames");
				// Destruction while live is the same path used on profile changes and shutdown.
			}
			Check(landscapeReceiver.state() == QProcess::NotRunning || landscapeReceiver.waitForFinished(10000),
			      "landscape receiver survived active-output destruction");
			Check(portraitReceiver.state() == QProcess::NotRunning || portraitReceiver.waitForFinished(10000),
			      "portrait receiver survived active-output destruction");
			CheckVideo(shutdownLandscape, 1280, 720);
			CheckVideo(shutdownPortrait, 1080, 1920);
			QProcess reconnectReceiver;
			Receiver(reconnectReceiver, 19352, results.filePath(QStringLiteral("before-reconnect.flv")));
			const QString reconnectedFile = results.filePath(QStringLiteral("after-reconnect.flv"));
			{
				RunningOutput resilient;
				Start(resilient, 19352, Layout::Landscape);
				obs_output_set_reconnect_settings(resilient.output, 3, 1);
				Check(Await([&]() { return obs_output_get_total_frames(resilient.output) >= 30; }),
				      "reconnect test did not produce initial frames");
				reconnectReceiver.kill();
				Check(reconnectReceiver.waitForFinished(5000), "could not interrupt local receiver");
				Check(Await([&]() { return resilient.reconnecting.load(); }), "output did not attempt reconnect");
				Receiver(reconnectReceiver, 19352, reconnectedFile);
				Check(Await([&]() {
					      return !resilient.reconnecting.load() && obs_output_active(resilient.output) &&
						     obs_output_get_total_frames(resilient.output) >= 30;
				      }),
				      "output did not recover after receiver restart");
				obs_output_stop(resilient.output);
				Check(Await([&]() { return resilient.stopped.load(); }), "reconnected output did not stop");
			}
			Check(reconnectReceiver.state() == QProcess::NotRunning || reconnectReceiver.waitForFinished(10000),
			      "reconnected receiver did not finish");
			CheckVideo(reconnectedFile, 1280, 720);
			{
				RunningOutput failed;
				Start(failed, 19359, Layout::PortraitFit);
				Check(Await([&]() { return failed.stopped.load(); }), "unreachable local server did not fail");
				Check(failed.stopCode.load() != OBS_OUTPUT_SUCCESS, "unreachable local server reported success");
			}
			obs_set_output_source(0, nullptr);
		}
		std::cout << "PASS: simultaneous 1280x720 + 1080x1920 H.264/AAC, crop/fit, stop/restart, reconnect, active shutdown and unreachable-server cleanup\n";
	} catch (const std::exception &error) {
		std::cerr << "FAIL: " << error.what() << '\n';
		result = 1;
	}
	if (initialized)
		obs_shutdown();
	obs_module_free_locale();
	if (display)
		XCloseDisplay(display);
#ifdef ENABLE_WAYLAND
	if (wayland)
		wl_display_disconnect(wayland);
#endif
	return result;
}
