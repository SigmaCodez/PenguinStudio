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
        if (!maps.open(QIODevice::ReadOnly)) {
            QCoreApplication::exit(43);
            return;
        }
        QFile evidence(qEnvironmentVariable("PENGUIN_TEST_RESULT") + ".maps");
        const QByteArray loadedMaps = maps.readAll();
        if (!evidence.open(QIODevice::WriteOnly) || evidence.write(loadedMaps) != loadedMaps.size()) {
            QCoreApplication::exit(44);
            return;
        }
        QFile output(qEnvironmentVariable("PENGUIN_TEST_RESULT"));
        const QByteArray report = QJsonDocument(result).toJson();
        if (!output.open(QIODevice::WriteOnly) || output.write(report) != report.size()) {
            QCoreApplication::exit(45);
            return;
        }
        if (!main || !main->isVisible())
            QCoreApplication::exit(42);
        else
            main->close();
    });
}
Q_COREAPP_STARTUP_FUNCTION(check)
