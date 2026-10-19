// SPDX-License-Identifier: GPL-2.0-or-later
// Optional LD_PRELOAD regression probe. Use only an isolated test profile.
// Captures are Qt widget renders, not compositor screenshots.
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMainWindow>
#include <QTimer>
#include <QWizard>
#include <QStyleOptionComboBox>
#include <QStyle>
namespace {
QMainWindow *mainWindow=nullptr;
int ticks=0;
void tick() {
 ++ticks;
 for(auto *w: QApplication::topLevelWidgets()) {
  if(QByteArray(w->metaObject()->className())=="OBSBasic") mainWindow=qobject_cast<QMainWindow*>(w);
  if(ticks<12) if(auto *d=qobject_cast<QDialog*>(w); d && d->isVisible()) d->reject();
 }
 if(!mainWindow)return;
 if(ticks==14) {auto *a=mainWindow->findChild<QAction*>("autoConfigure"); if(a)QTimer::singleShot(0,a,&QAction::trigger);}
 if(ticks==18) for(auto *w:QApplication::topLevelWidgets()) if(auto *d=qobject_cast<QWizard*>(w);d && d->isVisible())d->next();
 if(ticks==20) for(auto *w:QApplication::topLevelWidgets()) if(auto *d=qobject_cast<QWizard*>(w); d && d->isVisible()) d->resize(514,304);
 if(ticks==22){
  QJsonArray fields;
  int previousBottom = -1;
  for(auto *w:QApplication::topLevelWidgets())if(auto *d=qobject_cast<QWizard*>(w); d && d->isVisible()){
   d->grab().save(qEnvironmentVariable("PENGUIN_WIZARD_RESULTS")+"/wizard.png");
   for(auto *c:d->findChildren<QComboBox*>())if(c->isVisible()){
    QStyleOptionComboBox o;o.initFrom(c);o.currentText=c->currentText();o.editable=c->isEditable();
    auto r=c->style()->subControlRect(QStyle::CC_ComboBox,&o,QStyle::SC_ComboBoxEditField,c);
    const int top = c->mapTo(d, QPoint()).y();
    const bool fits = r.height() >= c->fontMetrics().height() && top >= previousBottom;
    previousBottom = top + c->height();
    fields.append(QJsonObject{{"name",c->objectName()},{"height",c->height()},{"contentHeight",r.height()},{"fontHeight",c->fontMetrics().height()},{"text",c->currentText()},{"y",top},{"passed",fits}});
   }
   d->reject();
  }
  QFile f(qEnvironmentVariable("PENGUIN_WIZARD_RESULTS")+"/wizard.json");if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(QJsonObject{{"platform",QGuiApplication::platformName()},{"fields",fields}}).toJson());
  QFile q(qEnvironmentVariable("PENGUIN_WIZARD_RESULTS")+"/theme.qss");if(q.open(QIODevice::WriteOnly))q.write(qApp->styleSheet().toUtf8());
  mainWindow->close();
 }
}
void start(){if(qEnvironmentVariableIsEmpty("PENGUIN_WIZARD_RESULTS"))return; QTimer::singleShot(1000,qApp,[]{auto *t=new QTimer(qApp);QObject::connect(t,&QTimer::timeout,tick);t->start(250);});}
Q_COREAPP_STARTUP_FUNCTION(start)
}
