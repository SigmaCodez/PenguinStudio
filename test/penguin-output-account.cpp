// SPDX-License-Identifier: GPL-2.0-or-later
// Isolated GUI regression: synthetic credentials only, never starts an output.
#include "../shared/penguin-account-setup.hpp"
#include "../shared/penguin-streaming-access.hpp"
#include <obs-frontend-api.h>
#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>
#include <QWizard>
namespace {
QPointer<QMainWindow> mainWindow;
QJsonArray checks;
int ticks=0;
void Check(bool ok,const char *name){checks.append(QJsonObject{{"name",name},{"passed",ok}});}
void Tick(){
 ++ticks;
 if(!mainWindow) for(auto*w:QApplication::topLevelWidgets()) if(QByteArray(w->metaObject()->className())=="OBSBasic")mainWindow=qobject_cast<QMainWindow*>(w);
 if(!mainWindow)return;
 if(ticks==4)for(auto*w:QApplication::topLevelWidgets())if(auto*d=qobject_cast<QDialog*>(w))d->reject();
 if(ticks!=10)return;
 auto*list=mainWindow->findChild<QTreeWidget*>("penguinDestinationList");
 Check(list && list->topLevelItemCount()==1,"Legacy destination restored");
 if(!list || list->topLevelItemCount()!=1){mainWindow->close();return;}
 Check(list->topLevelItem(0)->text(2)=="Sign in to Twitch","Account destination asks for sign-in instead of a key");
 auto*access=PenguinStreamingAccess::Get(mainWindow);
 access->Update("twitch",{{},"synthetic-twitch","Synthetic account",{}});
 Check(list->topLevelItem(0)->text(2)=="Ready (connected account)","Shared sign-in immediately configures existing output");
 access->Update("youtube",{"rtmps://example.invalid/live","synthetic-youtube","Synthetic broadcast",{}});
 list->setCurrentItem(list->topLevelItem(0));
 auto*edit=mainWindow->findChild<QPushButton*>("penguinEditDestination");edit->click();
 auto*w=mainWindow->findChild<QWizard*>("penguinDestinationWizard");
 Check(w!=nullptr,"Destination editor opens");if(!w){mainWindow->close();return;}
 auto*mode=w->findChild<QCheckBox*>("penguinDestinationUseAccount");
 auto*key=w->findChild<QLineEdit*>("penguinDestinationKey");
 Check(mode->isChecked() && !key->isVisible() && key->text()=="synthetic-twitch","Editor reuses sign-in with no manual key field");
 mode->setChecked(false);
 Check(key->isVisible() && key->text().isEmpty(),"Manual alternative clears hidden account key");
 key->setText("synthetic-manual");
 access->Update("twitch",{{},"synthetic-rotated","Synthetic account",{}});
 Check(key->text()=="synthetic-manual","Account refresh cannot overwrite manual input");
 w->button(QWizard::FinishButton)->click();
 access->Update("twitch",{});
 Check(list->topLevelItem(0)->text(2)!="Sign in to Twitch","Manual destination survives account disconnect");
 edit->click();w=mainWindow->findChild<QWizard*>("penguinDestinationWizard");
 // Closed dialogs are deleted on the next event turn; find the visible editor.
 for(auto*c:mainWindow->findChildren<QWizard*>("penguinDestinationWizard"))if(c->isVisible())w=c;
 mode=w->findChild<QCheckBox*>("penguinDestinationUseAccount");key=w->findChild<QLineEdit*>("penguinDestinationKey");
 Check(!mode->isChecked() && key->text()=="synthetic-manual","Manual selection is retained when editing");
 auto*real=mainWindow->findChild<QAction*>("penguinConnectAccountAction");
 if(real)real->setObjectName("realAccountAction");
 auto*fake=new QAction(mainWindow);fake->setObjectName("penguinConnectAccountAction");
 QPointer<PenguinAccountSetupRequest> request;
 QObject::connect(fake,&QAction::triggered,mainWindow,[&]{request=dynamic_cast<PenguinAccountSetupRequest*>(fake->property("penguinSetupRequest").value<QObject*>());fake->setProperty("penguinSetupRequest",QVariant());});
 w->findChild<QPushButton*>("penguinDestinationConnect")->click();
 Check(request && request->provider=="twitch","Output editor requests streaming account setup");
 if(request)request->complete({},"synthetic-new","Synthetic account");
 Check(mode->isChecked() && !key->isVisible() && key->text()=="synthetic-new","Account completion fills streaming access and hides manual key");
 w->findChild<QComboBox*>("penguinDestinationPlatform")->setCurrentText("YouTube");
 Check(!request && key->text()=="synthetic-youtube","Provider switch cancels old request and uses the matching account");
 w->button(QWizard::FinishButton)->click();
 Check(list->topLevelItem(0)->text(2)=="Ready (connected account)","YouTube output is ready without copying a key");
 access->Update("youtube",{});
 Check(list->topLevelItem(0)->text(2)=="Sign in to YouTube","Disconnect invalidates account-based output credentials");
 Check(!obs_frontend_streaming_active() && !obs_frontend_recording_active(),"Account propagation never starts outputs");
 QFile saved(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/../config/PenguinStudio/plugin_config/penguin-outputs/destinations.json");
 bool opened=saved.open(QIODevice::ReadOnly);auto json=saved.readAll();
 Check(opened && !json.contains("synthetic-") && json.contains("useAccount"),"Saved destinations contain mode but no credentials");
 access->Update("youtube",{"rtmps://example.invalid/live","synthetic-youtube","Synthetic broadcast",{}});
 mainWindow->findChild<QPushButton*>("penguinAddDestination")->click();
 for(auto*c:mainWindow->findChildren<QWizard*>("penguinDestinationWizard"))if(c->isVisible())w=c;
 Check(w->findChild<QLineEdit*>("penguinDestinationKey")->text()=="synthetic-new","New destinations reuse the current account");
 w->button(QWizard::FinishButton)->click();
 Check(list->topLevelItemCount()==2,"Both providers can remain configured together");
 QObject::disconnect(fake,nullptr,mainWindow,nullptr);
 QCoreApplication::processEvents();
 if(auto*d=mainWindow->findChild<QDockWidget*>("penguin-broadcast-destinations")){d->show();d->raise();mainWindow->resize(1280,800);QCoreApplication::processEvents();d->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/account-destinations.png");}
 QFile out(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/output-account-checks.json");if(out.open(QIODevice::WriteOnly))out.write(QJsonDocument(QJsonObject{{"checks",checks}}).toJson());
 mainWindow->close();
}
void Start(){if(qEnvironmentVariableIsEmpty("PENGUIN_SMOKE_DIR"))return;auto*t=new QTimer(qApp);QObject::connect(t,&QTimer::timeout,Tick);t->start(500);QTimer::singleShot(40000,qApp,&QCoreApplication::quit);}
}
Q_COREAPP_STARTUP_FUNCTION(Start)
