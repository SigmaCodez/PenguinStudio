#include <QApplication>
#include <QTimer>
#include <QWidget>
#include <QPushButton>
#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
namespace {
QWidget *window=nullptr; QDialog *settings=nullptr,*updater=nullptr; int ticks=0; QJsonObject checks;
void tick(){
 ++ticks;
 if(!window)for(auto*w:QApplication::topLevelWidgets())if(w->objectName()=="OBSBasic")window=w;
 if(!window)return;
 if(ticks==30)QMetaObject::invokeMethod(window,"on_action_Settings_triggered",Qt::QueuedConnection);
 if(ticks==50){settings=qobject_cast<QDialog*>(QApplication::activeModalWidget()); auto*b=settings?settings->findChild<QPushButton*>("penguinSettingsCheckUpdates"):nullptr; checks["settings_button_present"]=b!=nullptr; if(b)b->click();}
 if(ticks==70){updater=qobject_cast<QDialog*>(QApplication::activeModalWidget());checks["updater_above_settings"]=updater&&updater->objectName()=="penguinLinuxUpdater"&&settings&&settings->isVisible();}
 if(ticks==140){
  if(updater){updater->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/settings-updater.png");QStringList texts;for(auto*l:updater->findChildren<QLabel*>())texts<<l->text();checks["updater_text"]=texts.join("\n");updater->reject();}
 }
 if(ticks==150){checks["settings_restored"]=settings&&QApplication::activeModalWidget()==settings; if(settings)settings->reject();}
 if(ticks==165){QFile f(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/settings-update-checks.json");if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(checks).toJson());window->close();}
}
void start(){if(qEnvironmentVariableIsEmpty("PENGUIN_SMOKE_DIR"))return;auto*t=new QTimer(qApp);QObject::connect(t,&QTimer::timeout,tick);t->start(100);}
Q_COREAPP_STARTUP_FUNCTION(start)
}
