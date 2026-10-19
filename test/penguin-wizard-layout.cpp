// SPDX-License-Identifier: GPL-2.0-or-later
// Optional LD_PRELOAD regression probe. Use only an isolated test profile.
// Captures are Qt widget renders, not compositor screenshots.
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QPointer>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QTimer>
#include <QWizard>

namespace {
QPointer<QMainWindow> mainWindow;
int ticks = 0;

void Tick()
{
	++ticks;
	for (auto *widget : QApplication::topLevelWidgets()) {
		if (QByteArray(widget->metaObject()->className()) == "OBSBasic")
			mainWindow = qobject_cast<QMainWindow *>(widget);
		if (ticks < 12)
			if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible())
				dialog->reject();
	}
	if (!mainWindow)
		return;
	if (ticks == 14) {
		if (auto *action = mainWindow->findChild<QAction *>("autoConfigure"))
			QTimer::singleShot(0, action, &QAction::trigger);
	}
	if (ticks == 18 || ticks == 20) {
		for (auto *widget : QApplication::topLevelWidgets()) {
			if (auto *wizard = qobject_cast<QWizard *>(widget); wizard && wizard->isVisible()) {
				if (ticks == 18)
					wizard->next();
				else
					wizard->resize(514, 304);
			}
		}
	}
	if (ticks != 22)
		return;

	QJsonArray fields;
	bool captured = false;
	const QString directory = qEnvironmentVariable("PENGUIN_WIZARD_RESULTS");
	for (auto *widget : QApplication::topLevelWidgets()) {
		auto *wizard = qobject_cast<QWizard *>(widget);
		if (!wizard || !wizard->isVisible())
			continue;
		captured = wizard->grab().save(directory + "/wizard.png");
		QList<QRect> previousFields;
		for (auto *combo : wizard->findChildren<QComboBox *>()) {
			if (!combo->isVisible())
				continue;
			QStyleOptionComboBox option;
			option.initFrom(combo);
			option.currentText = combo->currentText();
			option.editable = combo->isEditable();
			const QRect content = combo->style()->subControlRect(
				QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, combo);
			const QRect bounds(combo->mapTo(wizard, QPoint()), combo->size());
			bool fits = content.height() >= combo->fontMetrics().height();
			for (const auto &previous : previousFields)
				fits = fits && !previous.intersects(bounds);
			previousFields.append(bounds);
			fields.append(QJsonObject{{"name", combo->objectName()},
						 {"height", combo->height()},
						 {"contentHeight", content.height()},
						 {"fontHeight", combo->fontMetrics().height()},
						 {"text", combo->currentText()},
						 {"y", bounds.y()},
						 {"passed", fits}});
		}
		wizard->reject();
	}
	QFile report(directory + "/wizard.json");
	if (report.open(QIODevice::WriteOnly))
		report.write(QJsonDocument(QJsonObject{{"platform", QGuiApplication::platformName()},
						      {"captured", captured}, {"fields", fields}}).toJson());
	mainWindow->close();
}

void Start()
{
	if (qEnvironmentVariableIsEmpty("PENGUIN_WIZARD_RESULTS"))
		return;
	QTimer::singleShot(1000, qApp, [] {
		auto *timer = new QTimer(qApp);
		QObject::connect(timer, &QTimer::timeout, Tick);
		timer->start(250);
	});
}
Q_COREAPP_STARTUP_FUNCTION(Start)
} // namespace
