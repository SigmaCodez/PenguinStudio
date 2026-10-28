// SPDX-License-Identifier: GPL-2.0-or-later
// Isolated synthetic integration test; no webcam or account access.
#include <QApplication>
#include <QTimer>
#include <QElapsedTimer>
#include <QPointer>
#include <QWidget>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs.h>
#include <obs-frontend-api.h>
#include "../frontend/utility/PenguinCameraFilters.hpp"
namespace {
QPointer<QWidget> window;
obs_scene_t *scene=nullptr;
obs_source_t *source=nullptr;
int ticks=0,phase=-1,phaseTick=0;
int64_t initial=0;
bool done=false;
QJsonObject report;
QString filename;
void finish(const char *error=nullptr){done=true;if(error)report["error"]=error;report["phases_passed"]=phase;report["platform"]=QGuiApplication::platformName();QFile f(qEnvironmentVariable("PENGUIN_RECORD_RESULT"));if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(report).toJson());obs_source_release(source);source=nullptr;obs_scene_release(scene);scene=nullptr;if(window)window->close();}
void cleanup(obs_frontend_event e,void*){if(e==OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP){done=true;obs_source_release(source);source=nullptr;obs_scene_release(scene);scene=nullptr;}}
void beginPhase(){
 phaseTick=ticks;
 if(phase==4){auto f=PenguinCamera::CameraMatting(source);obs_source_set_enabled(f,false);return;}
 if(phase==5){auto f=PenguinCamera::CameraMatting(source);obs_source_set_enabled(f,true);return;}
 const bool small=phase==1||phase==2;
 QImage image(small?321:640,small?193:360,QImage::Format_ARGB32);
 for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)image.setPixel(x,y,qRgb((x*7+phase*31)%256,(y*5)%256,(x+y)%256));
 filename=QFileInfo(qEnvironmentVariable("PENGUIN_RECORD_RESULT")).absolutePath()+QString("/fixture-%1.png").arg(phase);
 if(!image.save(filename)){finish("fixture write failed");return;}
 OBSDataAutoRelease settings=obs_data_create();obs_data_set_string(settings,"file",filename.toUtf8().constData());obs_source_update(source,settings);
 if(!PenguinCamera::Apply(source,"Camera appearance",3,phase==2?4:phase==3?5:1,25,15,8,filename.toUtf8().constData(),true,phase==1||phase==2?"gpu":"cpu"))finish("filter apply failed");
 if(phase==0){
  for(int delay : {20,25,30}) QTimer::singleShot(delay,qApp,[delay]{
   if(done||!source)return;
   QElapsedTimer timer;timer.start();
   const bool ok=PenguinCamera::Apply(source,"Camera appearance",3,4,25,15,9,filename.toUtf8().constData(),true,delay==20?"gpu":"cpu");
   const double elapsed=timer.nsecsElapsed()/1000000.0;
   report["rapid_update_max_ms"]=std::max(report["rapid_update_max_ms"].toDouble(),elapsed);
   if(!ok)finish("rapid settings update failed");
  });
 }

}
void tick(){
 if(done)return;
 ++ticks;
 if(!window)for(auto*w:QApplication::topLevelWidgets())if(QByteArray(w->metaObject()->className())=="OBSBasic")window=w;
 if(!window)return;
 if(phase<0&&ticks>=30){obs_frontend_add_event_callback(cleanup,nullptr);for(unsigned i=1;i<=6;i++)obs_set_output_source(i,nullptr);scene=obs_scene_create("Synthetic lifecycle");source=obs_source_create("image_source","Changing fixture",nullptr,nullptr);if(!scene||!source){finish("source creation failed");return;}obs_scene_add(scene,source);obs_frontend_set_current_scene(obs_scene_get_source(scene));phase=0;beginPhase();}
 if(source&&ticks>phaseTick+20){
  auto f=PenguinCamera::CameraMatting(source);calldata_t s;calldata_init(&s);if(f)proc_handler_call(obs_source_get_proc_handler(f),"camera_status",&s);
  const int state=calldata_int(&s,"status");const bool preparing=calldata_bool(&s,"preparing"),gpu=calldata_bool(&s,"gpu");const int64_t completed=calldata_int(&s,"completed_frames");calldata_free(&s);
  const bool small=phase==1||phase==2;
  bool ready=phase==4|| (state==2&&!preparing&&completed>=initial+5&&obs_source_get_width(source)==(small?321u:640u)&&obs_source_get_height(source)==(small?193u:360u)&&(!(phase==1||phase==2)||gpu));
  if(ready&&phase==0){
   OBSDataAutoRelease settings=obs_source_get_settings(f);
   if(gpu||std::string(obs_data_get_string(settings,"useGPU"))!="cpu"||obs_data_get_int(settings,"blur_background")!=9){finish("latest settings were not applied");return;}
   report["latest_update_applied"]=true;
  }
  if(ready){initial=completed;++phase;if(phase==6){finish();return;}beginPhase();}
 }
 if(ticks>1200)finish("phase timeout");
}
void start(){if(qEnvironmentVariableIsEmpty("PENGUIN_RECORD_RESULT"))return;auto*t=new QTimer(qApp);QObject::connect(t,&QTimer::timeout,tick);t->start(100);}
Q_COREAPP_STARTUP_FUNCTION(start)
}
