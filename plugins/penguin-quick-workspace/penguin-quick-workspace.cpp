// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include "SimpleWorkspace.hpp"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QListWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QToolButton>
#include <QAction>
#include <functional>
#include <QPointer>
#include <QScopedValueRollback>
#include <QQmlContext>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTimer>
#include <QVariantList>
#include <util/config-file.h>

OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_description(void) { return "Optional Penguin Studio QML navigation preview"; }

// Keep native settings state and validation authoritative; replace presentation only.
class SettingsNavigation final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList pages READ pages CONSTANT)
    Q_PROPERTY(int currentPage READ currentPage NOTIFY currentPageChanged)
    Q_PROPERTY(qreal uiScale READ uiScale CONSTANT)
    QPointer<QListWidget> list;
    QVariantList entries;
public:
    SettingsNavigation(QListWidget *source, QObject *parent) : QObject(parent), list(source)
    {
        const QStringList icons{"inspector", "scene", "outputs", "sources", "audio", "scene", "details", "inspector", "details"};
        for (int i = 0; i < source->count(); ++i) {
            entries.append(QVariantMap{{"index", i}, {"title", source->item(i)->text()},
                                      {"icon", icons.value(i, "details")}});
        }
        connect(source, &QListWidget::currentRowChanged, this, &SettingsNavigation::currentPageChanged);
    }
    QVariantList pages() const { return entries; }
    int currentPage() const { return list ? list->currentRow() : -1; }
    qreal uiScale() const {
        const auto value = config_get_int(obs_frontend_get_user_config(), "Appearance", "FontScale");
        return std::clamp(value > 0 ? value / 10.0 : 1.0, 0.8, 2.0);
    }
    Q_INVOKABLE void selectPage(int index) {
        if (list && index >= 0 && index < list->count() && !list->item(index)->isHidden()
            && (list->item(index)->flags() & Qt::ItemIsEnabled))
            list->setCurrentRow(index);
    }
signals:
    void currentPageChanged();
};

class CommandCenter final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList commands READ commands CONSTANT)
    Q_PROPERTY(qreal uiScale READ uiScale CONSTANT)
    QPointer<QDialog> dialog;
    QList<QPointer<QAction>> actions;
    QList<QList<QPointer<QAction>>> ancestors;
    QVariantList entries;
public:
    CommandCenter(QMenu *menu, QDialog *window, QObject *parent) : QObject(parent), dialog(window) {
        std::function<void(QMenu *, const QString &, QList<QPointer<QAction>>)> collect =
            [&](QMenu *current, const QString &path, QList<QPointer<QAction>> parents) {
            for (auto *action : current->actions()) {
                if (action->isSeparator() || !action->isVisible()) continue;
                QString title = action->text();
                title.replace("&&", "\x01"); title.remove('&'); title.replace("\x01", "&");
                if (action->menu()) { auto next = parents; next.append(action);
                    collect(action->menu(), path.isEmpty() ? title : path + " / " + title, next); continue; }
                const int index = actions.size();
                actions.append(action);
                ancestors.append(parents);
                const bool parentEnabled = std::all_of(parents.cbegin(), parents.cend(),
                    [](const auto &parent) { return parent && parent->isEnabled() && parent->isVisible(); });
                entries.append(QVariantMap{{"index", index}, {"title", title}, {"group", path},
                    {"enabled", action->isEnabled() && parentEnabled}, {"checked", action->isCheckable() && action->isChecked()},
                    {"shortcut", action->shortcut().toString(QKeySequence::NativeText)}});
            }
        };
        collect(menu, {}, {});
    }
    QVariantList commands() const { return entries; }
    qreal uiScale() const {
        const auto value = config_get_int(obs_frontend_get_user_config(), "Appearance", "FontScale");
        return std::clamp(value > 0 ? value / 10.0 : 1.0, 0.8, 2.0);
    }
    Q_INVOKABLE void dismiss() { if (dialog) dialog->reject(); }
    Q_INVOKABLE void activate(int index) {
        if (index < 0 || index >= actions.size()) return;
        QPointer<QAction> action = actions[index];
        if (!action || !action->isEnabled() || !action->isVisible()) return;
        const auto parents = ancestors[index];
        if (!std::all_of(parents.cbegin(), parents.cend(),
            [](const auto &parent) { return parent && parent->isEnabled() && parent->isVisible(); })) return;
        if (dialog) dialog->accept();
        // Trigger after the palette has closed; keep native confirmations and state checks.
        QTimer::singleShot(0, action, [action, parents] {
            if (action && action->isEnabled() && action->isVisible() &&
                std::all_of(parents.cbegin(), parents.cend(),
                    [](const auto &parent) { return parent && parent->isEnabled() && parent->isVisible(); }))
                action->trigger();
        });
    }
};

class DialogDesigner final : public QObject {
public:
    explicit DialogDesigner(QObject *parent) : QObject(parent) { qApp->installEventFilter(this); }
    void installCommandCenter(QMainWindow *main) {
        auto *brand = main ? main->findChild<QToolButton *>("penguinBrand") : nullptr;
        if (!brand || !brand->menu() || brand->property("penguinCommandCenter").toBool()) return;
        brand->setProperty("penguinCommandCenter", true);
        brand->setPopupMode(QToolButton::DelayedPopup);
        brand->setToolTip("Open command center. Hold for all menus.");
        connect(brand, &QToolButton::clicked, this, [brand, main] {
            if (!brand->menu()) return;
            auto *dialog = new QDialog(main);
            dialog->setObjectName("penguinCommandCenter");
            dialog->setWindowTitle("Penguin Studio — Command center");
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->resize(640, 620);
            auto *layout = new QVBoxLayout(dialog);
            layout->setContentsMargins(0, 0, 0, 0);
            auto *view = new QQuickWidget(dialog);
            view->setObjectName("penguinCommandView");
            view->setResizeMode(QQuickWidget::SizeRootObjectToView);
            view->setClearColor(QColor("#11141b"));
            auto *commands = new CommandCenter(brand->menu(), dialog, view);
            view->rootContext()->setContextProperty("commandBridge", commands);
            view->setSource(QUrl("qrc:/penguin/CommandCenter.qml"));
            if (view->status() == QQuickWidget::Error) { delete dialog; return; }
            layout->addWidget(view);
            dialog->open();
        });
    }
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() != QEvent::Show || QByteArray(object->metaObject()->className()) != "OBSBasicSettings")
            return false;
        auto *dialog = qobject_cast<QDialog *>(object);
        if (!dialog || dialog->property("penguinSettingsNavigation").toBool()) return false;
        dialog->setProperty("penguinSettingsNavigation", true);
        QTimer::singleShot(0, dialog, [dialog] {
            auto *list = dialog->findChild<QListWidget *>("listWidget");
            if (!list || !dialog->layout()) return;
            auto *view = new QQuickWidget(dialog);
            view->setObjectName("penguinSettingsNavigation");
            view->setResizeMode(QQuickWidget::SizeRootObjectToView);
            view->setClearColor(QColor("#11141b"));
            auto *navigation = new SettingsNavigation(list, view);
            view->setFixedWidth(int(224 * navigation->uiScale()));
            view->setMinimumHeight(240);
            view->rootContext()->setContextProperty("settingsBridge", navigation);
            view->setSource(QUrl("qrc:/penguin/SettingsNavigation.qml"));
            if (view->status() == QQuickWidget::Error) { delete view; return; }
            auto *old = dialog->layout()->replaceWidget(list, view);
            if (!old) { delete view; return; }
            delete old;
            list->hide();
            view->show();
        });
        return false;
    }
};
static QPointer<DialogDesigner> dialogDesigner;
static QPointer<SimpleWorkspace> simpleWorkspace;

class WorkspaceBridge final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList scenes READ scenes NOTIFY scenesChanged)
    Q_PROPERTY(QString currentScene READ currentScene NOTIFY selectionChanged)
    Q_PROPERTY(bool live READ live NOTIFY statusChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY statusChanged)
    Q_PROPERTY(qreal uiScale READ uiScale NOTIFY scaleChanged)
    Q_PROPERTY(QString activePanel READ activePanel NOTIFY panelChanged)
    qreal textScale = 1.0;
    QString selectedPanel = "penguinInspectorDock";
    bool switchingPanel = false, trackingPanels = false;
    QVariantList items;
    QString selected;
    bool streaming = false, capturing = false;
public:
    explicit WorkspaceBridge(QObject *parent) : QObject(parent) {}
    QVariantList scenes() const { return items; }
    QString currentScene() const { return selected; }
    bool live() const { return streaming; }
    bool recording() const { return capturing; }
    qreal uiScale() const { return textScale; }
    QString activePanel() const { return selectedPanel; }
    void refresh()
    {
        QVariantList next;
        obs_frontend_source_list list{};
        obs_frontend_get_scenes(&list);
        for (size_t i = 0; i < list.sources.num; ++i) {
            auto *source = list.sources.array[i];
            next.append(QVariantMap{{"id", QString::fromUtf8(obs_source_get_uuid(source))},
                                    {"name", QString::fromUtf8(obs_source_get_name(source))}});
        }
        obs_frontend_source_list_free(&list);
        auto *source = obs_frontend_get_current_scene();
        const QString current = source ? QString::fromUtf8(obs_source_get_uuid(source)) : QString();
        obs_source_release(source);
        const bool output = obs_frontend_streaming_active(), record = obs_frontend_recording_active();
        if (next != items) { items = next; emit scenesChanged(); }
        if (current != selected) { selected = current; emit selectionChanged(); }
        if (streaming != output || capturing != record) {
            streaming = output; capturing = record; emit statusChanged();
        }
        const auto setting = config_get_int(obs_frontend_get_user_config(), "Appearance", "FontScale");
        const qreal scale = std::clamp(setting > 0 ? setting / 10.0 : 1.0, 0.8, 2.0);
        if (scale != textScale) { textScale = scale; emit scaleChanged(); }
    }
    Q_INVOKABLE void selectScene(const QString &id)
    {
        // Resolve only current scenes, so stale IDs cannot select another source.
        obs_frontend_source_list list{};
        obs_frontend_get_scenes(&list);
        for (size_t i = 0; i < list.sources.num; ++i) {
            auto *source = list.sources.array[i];
            if (QString::fromUtf8(obs_source_get_uuid(source)) == id) {
                obs_frontend_set_current_scene(source);
                break;
            }
        }
        obs_frontend_source_list_free(&list);
    }
    void trackPanels()
    {
        if (trackingPanels) return;
        trackingPanels = true;
        auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        for (const char *name : {"penguinInspectorDock", "penguin-broadcast-destinations", "penguin-stream-details", "penguinTwitchChat"}) {
            if (auto *panel = main->findChild<QDockWidget *>(name)) {
                const QString id = QString::fromLatin1(name);
                connect(panel, &QDockWidget::visibilityChanged, this, [this, main, id](bool visible) {
                    if (visible && !switchingPanel && main->property("penguinSimpleWorkspace").toBool()) openPanel(id);
                });
            }
        }
    }
    Q_INVOKABLE void openPanel(const QString &id)
    {
        if (switchingPanel) return;
        QScopedValueRollback<bool> guard(switchingPanel, true);
        static const QStringList allowed{"penguinInspectorDock", "penguin-broadcast-destinations",
                                         "penguin-stream-details", "penguinTwitchChat", "sourcesDock", "mixerDock", "scenesDock"};
        if (!allowed.contains(id)) return;
        auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        if (!main) return;
        if (auto *dock = main->findChild<QDockWidget *>(id)) {
            const QStringList panels{"penguinInspectorDock", "penguin-broadcast-destinations", "penguin-stream-details", "penguinTwitchChat"};
            if (panels.contains(id)) {
                for (const auto &other : panels) if (other != id)
                    if (auto *panel = main->findChild<QDockWidget *>(other)) panel->hide();
                if (selectedPanel != id) { selectedPanel = id; emit panelChanged(); }
            }
            dock->show(); dock->raise();
        }
    }
    Q_INVOKABLE void arrangeWorkspace()
    {
        QScopedValueRollback<bool> guard(switchingPanel, true);
        auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        auto *config = obs_frontend_get_user_config();
        if (!main || !config) return;
        auto *navigation = main->findChild<QDockWidget *>("penguinQuickWorkspace");
        auto *sources = main->findChild<QDockWidget *>("sourcesDock");
        auto *mixer = main->findChild<QDockWidget *>("mixerDock");
        auto *scenes = main->findChild<QDockWidget *>("scenesDock");
        if (!navigation || !sources || !mixer || !scenes) return;
        if (!config_has_user_value(config, "PenguinQuickPreview", "PreviousLayout")) {
            const auto previous = main->saveState().toBase64();
            config_set_string(config, "PenguinQuickPreview", "PreviousLayout", previous.constData());
        }
        if (config_get_int(config, "PenguinQuickPreview", "DesignLayout") < 2 &&
            !config_has_user_value(config, "PenguinQuickPreview", "BeforeSimpleLayout")) {
            const auto previous = main->saveState().toBase64();
            config_set_string(config, "PenguinQuickPreview", "BeforeSimpleLayout", previous.constData());
        }
        main->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        navigation->setFloating(false);
        main->addDockWidget(Qt::LeftDockWidgetArea, navigation);
        navigation->show();
        scenes->hide();
        sources->setFloating(false);
        mixer->setFloating(false);
        main->addDockWidget(Qt::BottomDockWidgetArea, sources);
        main->splitDockWidget(sources, mixer, Qt::Horizontal);
        sources->show();
        mixer->show();
        main->resizeDocks({navigation}, {int(240 * textScale)}, Qt::Horizontal);
        main->resizeDocks({sources, mixer}, {280, 400}, Qt::Horizontal);
        if (auto *inspector = main->findChild<QDockWidget *>("penguinInspectorDock")) {
            inspector->setFloating(false);
            main->addDockWidget(Qt::RightDockWidgetArea, inspector);
            for (const char *name : {"penguin-broadcast-destinations", "penguin-stream-details", "penguinTwitchChat"}) {
                if (auto *panel = main->findChild<QDockWidget *>(name)) {
                    panel->setFloating(false);
                    main->addDockWidget(Qt::RightDockWidgetArea, panel);
                    main->tabifyDockWidget(inspector, panel);
                    panel->hide();
                }
            }
            inspector->show(); inspector->raise();
        }
        selectedPanel = "penguinInspectorDock"; emit panelChanged();
        config_set_int(config, "PenguinQuickPreview", "DesignLayout", 2);
    }
    Q_INVOKABLE void restorePreviousLayout()
    {
        QScopedValueRollback<bool> guard(switchingPanel, true);
        auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        auto *config = obs_frontend_get_user_config();
        if (!main || !config) return;
        const char *key = config_has_user_value(config, "PenguinQuickPreview", "BeforeSimpleLayout") ? "BeforeSimpleLayout" : "PreviousLayout";
        const char *saved = config_get_string(config, "PenguinQuickPreview", key);
        if (saved && *saved) main->restoreState(QByteArray::fromBase64(saved));
    }
    Q_INVOKABLE void showLayoutMenu()
    {
        auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        if (!main) return;
        auto *menu = new QMenu(main);
        menu->addAction("Arrange workspace", this, &WorkspaceBridge::arrangeWorkspace);
        auto *restore = menu->addAction("Restore previous layout", this, &WorkspaceBridge::restorePreviousLayout);
        restore->setEnabled(config_has_user_value(obs_frontend_get_user_config(), "PenguinQuickPreview", "PreviousLayout"));
        QObject::connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);
        auto *view = qobject_cast<QWidget *>(parent());
        menu->popup(view ? view->mapToGlobal(QPoint(16, view->height() - 40)) : main->mapToGlobal(QPoint(20, 60)));
    }

signals:
    void scenesChanged();
    void selectionChanged();
    void statusChanged();
    void scaleChanged();
    void panelChanged();
};

static QPointer<WorkspaceBridge> bridge;
static bool exiting = false;
static constexpr auto DockId = "penguinQuickWorkspace";
static void createWorkspace()
{
    if (bridge || exiting) return;
    auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!main) return;
    // This static navigation must not share Qt Quick OpenGL framebuffer handling
    // with OBS native video surfaces on Wayland (Qt 6.10 FBO recursion on resize).
    // OBS rendering, encoders and camera inference keep their own GPU backends.
    if (QGuiApplication::platformName() == "wayland")
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    auto *view = new QQuickWidget;
    view->setObjectName("penguinQuickView");
    view->setMinimumSize(228, 420);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    view->setClearColor(QColor("#11141b"));
    bridge = new WorkspaceBridge(view);
    bridge->setObjectName("penguinWorkspaceBridge");
    bridge->refresh();
    auto resizeMinimum = [view] {
        if (bridge) view->setMinimumSize(int(228 * bridge->uiScale()), int(420 * bridge->uiScale()));
    };
    QObject::connect(bridge, &WorkspaceBridge::scaleChanged, view, resizeMinimum);
    resizeMinimum();
    view->rootContext()->setContextProperty("bridge", bridge.data());
    view->setSource(QUrl("qrc:/penguin/Navigation.qml"));
    if (view->status() == QQuickWidget::Error) {
        blog(LOG_WARNING, "[Penguin QML] Navigation unavailable; existing workspace retained");
        delete view;
        return;
    }
    if (!obs_frontend_add_dock_by_id(DockId, "Workspace", view)) { delete view; return; }
    if (auto *dock = main->findChild<QDockWidget *>(DockId)) {
        dock->setFloating(false);
        main->addDockWidget(Qt::LeftDockWidgetArea, dock, Qt::Horizontal);
        main->resizeDocks({dock}, {240}, Qt::Horizontal);
    }
}
static void event(obs_frontend_event type, void *)
{
    if (type == OBS_FRONTEND_EVENT_EXIT) {
        exiting = true;
        delete dialogDesigner.data();
        obs_frontend_remove_dock(DockId);
        return;
    }
    if (type == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
        if (!bridge) return;
        QTimer::singleShot(0, bridge.data(), [] {
            if (!bridge || exiting) return;
            bridge->refresh();
            if (dialogDesigner) dialogDesigner->installCommandCenter(static_cast<QMainWindow *>(obs_frontend_get_main_window()));
            auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
            if (!simpleWorkspace) {
                simpleWorkspace = new SimpleWorkspace(mainWindow, dialogDesigner);
                if (auto *navigation = mainWindow->findChild<QDockWidget *>(DockId)) {
                    QObject::connect(navigation, &QDockWidget::visibilityChanged, simpleWorkspace, [window = QPointer<QMainWindow>(mainWindow)](bool visible) {
                        if (simpleWorkspace && !exiting && window && window->isVisible()) simpleWorkspace->setActive(visible);
                    });
                    simpleWorkspace->setActive(navigation->isVisible());
                    bridge->trackPanels();
                }
            }
            auto *config = obs_frontend_get_user_config();
            if (config_get_bool(config, "PenguinQuickPreview", "Placed")) {
                if (config_get_int(config, "PenguinQuickPreview", "DesignLayout") < 2) bridge->arrangeWorkspace();
                return;
            }
            auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
            if (auto *dock = main->findChild<QDockWidget *>(DockId)) {
                dock->setFloating(false);
                main->addDockWidget(Qt::LeftDockWidgetArea, dock, Qt::Horizontal);
                dock->show();
                main->resizeDocks({dock}, {240}, Qt::Horizontal);
                config_set_bool(config, "PenguinQuickPreview", "Placed", true);
                if (config_get_int(config, "PenguinQuickPreview", "DesignLayout") < 2) bridge->arrangeWorkspace();
            }
        });
        return;
    }
    if (bridge) {
        QTimer::singleShot(0, bridge, [] { if (bridge && !exiting) bridge->refresh(); });
    }
}
bool obs_module_load(void)
{
    obs_frontend_add_event_callback(event, nullptr);
    return true;
}
void obs_module_post_load(void)
{
    createWorkspace();
    if (!dialogDesigner) dialogDesigner = new DialogDesigner(static_cast<QObject *>(static_cast<QMainWindow *>(obs_frontend_get_main_window())));
}
void obs_module_unload(void)
{
    if (exiting) return; // Frontend callbacks are gone after EXIT.
    if (simpleWorkspace) simpleWorkspace->setActive(false);
    delete dialogDesigner.data();
    obs_frontend_remove_event_callback(event, nullptr);
    if (bridge) obs_frontend_remove_dock(DockId);
}
#include "penguin-quick-workspace.moc"
