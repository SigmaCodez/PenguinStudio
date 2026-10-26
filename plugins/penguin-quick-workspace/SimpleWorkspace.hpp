// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QAction>
#include <QBoxLayout>
#include <QMainWindow>
#include <QDockWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QToolButton>
#include <QWidget>
#include <vector>

// Presentation-only adapter. Native controls continue to own output state.
// Reparent redundant shortcuts into a hidden holder: native refresh timers may
// still change their visibility, without bringing duplicate buttons back onscreen.
class SimpleWorkspace final : public QObject {
    struct Shortcut {
        QPointer<QWidget> widget, parent;
        QPointer<QBoxLayout> layout;
        int index;
        bool hidden;
    };
    QPointer<QMainWindow> main;
    QPointer<QWidget> holder;
    std::vector<Shortcut> shortcuts;
    QPointer<QToolButton> live, more, settings;
    QPointer<QMenu> liveMenu;
    QList<QPointer<QAction>> additions;
    QList<QPointer<QAction>> concealed;
    QString moreText, moreAccessible, moreTooltip;
    struct Caption { QPointer<QLabel> label; QString text; };
    std::vector<Caption> captions;
    QPointer<QLineEdit> sourceSearch;
    QString searchPlaceholder, searchAccessible;
    Qt::ToolButtonStyle settingsStyle = Qt::ToolButtonIconOnly;
    bool active = false;
public:
    explicit SimpleWorkspace(QMainWindow *window, QObject *parent) : QObject(parent), main(window) {}
    void setActive(bool enabled)
    {
        if (active == enabled || !main) return;
        active = enabled;
        main->setProperty("penguinSimpleWorkspace", enabled);
        if (!enabled) { restore(); return; }
        for (const auto &entry : {qMakePair("sourcesDock", "Sources"), qMakePair("penguinInspectorDock", "Source settings")}) {
            if (auto *dock = main->findChild<QDockWidget *>(entry.first))
                if (auto *label = dock->findChild<QLabel *>("penguinPanelTitle")) {
                    captions.push_back({label, label->text()});
                    label->setText(QString::fromLatin1(entry.second));
                }
        }
        sourceSearch = main->findChild<QLineEdit *>("penguinLayerSearch");
        if (sourceSearch) {
            searchPlaceholder = sourceSearch->placeholderText(); searchAccessible = sourceSearch->accessibleName();
            sourceSearch->setPlaceholderText("Search sources"); sourceSearch->setAccessibleName("Search sources");
        }
        holder = new QWidget(main);
        holder->hide();
        for (const char *name : {"penguinDestinationsButton", "penguinDetailsButton", "penguinInspectorButton",
                                 "penguinClipButton", "penguinCanvasMode"}) {
            auto *widget = main->findChild<QWidget *>(name);
            auto *parent = widget ? widget->parentWidget() : nullptr;
            auto *layout = parent ? qobject_cast<QBoxLayout *>(parent->layout()) : nullptr;
            if (!widget || !layout) continue;
            shortcuts.push_back({widget, parent, layout, layout->indexOf(widget), widget->isHidden()});
            widget->setParent(holder);
        }
        settings = main->findChild<QToolButton *>("penguinSettingsButton");
        if (settings) { settingsStyle = settings->toolButtonStyle(); settings->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); }
        live = main->findChild<QToolButton *>("penguinLiveButton");
        more = main->findChild<QToolButton *>("penguinToolbarButton");
        if (more) {
            moreText = more->text(); moreAccessible = more->accessibleName(); moreTooltip = more->toolTip();
            more->setText("Advanced");
            more->setAccessibleName("Advanced tools");
            more->setToolTip("Replay buffer, virtual camera and studio mode");
            more->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        }
        auto *menu = more ? more->menu() : nullptr;
        if (menu) {
            for (auto *action : menu->actions()) {
                if (settings && action->text() == settings->text() && action->isVisible()) {
                    concealed.append(action);
                    action->setVisible(false);
                }
            }
            if (live && live->menu()) {
                liveMenu = live->menu();
                // The last action is the distinct advanced primary-stream control.
                const auto actions = liveMenu->actions();
                if (!actions.isEmpty()) { menu->addAction(actions.last()); additions.append(actions.last()); }
                live->setMenu(nullptr);
                live->setPopupMode(QToolButton::DelayedPopup);
            }
            auto addControl = [&](const char *name, const QString &title, bool checkable) {
                QPointer<QToolButton> original = main->findChild<QToolButton *>(name);
                if (!original) return;
                auto *action = new QAction(title, this);
                action->setObjectName(QString::fromLatin1(name) + "AdvancedAction");
                action->setCheckable(checkable);
                connect(action, &QAction::triggered, this, [original] {
                    if (original && original->isEnabled()) original->click();
                });
                connect(menu, &QMenu::aboutToShow, action, [original, action] {
                    action->setEnabled(original && original->isEnabled());
                    if (original && action->isCheckable()) action->setChecked(original->isChecked());
                });
                menu->addAction(action);
                additions.append(action);
            };
            addControl("penguinClipButton", "Save replay clip", false);
            addControl("penguinCanvasMode", "Studio mode (preview and program)", true);
        }
    }
    void restore()
    {
        if (!main) return;
        for (const auto &caption : captions) if (caption.label) caption.label->setText(caption.text);
        captions.clear();
        if (sourceSearch) { sourceSearch->setPlaceholderText(searchPlaceholder); sourceSearch->setAccessibleName(searchAccessible); }
        if (live && liveMenu) { live->setMenu(liveMenu); live->setPopupMode(QToolButton::MenuButtonPopup); }
        if (more && more->menu()) for (auto &action : additions) if (action) {
            more->menu()->removeAction(action);
            if (action->parent() == this) delete action;
        }
        additions.clear();
        for (auto &action : concealed) if (action) action->setVisible(true);
        concealed.clear();
        if (more) { more->setToolButtonStyle(Qt::ToolButtonIconOnly); more->setText(moreText); more->setAccessibleName(moreAccessible); more->setToolTip(moreTooltip); }
        if (settings) settings->setToolButtonStyle(settingsStyle);
        // Each index was captured after prior removals, so restore in reverse order.
        for (auto it = shortcuts.rbegin(); it != shortcuts.rend(); ++it) {
            if (!it->widget || !it->parent || !it->layout) continue;
            it->widget->setParent(it->parent);
            it->layout->insertWidget(it->index, it->widget);
            it->widget->setVisible(!it->hidden);
        }
        shortcuts.clear();
        delete holder.data();
    }
};
