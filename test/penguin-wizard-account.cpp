// SPDX-License-Identifier: GPL-2.0-or-later
// Real wizard with a synthetic account bridge. Never logs in or starts outputs.
#include "../shared/penguin-account-setup.hpp"
#include <obs-frontend-api.h>
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QWizard>
namespace {
QPointer<QMainWindow> mainWindow;
QPointer<QWizard> wizard;
QPointer<PenguinAccountSetupRequest> request;
QJsonArray checks;
int ticks = 0;
void Check(bool passed, const char *name) { checks.append(QJsonObject{{"name", name}, {"passed", passed}}); }
void Finish() {
 QFile out(qEnvironmentVariable("PENGUIN_SMOKE_DIR") + "/wizard-account-checks.json");
 if (out.open(QIODevice::WriteOnly)) out.write(QJsonDocument(QJsonObject{{"checks",checks}}).toJson());
 if (wizard) wizard->reject();
 if (mainWindow) mainWindow->close();
}
void Tick() {
 ++ticks;
 if (!mainWindow) for (auto *w : QApplication::topLevelWidgets())
  if (QByteArray(w->metaObject()->className()) == "OBSBasic") mainWindow = qobject_cast<QMainWindow *>(w);
 if (!mainWindow) { if(ticks>40) qApp->quit(); return; }
 if (ticks == 4) {
  for (auto *w : QApplication::topLevelWidgets()) if (auto *d = qobject_cast<QDialog *>(w)) d->reject();
  auto *real = mainWindow->findChild<QAction *>("penguinConnectAccountAction");
  Check(real != nullptr, "Account module provides wizard bridge");
  if (real) real->setObjectName("realAccountAction");
  auto *fake = new QAction(mainWindow); fake->setObjectName("penguinConnectAccountAction");
  QObject::connect(fake, &QAction::triggered, mainWindow, [fake] {
   request = dynamic_cast<PenguinAccountSetupRequest *>(fake->property("penguinSetupRequest").value<QObject *>());
   fake->setProperty("penguinSetupRequest", QVariant());
  });
  QMetaObject::invokeMethod(mainWindow, "on_autoConfigure_triggered", Qt::QueuedConnection);
 }
 if (ticks == 8) {
  for (auto *w : QApplication::topLevelWidgets()) if (QByteArray(w->metaObject()->className()) == "AutoConfig") wizard = qobject_cast<QWizard *>(w);
  Check(bool(wizard), "Auto-configuration wizard opens");
  if (!wizard) { Finish(); return; }
  wizard->findChild<QRadioButton *>("prioritizeStreaming")->click(); wizard->next(); wizard->next();
 }
 if (ticks == 10 && wizard) {
  auto *page=wizard->currentPage(); auto *service=page->findChild<QComboBox *>("service");
  auto *key=page->findChild<QLineEdit *>("key"); auto *connect=page->findChild<QPushButton *>("connectAccount");
  auto *manual=page->findChild<QPushButton *>("useStreamKey");
  service->setCurrentIndex(service->findText("Twitch"));
  QCoreApplication::processEvents();
  Check(connect->isVisible() && manual->isVisible() && !key->isVisible(), "Twitch defaults to account sign-in with optional key");
  Check(!page->isComplete() && !wizard->button(QWizard::NextButton)->isEnabled(), "No credentials cannot advance");
  wizard->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/twitch-sign-in.png");
  manual->click(); key->setText("synthetic-manual");
  Check(key->isVisible() && page->isComplete(), "Manual stream key remains available");
  page->findChild<QPushButton *>("connectAccount2")->click();
  Check(request && request->provider=="twitch", "Twitch button requests matching provider");
  if(request) request->complete({}, "synthetic-twitch", "Synthetic Twitch account");
  Check(page->isComplete() && !key->isVisible() && key->text()=="synthetic-twitch", "Twitch account supplies hidden key and enables Next");
  int youtube=-1;for(int i=0;i<service->count();++i)if(service->itemText(i).startsWith("YouTube")){youtube=i;break;}
  Check(youtube>=0,"YouTube service available"); service->setCurrentIndex(youtube);
  QCoreApplication::processEvents();
  Check(!request && key->text().isEmpty() && !page->isComplete(), "Service switch discards pending account and credentials");
  Check(connect->isVisible() && !key->isVisible(), "YouTube defaults to account sign-in");
  wizard->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/youtube-sign-in.png");
  connect->click(); Check(request && request->provider=="youtube", "YouTube button requests matching provider");
  if(request) request->complete("rtmps://a.rtmps.youtube.com/live2", "synthetic-youtube", "Synthetic upcoming broadcast");
  auto *bandwidth=page->findChild<QCheckBox *>("doBandwidthTest");
  Check(page->isComplete() && !key->isVisible(), "YouTube account completes setup without manual key entry");
  Check(!bandwidth->isChecked() && !bandwidth->isEnabled(), "Account-connected YouTube cannot send bandwidth-test video");
  Check(page->validatePage(), "Resolved YouTube stream settings validate");
  Check(!obs_frontend_streaming_active() && !obs_frontend_recording_active(), "Account setup never starts outputs");
  QCoreApplication::processEvents();
  wizard->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/youtube-connected.png");
  page->findChild<QPushButton *>("useStreamKeyAdv")->click();
  Check(key->isVisible() && key->text().isEmpty() && bandwidth->isEnabled(), "Manual alternative clears account key and restores controls");
  Finish();
 }
 if(ticks>60){Check(false,"Wizard timed out");Finish();}
}
void Start(){if(qEnvironmentVariableIsEmpty("PENGUIN_SMOKE_DIR"))return;auto*t=new QTimer(qApp);QObject::connect(t,&QTimer::timeout,Tick);t->start(500);}
Q_COREAPP_STARTUP_FUNCTION(Start)
}
