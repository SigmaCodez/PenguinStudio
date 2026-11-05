// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QPointer>
#include <QHash>
#include <QVector>
#include <QString>
#include <functional>

// Session-only streaming credentials shared by account UI and broadcast outputs.
// No credentials are exposed as QObject properties or written to destination JSON.
class PenguinStreamingAccess final : public QObject {
public:
 struct Access { QString server, key, label, status; quint64 revision = 0; };
 using Observer = std::function<void()>;
 explicit PenguinStreamingAccess(QObject *parent) : QObject(parent) { setObjectName("penguinStreamingAccess"); }
 static PenguinStreamingAccess *Get(QObject *parent) {
  if (!parent) return nullptr;
  if (auto *existing = parent->findChild<QObject *>("penguinStreamingAccess"))
   return dynamic_cast<PenguinStreamingAccess *>(existing);
  return new PenguinStreamingAccess(parent);
 }
 Access Read(const QString &provider) const { return values.value(provider); }
 quint64 Update(const QString &provider, Access access) {
  access.revision = ++serial;
  values.insert(provider, access);
  const auto listeners = observers;
  for (const auto &listener : listeners) if (listener.context) listener.notify();
  for (int i=observers.size()-1; i>=0; --i) if (!observers[i].context) observers.removeAt(i);
  return access.revision;
 }
 void Subscribe(QObject *context, Observer notify) { observers.append({context, std::move(notify)}); }
private:
 struct Listener { QPointer<QObject> context; Observer notify; };
 QHash<QString, Access> values;
 QVector<Listener> observers;
 quint64 serial = 0;
};
