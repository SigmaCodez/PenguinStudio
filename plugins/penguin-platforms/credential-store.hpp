// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <qt6keychain/keychain.h>
#include <QQueue>
#include <functional>

class CredentialStore : public QObject {
public:
	using Done = std::function<void(bool, const QByteArray &)>;
	using QObject::QObject;
	virtual void Read(Done) = 0;
	virtual void Write(const QByteArray &, Done) = 0;
	virtual void Remove(Done) = 0;
};
// Serialize operations so a delayed token write cannot undo a disconnect.
class KeychainStore final : public CredentialStore {
	struct Operation {
		int kind;
		QByteArray value;
		Done done;
	};
	QQueue<Operation> queue;
	QString key;
	bool busy = false;
	void Next()
	{
		if (busy || queue.isEmpty()) {
			return;
		}
		busy = true;
		auto op = queue.dequeue();
		QKeychain::Job *job = nullptr;
		if (op.kind == 0) {
			job = new QKeychain::ReadPasswordJob("PenguinStudio", this);
		} else if (op.kind == 1) {
			auto *w = new QKeychain::WritePasswordJob("PenguinStudio", this);
			w->setBinaryData(op.value);
			job = w;
		} else {
			job = new QKeychain::DeletePasswordJob("PenguinStudio", this);
		}
		job->setKey(key);
		job->setInsecureFallback(false);
		connect(job, &QKeychain::Job::finished, this, [this, op](QKeychain::Job *j) {
			QByteArray value;
			if (op.kind == 0 && j->error() == QKeychain::NoError) {
				value = static_cast<QKeychain::ReadPasswordJob *>(j)->binaryData();
			}
			const bool ok = j->error() == QKeychain::NoError || j->error() == QKeychain::EntryNotFound;
			busy = false;
			op.done(ok, value);
			Next();
		});
		job->start();
	}
	void Enqueue(int kind, const QByteArray &value, Done done)
	{
		queue.enqueue({kind, value, std::move(done)});
		Next();
	}

public:
	KeychainStore(QString key, QObject *parent) : CredentialStore(parent), key(std::move(key)) {}
	void Read(Done done) override { Enqueue(0, {}, std::move(done)); }
	void Write(const QByteArray &value, Done done) override { Enqueue(1, value, std::move(done)); }
	void Remove(Done done) override { Enqueue(2, {}, std::move(done)); }
};
