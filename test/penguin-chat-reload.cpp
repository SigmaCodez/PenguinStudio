// SPDX-License-Identifier: GPL-2.0-or-later
// No website, account or network access: exercise the real chat menu routing.
#include "../plugins/penguin-platforms/web-chat.hpp"
#include <QApplication>
#include <cstdlib>
#include <iostream>
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    PenguinChat::WebsiteChat wrapper("youtube", new QWidget);
    int reloads = 0;
    wrapper.reloadIntegrated = [&] { ++reloads; };
    QAction *reload = nullptr, *mode = nullptr;
    for (auto *action : wrapper.findChildren<QAction *>()) {
        if (action->text() == "Reload chat") reload = action;
        if (action->text() == "Use integrated chat") mode = action;
    }
    if (!reload || !mode) return 1;
    reload->trigger();
    if (reloads != 0) return 2;
    mode->setChecked(true);
    reload->trigger();
    if (reloads != 1) return 3;
    mode->setChecked(false);
    reload->trigger();
    if (reloads != 1) return 4;
    mode->setChecked(true);
    reload->trigger();
    if (reloads != 2) return 5;
    std::cout << "Original/integrated chat reload routing passed\n";
}
