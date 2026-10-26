// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-License-Identifier: GPL-2.0-or-later
// Test-only preload helper. Explicit focus setup isolates keyboard event handling
// from compositor activation, which Wayland need not grant to automated windows.
#include <QApplication>
#include <QTimer>
#include <QMainWindow>
#include <QDockWidget>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlContext>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QKeyEvent>
#include <QFocusEvent>
#include <obs.h>
#include <obs-frontend-api.h>
namespace {
bool done=false;int ticks=0;QJsonObject report;QPointer<QMainWindow> main;QPointer<QQuickWidget> view;
void check(const char *name,bool ok){report[name]=ok;}
void finish(){done=true;QFile f(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/qml-checks.json");if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(report).toJson());if(main)main->close();}
void tick(){
 if(done)return;
 ++ticks;
 if(!main)main=static_cast<QMainWindow*>(obs_frontend_get_main_window());
 if(!main)return;
 if(!view)view=main->findChild<QQuickWidget*>("penguinQuickView");
 if(ticks==40){
  check("qml_ready",view&&view->status()==QQuickWidget::Ready);
  check("native_wayland",QGuiApplication::platformName()=="wayland");
  if(!view){finish();return;}
  auto *dock=main->findChild<QDockWidget*>("penguinQuickWorkspace");if(qEnvironmentVariableIsSet("PENGUIN_EXPECT_HIDDEN")){check("hidden_state_restored",dock&&!dock->isVisible());dock->show();}
  for (const char *name : {"Starting soon", "Main stage"}) { auto *existing=obs_get_source_by_name(name); if(!existing) {auto *scene=obs_scene_create(name);obs_scene_release(scene);} obs_source_release(existing); }
 }
 if(ticks==55 && view){
  QObject *bridge=view->rootContext()->contextProperty("bridge").value<QObject*>();
  auto items=bridge->property("scenes").toList();check("scene_model_populated",items.size()>=3);
  if(!items.empty())QMetaObject::invokeMethod(bridge,"selectScene",Q_ARG(QString,items.last().toMap()["id"].toString()));
  auto *button=view->rootObject()->findChild<QQuickItem*>("navChat");
  check("chat_button_found",button);if(button){view->setFocus(Qt::OtherFocusReason);button->forceActiveFocus();report["button_focus"]=button->hasFocus();}
 }
 if(ticks==62 && view){auto *button=view->rootObject()->findChild<QQuickItem*>("navChat");QFocusEvent focus(QEvent::FocusIn, Qt::OtherFocusReason);QApplication::sendEvent(view,&focus);QApplication::sendEvent(view->quickWindow(),&focus);view->quickWindow()->contentItem()->forceActiveFocus();view->rootObject()->forceActiveFocus();button->forceActiveFocus();report["active_focus"]=button->hasActiveFocus();report["focus_policy"]=int(view->focusPolicy());report["root_focus"]=view->rootObject()->hasFocus();report["root_active_focus"]=view->rootObject()->hasActiveFocus();report["window_active"]=view->quickWindow()->isActive();report["window_visible"]=view->quickWindow()->isVisible();QKeyEvent key(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(view,&key);}
 if(ticks==65 && view){
  QObject *bridge=view->rootContext()->contextProperty("bridge").value<QObject*>();
  auto *source=obs_frontend_get_current_scene();check("scene_selection_synced",source&&bridge->property("currentScene").toString()==QString::fromUtf8(obs_source_get_uuid(source)));obs_source_release(source);
  auto *chat=main->findChild<QDockWidget*>("penguinTwitchChat");check("keyboard_opens_chat",chat&&chat->isVisible());
  for(auto pair:{qMakePair("navInspector","penguinInspectorDock"),qMakePair("navOutputs","penguin-broadcast-destinations"),qMakePair("navDetails","penguin-stream-details")}){
   auto *button=view->rootObject()->findChild<QObject*>(pair.first);if(button)QMetaObject::invokeMethod(button,"triggered");auto *dock=main->findChild<QDockWidget*>(pair.second);check(pair.first,dock&&dock->isVisible());
  }
 }
 if(ticks==75&&view){auto *dock=main->findChild<QDockWidget*>("penguinQuickWorkspace");dock->hide();check("classic_fallback",!dock->isVisible()&&main->findChild<QWidget*>("penguinTopBar"));dock->show();}
 if(ticks==85&&view){
  auto image=view->grabFramebuffer();check("qml_capture",!image.isNull());image.save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/navigation.png");main->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR")+"/workspace.png");auto *dock=main->findChild<QDockWidget*>("penguinQuickWorkspace"); check("docked",dock && !dock->isFloating()); check("visible",dock && dock->isVisible()); report["dock_x"]=dock->x(); report["dock_y"]=dock->y(); check("usable_width",view->width()>=228);if(qEnvironmentVariableIsSet("PENGUIN_HIDE_FINAL"))dock->hide();finish();
 }
 if(ticks>100)finish();
}
void start(){if(qEnvironmentVariable("PENGUIN_SMOKE_MODE")!="qml")return;QTimer::singleShot(1000,qApp,[]{auto *timer=new QTimer(qApp);QObject::connect(timer,&QTimer::timeout,tick);timer->start(100);});}
Q_COREAPP_STARTUP_FUNCTION(start)
}
