// SPDX-License-Identifier: GPL-2.0-or-later
// LD_PRELOAD helper: use only with an isolated packaging-test profile.
#include <QApplication>
#include <QTimer>
#include <QWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <cstring>
static void check()
{
    QTimer::singleShot(10000, qApp, [] {
        QWidget *main = nullptr;
        for (auto *widget : QApplication::topLevelWidgets())
            if (std::strcmp(widget->metaObject()->className(), "OBSBasic") == 0)
                main = widget;
        QJsonObject result;
        result["platform"] = QGuiApplication::platformName();
        result["main_window_visible"] = main && main->isVisible();
        result["executable"] = QCoreApplication::applicationFilePath();
        QFile maps("/proc/self/maps");
        maps.open(QIODevice::ReadOnly);
        QFile evidence(qEnvironmentVariable("PENGUIN_TEST_RESULT") + ".maps");
        evidence.open(QIODevice::WriteOnly);
        evidence.write(maps.readAll());
        QFile output(qEnvironmentVariable("PENGUIN_TEST_RESULT"));
        output.open(QIODevice::WriteOnly);
        output.write(QJsonDocument(result).toJson());
        if (!main || !main->isVisible())
            QCoreApplication::exit(42);
        else
            main->close();
    });
}
Q_COREAPP_STARTUP_FUNCTION(check)
