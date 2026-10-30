// SPDX-License-Identifier: GPL-2.0-or-later
// Native Wayland regression probe. Isolated profile and synthetic content only.
#include "../frontend/widgets/OBSQTDisplay.hpp"
#include <obs-frontend-api.h>
#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QTimer>
#include <QWebEngineView>
#include <QWindow>
#include <atomic>
namespace {
QPointer<QMainWindow> window;
QPointer<QDockWidget> browser;
std::atomic_uint frames{0};
obs_display_t *last = nullptr;
unsigned checkpoint = 0;
int ticks = 0;
QJsonArray checks;
void Check(bool ok, const char *name) { checks.append(QJsonObject{{"name", name}, {"passed", ok}}); }
void Draw(void *, uint32_t, uint32_t) { ++frames; }
class Observer final : public QObject {
public:
 using QObject::QObject;
 bool eventFilter(QObject *, QEvent *event) override {
  if (event->type() == QEvent::PlatformSurface &&
      static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
   last = nullptr;
  return false;
 }
};
void AddBrowser() {
 browser = new QDockWidget("Synthetic browser", window);
 browser->setObjectName("syntheticBrowserDock");
 auto *view = new QWebEngineView(browser);
 browser->setWidget(view);
 window->addDockWidget(Qt::RightDockWidgetArea, browser);
 view->setHtml("<html><body style='background:#222;color:white'>Local synthetic browser test</body></html>");
 browser->show();
}
void Tick() {
 ++ticks;
 if (!window) for (auto *w : QApplication::topLevelWidgets())
  if (QByteArray(w->metaObject()->className()) == "OBSBasic") window = qobject_cast<QMainWindow *>(w);
 if (!window) return;
 auto *widget = window->findChild<QWidget *>("preview");
 auto *display = widget ? static_cast<OBSQTDisplay *>(widget) : nullptr;
 if (ticks == 4) {
  OBSDataAutoRelease settings = obs_data_create();
  obs_data_set_int(settings, "color", 0xff0000ff);
  obs_data_set_int(settings, "width", 1280); obs_data_set_int(settings, "height", 720);
  OBSSourceAutoRelease source = obs_source_create("color_source_v3", "Synthetic red", settings, nullptr);
  OBSSceneAutoRelease scene = obs_scene_create("Preview lifecycle test");
  obs_scene_add(scene, source); obs_frontend_set_current_scene(obs_scene_get_source(scene));
  if (widget && widget->windowHandle()) widget->windowHandle()->installEventFilter(new Observer(widget));
  Check(QGuiApplication::platformName() == "wayland", "Native Wayland platform");
 }
 if (display && display->GetDisplay() && display->GetDisplay() != last) {
  last = display->GetDisplay(); obs_display_add_draw_callback(last, Draw, nullptr);
 }
 if (ticks == 10) {
  Check(frames.load() > 30, "Preview rendered before browser creation"); checkpoint = frames.load();
  obs_frontend_recording_start();
 }
 if (ticks == 12) AddBrowser();
 if (ticks == 20) {
  Check(display && display->GetDisplay() && obs_display_enabled(display->GetDisplay()), "Native display restored after browser creation");
  Check(frames.load() > checkpoint + 60, "Preview frames continue after browser creation");
  checkpoint = frames.load();
 }
 if (ticks == 23 && browser) { delete browser.data(); }
 if (ticks == 28) AddBrowser();
 if (ticks == 35) {
  Check(display && display->GetDisplay() && obs_display_enabled(display->GetDisplay()), "Display survives browser replacement");
  Check(frames.load() > checkpoint + 120, "Frames continue across browser replacement");
  Check(obs_frontend_recording_active(), "Recording survives native surface changes");
  obs_frontend_recording_stop();
 }
 if (ticks == 40) {
  Check(!obs_frontend_recording_active(), "Recording stops cleanly");
  QFile file(qEnvironmentVariable("PENGUIN_SMOKE_DIR") + "/preview-webengine-checks.json");
  if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(QJsonObject{{"checks", checks}, {"frames", int(frames.load())}}).toJson());
  window->close();
 }
}
void Start() {
 if (qEnvironmentVariableIsEmpty("PENGUIN_SMOKE_DIR")) return;
 auto *timer = new QTimer(qApp); QObject::connect(timer, &QTimer::timeout, Tick); timer->start(1000);
}
Q_COREAPP_STARTUP_FUNCTION(Start)
}
