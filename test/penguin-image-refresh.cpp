// SPDX-License-Identifier: GPL-2.0-or-later
// Isolated-profile integration probe for asynchronous image timestamp polling.
#include <QApplication>
#include <QTimer>
#include <QMainWindow>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-frontend-api.h>
namespace {
QJsonObject report;
obs_source_t *source = nullptr, *sceneSource = nullptr;
obs_sceneitem_t *item = nullptr;
int ticks = 0;
QString path(const char *name) { return QDir(qEnvironmentVariable("PENGUIN_SMOKE_DIR")).filePath(name); }
bool writeImage(const char *name, int width, int height)
{
 QImage image(width, height, QImage::Format_ARGB32);image.fill(Qt::green);return image.save(path(name));
}
void finish()
{
 if(item) obs_sceneitem_remove(item);
 obs_source_release(source);obs_source_release(sceneSource);source=nullptr;sceneSource=nullptr;item=nullptr;
 QFile file(path("image-refresh.json"));if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(report).toJson());
 if(auto *window=static_cast<QMainWindow*>(obs_frontend_get_main_window()))window->close();
}
void tick()
{
 ++ticks;
 if(ticks==30){
  report["wayland"]=QGuiApplication::platformName()=="wayland";
  report["fixture_written"]=writeImage("watched.png",64,48);
  obs_data_t *settings=obs_data_create();obs_data_set_string(settings,"file",path("watched.png").toUtf8());
  source=obs_source_create("image_source","File refresh fixture",settings,nullptr);obs_data_release(settings);
  sceneSource=obs_frontend_get_current_scene();auto *scene=obs_scene_from_source(sceneSource);
  if(!source||!scene){report["source_created"]=false;finish();return;}
  item=obs_scene_add(scene,source);report["source_created"]=bool(item);
 }
 if(ticks==60){
  report["initial_size"]=source&&obs_source_get_width(source)==64&&obs_source_get_height(source)==48;
  report["replacement_written"]=writeImage("watched.png",96,64);
 }
 if(ticks==100){
  report["changed_file_reloaded"]=source&&obs_source_get_width(source)==96&&obs_source_get_height(source)==64;
  report["new_path_written"]=writeImage("replacement.png",128,72);
  if(source){auto *settings=obs_source_get_settings(source);obs_data_set_string(settings,"file",path("replacement.png").toUtf8());obs_source_update(source,settings);obs_data_release(settings);}
  writeImage("watched.png",160,100);
 }
 if(ticks==140){
  report["new_path_retained"]=source&&obs_source_get_width(source)==128&&obs_source_get_height(source)==72;
  finish();
 }
}
void start()
{
 if(qEnvironmentVariable("PENGUIN_SMOKE_MODE")!="image-refresh")return;
 auto *timer=new QTimer(qApp);QObject::connect(timer,&QTimer::timeout,[]{tick();});timer->start(100);
}
Q_COREAPP_STARTUP_FUNCTION(start)
}
