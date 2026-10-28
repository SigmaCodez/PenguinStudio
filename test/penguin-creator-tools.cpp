// SPDX-License-Identifier: GPL-2.0-or-later
// Synthetic-only Wayland integration test. Never loads a real account or camera.
#include <obs.h>
#include <obs-frontend-api.h>
#include <obs.hpp>
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QStatusBar>
#include <QLabel>
#include <QTreeWidget>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QToolButton>
#include <QStyleOptionButton>
#include <QStyle>
#include <cmath>
#include <QTabWidget>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QDir>
namespace {
QPointer<QWidget> window, dialog;
QPointer<QPushButton> shield;
OBSSource fixture, tone, addedTone;
OBSScene scene;
QJsonArray checks;
int tickCount = 0;
void check(bool ok, const char *name) { checks.append(QJsonObject{{"name", name}, {"passed", ok}}); }
QPushButton *button(const QString &prefix) {
 if (!dialog) return nullptr;
 for(auto *b : dialog->findChildren<QPushButton*>()) if(b->text().startsWith(prefix))return b;
 return nullptr;
}
void tick() {
 ++tickCount;
 if(!window)for(auto*w:QApplication::topLevelWidgets())if(QByteArray(w->metaObject()->className())=="OBSBasic")window=w;
 if(!window)return;
 if(tickCount==4){
  if(qEnvironmentVariableIsSet("PENGUIN_TEST_UPLOAD")) {
   auto *label=window->findChild<QLabel*>("penguinUploadRate");
   auto *poller=window->findChild<QTimer*>("penguinOutputMonitor");
   check(label && label->isVisible() && label->text().contains("—"),"Upload status is visible while offline");
   check(poller && !poller->isActive(),"Output monitor sleeps while offline");
   OBSDataAutoRelease settings=obs_data_create();
   obs_data_set_string(settings,"server","rtmp://127.0.0.1:19428/live");
   obs_data_set_string(settings,"key","test");
   OBSServiceAutoRelease service=obs_service_create("rtmp_custom","Local upload test",settings,nullptr);
   obs_frontend_set_streaming_service(service);
  }
  for(uint32_t i=1;i<MAX_CHANNELS;++i)obs_set_output_source(i,nullptr);
  OBSDataAutoRelease image=obs_data_create();obs_data_set_int(image,"color",0xff32d8f5);obs_data_set_int(image,"width",1280);obs_data_set_int(image,"height",720);
  fixture=OBSSourceAutoRelease(obs_source_create("color_source_v3","Synthetic yellow",image,nullptr));
  scene=OBSSceneAutoRelease(obs_scene_create("Creator tools validation"));obs_scene_add(scene,fixture);
  OBSDataAutoRelease audio=obs_data_create();const auto path=qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/Microphone test tone.wav";
  obs_data_set_string(audio,"local_file",path.toUtf8().constData());obs_data_set_bool(audio,"is_local_file",true);obs_data_set_bool(audio,"looping",true);
  tone=OBSSourceAutoRelease(obs_source_create("ffmpeg_source","Synthetic voice",audio,nullptr));obs_scene_add(scene,tone);
  OBSSourceAutoRelease compressor=obs_source_create("compressor_filter","Existing user compressor",nullptr,nullptr);
  obs_source_filter_add(tone,compressor);
  obs_frontend_set_current_scene(obs_scene_get_source(scene));
  for(auto*a:window->findChildren<QAction*>())if(a->text()=="Creator tools"){a->trigger();break;}
  dialog=window->findChild<QDialog*>("penguinCreatorTools");check(bool(dialog),"Creator tools opens");
  if(dialog) { shield=dialog->findChild<QPushButton*>("creatorPrivacy"); }
  check(bool(shield),"Privacy action exists");
  obs_frontend_recording_start();
  if(qEnvironmentVariableIsSet("PENGUIN_TEST_UPLOAD"))obs_frontend_streaming_start();
 }
 if(tickCount==8&&shield){shield->click();check(shield->isChecked(),"Privacy activated");}
 if(tickCount==10){
  OBSDataAutoRelease settings=obs_source_get_settings(tone);
  addedTone=OBSSourceAutoRelease(obs_source_create("ffmpeg_source","Added while private",settings,nullptr));
  obs_scene_add(scene,addedTone);
  OBSSourceAutoRelease guard=obs_source_get_filter_by_name(addedTone,"Penguin privacy shield");check(bool(guard),"New audio source protected while private");
  obs_frontend_source_list transitions{};obs_frontend_get_transitions(&transitions);
  if(transitions.sources.num>1)obs_frontend_set_current_transition(transitions.sources.array[1]);
  obs_frontend_source_list_free(&transitions);
  auto *bar=window->findChild<QWidget*>("penguinPrivacyButton");check(bool(bar),"Persistent privacy toolbar control exists");
  auto *privacyButton=window->findChild<QToolButton*>("penguinPrivacyButton");
  auto *settingsButton=window->findChild<QToolButton*>("penguinSettingsButton");
  auto *recordButton=window->findChild<QToolButton*>("penguinRecordButton");
  check(privacyButton && settingsButton && privacyButton->parentWidget()==settingsButton->parentWidget(),"Privacy shares main action layout");
  check(privacyButton && recordButton && privacyButton->height()==recordButton->height(),"Privacy matches recording button height");
  check(privacyButton && !privacyButton->icon().isNull(),"Privacy icon retained in active state");
 }
 if(tickCount==11){
  OBSDataAutoRelease settings=obs_data_create();obs_data_set_int(settings,"color",0xfff57432);obs_data_set_int(settings,"width",1280);obs_data_set_int(settings,"height",720);
  OBSSourceAutoRelease image=obs_source_create("color_source_v3","New private scene",settings,nullptr);
  OBSSceneAutoRelease other=obs_scene_create("Private scene change");obs_scene_add(other,image);obs_scene_add(other,tone);
  obs_frontend_set_current_scene(obs_scene_get_source(other));
 }
 if(tickCount==16&&shield){shield->click();check(!shield->isChecked(),"Privacy released");}
 if(tickCount==17&&dialog){
  if(qEnvironmentVariableIsSet("PENGUIN_TEST_UPLOAD")) {
   auto *label=window->findChild<QLabel*>("penguinUploadRate");
   auto *poller=window->findChild<QTimer*>("penguinOutputMonitor");
   check(obs_frontend_streaming_active(),"Loopback stream is active");
   check(label && (label->text().contains("Mbps") || label->text().contains("kbps")) && !label->text().contains(" 0 kbps"),"Upload reports measured nonzero bitrate");
   check(poller && poller->isActive(),"Output monitor wakes for streaming");
   if(auto *bar=window->findChild<QStatusBar*>())bar->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/upload-status.png");
  }
  dialog->findChild<QTabWidget*>()->setCurrentIndex(1);
  auto *summary=dialog->findChild<QLabel*>("creatorSessionSummary");
  check(summary && summary->text().contains("Recording: Recording"),"Session summary reflects active recording");
  check(summary && summary->text().contains("Written data:") && summary->text().contains("Privacy: Off"),"Session summary contains output metrics and privacy state");
  auto *copy=dialog->findChild<QPushButton*>("creatorCopyDiagnostics");
  check(copy && copy->isEnabled(),"Safe diagnostics action is available");
  bool advice=false;
  for(auto *label:dialog->findChildren<QLabel*>())if(label->text()=="Recording resilience")advice=true;
  check(advice,"Recording resilience advice is present");
  dialog->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/creator-performance.png");
 }
 if(tickCount==18&&dialog){
  auto *tabs=dialog->findChild<QTabWidget*>();tabs->setCurrentIndex(2);
  auto *combo=dialog->findChild<QComboBox*>("creatorMicrophone");check(combo&&combo->count()>0,"Audio source list populated");
  if(combo)combo->setCurrentIndex(combo->findText("Synthetic voice"));
  if(auto*b=button("Measure voice"))b->click();
 }
 if(tickCount==30&&dialog){
  auto *noise=dialog->findChild<QCheckBox*>();if(noise)noise->setChecked(true);
  auto*b=button("Apply suggested");check(b&&b->isEnabled(),"Measured audio produces suggestion");if(b)b->click();
  OBSSourceAutoRelease limiter=obs_source_get_filter_by_name(tone,"Creator voice limiter");check(bool(limiter),"Limiter added");
  OBSSourceAutoRelease suppression=obs_source_get_filter_by_name(tone,"Creator voice noise suppression");check(bool(suppression),"Optional noise suppression added");
  obs_source_set_volume(tone,0.42f);
  if(auto*u=button("Undo assistant"))u->click();
  check(std::abs(obs_source_get_volume(tone)-0.42f)<0.0001f,"Undo preserves later manual volume");
  OBSSourceAutoRelease existing=obs_source_get_filter_by_name(tone,"Existing user compressor");check(bool(existing),"Existing user compressor preserved");
  OBSSourceAutoRelease newCompressor=obs_source_get_filter_by_name(tone,"Creator voice compressor");check(!newCompressor,"No duplicate compressor created");
  obs_source_set_volume(tone,1.0f);
  OBSSourceAutoRelease removed=obs_source_get_filter_by_name(tone,"Creator voice limiter");check(!removed,"Undo removes assistant filter");
  bool labelsFit=true;
  for(auto *control:dialog->findChildren<QPushButton*>())if(control->isVisible()){
   QStyleOptionButton option;option.initFrom(control);option.text=control->text();
   const auto content=control->style()->subElementRect(QStyle::SE_PushButtonContents,&option,control);
   labelsFit=labelsFit && content.width()>=control->fontMetrics().horizontalAdvance(control->text());
  }
  check(labelsFit,"Visible button labels fit their contents");
  dialog->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/creator-audio.png");
  dialog->findChild<QTabWidget*>()->setCurrentIndex(0);
  if(auto*r=button("Run stream check"))r->click();
  dialog->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/creator-checks.png");
 }
 if(tickCount==32&&shield)shield->click();
 if(tickCount==34){
  obs_frontend_recording_stop();
  if(qEnvironmentVariableIsSet("PENGUIN_TEST_UPLOAD"))obs_frontend_streaming_stop();
 }
 if(tickCount==38){
  check(!obs_frontend_recording_active(),"Recording stopped");
  if(qEnvironmentVariableIsSet("PENGUIN_TEST_UPLOAD")) {
   auto *label=window->findChild<QLabel*>("penguinUploadRate");
   auto *poller=window->findChild<QTimer*>("penguinOutputMonitor");
   check(!obs_frontend_streaming_active() && label && label->text().contains("—"),"Upload resets after streaming stops");
   check(poller && !poller->isActive(),"Output monitor returns to sleep");
  }
  auto *summary=dialog ? dialog->findChild<QLabel*>("creatorSessionSummary") : nullptr;
  check(summary && summary->text().contains("Recording: Stopped"),"Session summary reflects stopped recording");
  QFile file(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/creator-checks.json");if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"checks",checks},{"platform",QGuiApplication::platformName()}}).toJson());
  fixture=nullptr;tone=nullptr;addedTone=nullptr;scene=nullptr;
  window->close();
 }
}
void start(){if(qEnvironmentVariableIsEmpty("PENGUIN_SMOKE_DIR"))return;auto*t=new QTimer(qApp);QObject::connect(t,&QTimer::timeout,tick);t->start(1000);}
Q_COREAPP_STARTUP_FUNCTION(start)
}
