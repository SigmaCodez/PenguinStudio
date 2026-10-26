// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QQmlContext>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTimer>
#include <QVariantList>
#include <util/config-file.h>

OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_description(void) { return "Optional Penguin Studio QML navigation preview"; }

class WorkspaceBridge final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList scenes READ scenes NOTIFY scenesChanged)
    Q_PROPERTY(QString currentScene READ currentScene NOTIFY selectionChanged)
    Q_PROPERTY(bool live READ live NOTIFY statusChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY statusChanged)
    Q_PROPERTY(qreal uiScale READ uiScale NOTIFY scaleChanged)
    qreal textScale = 1.0;
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
    Q_INVOKABLE void openPanel(const QString &id)
    {
        static const QStringList allowed{"penguinInspectorDock", "penguin-broadcast-destinations",
                                         "penguin-stream-details", "penguinTwitchChat", "sourcesDock", "mixerDock", "scenesDock"};
        if (!allowed.contains(id)) return;
        auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        if (auto *dock = main->findChild<QDockWidget *>(id)) { dock->show(); dock->raise(); }
    }
    Q_INVOKABLE void arrangeWorkspace()
    {
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
        config_set_int(config, "PenguinQuickPreview", "DesignLayout", 1);
    }
    Q_INVOKABLE void restorePreviousLayout()
    {
        auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
        auto *config = obs_frontend_get_user_config();
        if (!main || !config) return;
        const char *saved = config_get_string(config, "PenguinQuickPreview", "PreviousLayout");
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
    if (!obs_frontend_add_dock_by_id(DockId, "Workspace preview", view)) { delete view; return; }
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
        obs_frontend_remove_dock(DockId);
        return;
    }
    if (type == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
        if (!bridge) return;
        QTimer::singleShot(0, bridge.data(), [] {
            if (!bridge || exiting) return;
            bridge->refresh();
            auto *config = obs_frontend_get_user_config();
            if (config_get_bool(config, "PenguinQuickPreview", "Placed")) {
                if (!config_get_int(config, "PenguinQuickPreview", "DesignLayout")) bridge->arrangeWorkspace();
                return;
            }
            auto *main = static_cast<QMainWindow *>(obs_frontend_get_main_window());
            if (auto *dock = main->findChild<QDockWidget *>(DockId)) {
                dock->setFloating(false);
                main->addDockWidget(Qt::LeftDockWidgetArea, dock, Qt::Horizontal);
                dock->show();
                main->resizeDocks({dock}, {240}, Qt::Horizontal);
                config_set_bool(config, "PenguinQuickPreview", "Placed", true);
                if (!config_get_int(config, "PenguinQuickPreview", "DesignLayout")) bridge->arrangeWorkspace();
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
}
void obs_module_unload(void)
{
    if (exiting) return; // Frontend callbacks are gone after EXIT.
    obs_frontend_remove_event_callback(event, nullptr);
    if (bridge) obs_frontend_remove_dock(DockId);
}
#include "penguin-quick-workspace.moc"
