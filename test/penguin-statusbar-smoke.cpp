// Local English synthetic status-bar review, no camera or network outputs.
#include <QApplication>
#include <QTimer>
#include <QWidget>
#include <QLabel>
#include <QStatusBar>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs.h>
#include <obs-frontend-api.h>
namespace {
QWidget *window=nullptr,*bar=nullptr;obs_scene_t *scene=nullptr;int ticks=0;bool done=false;QJsonObject checks;
void capture(const char *name){if(bar)bar->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/"+name+".png");}
QWidget *part(const char *name){return bar?bar->findChild<QWidget*>(name):nullptr;}
void check(const char *name,bool passed){checks[name]=passed;}
void cleanup(obs_frontend_event e,void*){if(e==OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP){done=true;obs_scene_release(scene);scene=nullptr;}}
void tick(){
 if(done)return;++ticks;if(!window)for(auto*w:QApplication::topLevelWidgets())if(w->objectName()=="OBSBasic")window=w;
 if(!window)return;if(!bar)bar=window->findChild<QStatusBar*>();
 if(ticks==30){
  obs_frontend_add_event_callback(cleanup,nullptr);for(unsigned i=1;i<=6;i++)obs_set_output_source(i,nullptr);
  scene=obs_scene_create("Status bar review");obs_data_t*d=obs_data_create();obs_data_set_int(d,"color",0xff342720);obs_data_set_int(d,"width",1280);obs_data_set_int(d,"height",720);auto*s=obs_source_create("color_source","Synthetic background",d,nullptr);obs_data_release(d);if(s){obs_scene_add(scene,s);obs_source_release(s);}obs_frontend_set_current_scene(obs_scene_get_source(scene));
  check("idle_outputs_hidden",part("recordFrame")&&!part("recordFrame")->isVisible()&&part("streamFrame")&&!part("streamFrame")->isVisible());capture("idle");obs_frontend_recording_start();
 }
 if(ticks==70){auto*l=bar->findChild<QLabel*>("recordTime");check("recording_visible",obs_frontend_recording_active()&&part("recordFrame")->isVisible());check("english_recording_label",l&&l->text().startsWith("Recording"));auto*g=bar->findChild<QLabel*>("gpuUsage");check("gpu_real_adapter",g&&g->text().contains("%")&&g->toolTip().contains("9070"));capture("recording");obs_frontend_recording_pause(true);}
 if(ticks==100){auto*l=bar->findChild<QLabel*>("recordTime");check("paused_label",obs_frontend_recording_paused()&&l&&l->text().startsWith("Paused"));capture("paused");obs_frontend_recording_pause(false);}
 if(ticks==120){
  for(const char *n:{"recordTime","cpuUsage","gpuUsage","fpsCurrent"}){auto*l=bar->findChild<QLabel*>(n);check(n,l&&l->width()>=l->fontMetrics().horizontalAdvance(l->text())&&l->alignment().testFlag(Qt::AlignHCenter)&&l->height()>=l->fontMetrics().height()&&l->parentWidget()->rect().contains(l->geometry())&&bar->rect().contains(QRect(l->mapTo(bar,QPoint()),l->size())));}
  window->resize(900,650);capture("compact");obs_frontend_recording_stop();
 }
 if(ticks==160){check("stopped_idle",!obs_frontend_recording_active()&&!part("recordFrame")->isVisible());capture("stopped");QFile f(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/statusbar-checks.json");if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(checks).toJson());done=true;obs_scene_release(scene);scene=nullptr;window->close();}
}
void start(){if(qEnvironmentVariableIsEmpty("PENGUIN_SMOKE_DIR"))return;auto*t=new QTimer(qApp);QObject::connect(t,&QTimer::timeout,tick);t->start(100);}
Q_COREAPP_STARTUP_FUNCTION(start)
}
