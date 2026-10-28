// SPDX-License-Identifier: GPL-2.0-or-later
// Isolated-profile integration probe: presentation must route to native settings.
#include <QApplication>
#include <QDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QMainWindow>
#include <QPointer>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QTimer>
#include <QToolButton>
#include <obs-frontend-api.h>
namespace {
QJsonObject report;
QPointer<QMainWindow> mainWindow;
QPointer<QDialog> settings;
int ticks = 0;
void check(const char *name, bool passed) { report[name] = passed; }
void finish() {
    QFile file(qEnvironmentVariable("PENGUIN_SMOKE_DIR") + "/dialog-navigation.json");
    if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(report).toJson());
    if (settings) settings->reject();
    if (mainWindow) mainWindow->close();
}
void tick() {
    ++ticks;
    if (!mainWindow) mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!mainWindow) return;
    if (ticks == 40) {
        check("wayland", QGuiApplication::platformName() == "wayland");
        auto *button = mainWindow->findChild<QToolButton *>("penguinBrand");
        check("brand_found", button);
        if (button) button->click();
    }
    if (ticks == 45) {
        auto *dialog = mainWindow->findChild<QDialog *>("penguinCommandCenter");
        auto *view = dialog ? dialog->findChild<QQuickWidget *>("penguinCommandView") : nullptr;
        check("command_center_ready", view && view->status() == QQuickWidget::Ready);
        if (!view) { finish(); return; }
        view->grabFramebuffer().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR") + "/command-center.png");
        auto *search = view->rootObject()->findChild<QObject *>("commandSearch");
        check("command_search_found", search);
        if (search) search->setProperty("text", "no-matching-action-123");
    }
    if (ticks == 48) {
        auto *dialog = mainWindow->findChild<QDialog *>("penguinCommandCenter");
        auto *view = dialog ? dialog->findChild<QQuickWidget *>("penguinCommandView") : nullptr;
        if (!view) { finish(); return; }
        auto *results = view->rootObject()->findChild<QObject *>("commandResults");
        check("command_empty_search", results && results->property("count").toInt() == 0);
        auto *bridge = view->rootContext()->contextProperty("commandBridge").value<QObject *>();
        int index = -1;
        for (const auto &value : bridge->property("commands").toList()) {
            const auto entry = value.toMap();
            if (entry["title"].toString().startsWith("Settings")) { index = entry["index"].toInt(); break; }
        }
        check("settings_action_found", index >= 0);
        if (index >= 0) QMetaObject::invokeMethod(bridge, "activate", Q_ARG(int, index));
    }
    if (ticks == 55) {
        for (auto *dialog : mainWindow->findChildren<QDialog *>())
            if (QByteArray(dialog->metaObject()->className()) == "OBSBasicSettings") settings = dialog;
        auto *view = settings ? settings->findChild<QQuickWidget *>("penguinSettingsNavigation") : nullptr;
        check("settings_navigation_ready", view && view->status() == QQuickWidget::Ready);
        if (!view) { finish(); return; }
        auto *list = settings->findChild<QListWidget *>("listWidget");
        check("native_navigation_retained_hidden", list && !list->isVisible());
        auto *bridge = view->rootContext()->contextProperty("settingsBridge").value<QObject *>();
        check("all_native_pages_available", bridge->property("pages").toList().size() == list->count());
        for (int i = 0; i < list->count(); ++i) {
            QMetaObject::invokeMethod(bridge, "selectPage", Q_ARG(int, i));
            report[QString("page_%1_routes_to_native").arg(i)] = list->currentRow() == i;
        }
        const int previous = list->currentRow();
        QMetaObject::invokeMethod(bridge, "selectPage", Q_ARG(int, -1));
        QMetaObject::invokeMethod(bridge, "selectPage", Q_ARG(int, list->count()));
        check("invalid_pages_ignored", list->currentRow() == previous);
        const auto flags = list->item(0)->flags();
        list->item(0)->setFlags(flags & ~Qt::ItemIsEnabled);
        QMetaObject::invokeMethod(bridge, "selectPage", Q_ARG(int, 0));
        check("disabled_page_ignored", list->currentRow() == previous);
        list->item(0)->setFlags(flags);
        QMetaObject::invokeMethod(bridge, "selectPage", Q_ARG(int, 0));
        auto *search = view->rootObject()->findChild<QObject *>("settingsSearch");
        check("settings_search_found", search);
        if (search) search->setProperty("text", "aUdIo");
    }
    if (ticks == 60 && settings) {
        auto *view = settings->findChild<QQuickWidget *>("penguinSettingsNavigation");
        auto *pages = view->rootObject()->findChild<QObject *>("settingsPageList");
        check("settings_search_case_insensitive", pages && pages->property("count").toInt() == 1);
        view->rootObject()->findChild<QObject *>("settingsSearch")->setProperty("text", "");
    }
    if (ticks == 65 && settings) {
        settings->grab().save(qEnvironmentVariable("PENGUIN_SMOKE_DIR") + "/settings-navigation.png");
        settings->reject();
    }
    if (ticks == 70) finish();
}
void start() {
    if (qEnvironmentVariable("PENGUIN_SMOKE_MODE") != "qml-dialogs") return;
    QTimer::singleShot(1000, qApp, [] {
        auto *timer = new QTimer(qApp);
        QObject::connect(timer, &QTimer::timeout, timer, [] { if (ticks < 70) tick(); });
        timer->start(100);
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
}
