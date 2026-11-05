// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QString>
#include <functional>

// In-process, lifetime-bound bridge between the frontend wizard and account module.
// Stream credentials are delivered directly, never stored on the global action.
class PenguinAccountSetupRequest final : public QObject {
public:
    using QObject::QObject;
    QString provider;
    std::function<void(const QString &, const QString &, const QString &)> complete;
};
