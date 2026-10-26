// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <mutex>

// Linux driver counters are sampled off the UI/render threads. A single pending
// read owns shared state, not a widget; destroying the status bar is safe.
class PenguinGPUUsage {
public:
	struct Sample {
		int percent = -1;
		QString renderer;
		qint64 timestamp = 0;
	};
	static int ReadPercent(const QString &filename)
	{
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly))
			return -1;
		const QByteArray text = file.read(32).trimmed();
		if (text.isEmpty() || text.size() > 3)
			return -1;
		for (const char character : text)
			if (character < '0' || character > '9')
				return -1;
		bool valid = false;
		const int value = text.toInt(&valid);
		return valid && value <= 100 ? value : -1;
	}

private:
	struct State {
		std::mutex mutex;
		std::atomic<bool> pending{false};
		QString path;
		Sample sample;
		uint64_t generation = 0;
	};
	std::shared_ptr<State> state = std::make_shared<State>();
	const QString sysfsRoot;

public:
	explicit PenguinGPUUsage(QString root = "/sys/class/drm") : sysfsRoot(std::move(root)) {}
	void Configure(const QString &renderNode, const QString &renderer)
	{
		std::lock_guard lock(state->mutex);
		++state->generation;
		state->sample = {-1, renderer, 0};
		state->path.clear();
#ifdef __linux__
		// OBS's Wayland backend identifies the selected adapter by its render node.
		const QString name = QFileInfo(renderNode).fileName();
		bool number = false;
		name.mid(7).toUInt(&number);
		if (renderNode.startsWith("/dev/dri/") && name.startsWith("renderD") && number)
			state->path = sysfsRoot + "/" + name + "/device/gpu_busy_percent";
#endif
	}
	Sample Poll()
	{
		Sample result;
		QString path;
		uint64_t generation;
		{
			std::lock_guard lock(state->mutex);
			result = state->sample;
			path = state->path;
			generation = state->generation;
		}
		if (QDateTime::currentMSecsSinceEpoch() - result.timestamp > 6000)
			result.percent = -1;
		if (!path.isEmpty() && !state->pending.exchange(true)) {
			QThreadPool::globalInstance()->start([shared = state, path, generation] {
				const int percent = ReadPercent(path);
				{
					std::lock_guard lock(shared->mutex);
					if (shared->generation == generation) {
						shared->sample.percent = percent;
						shared->sample.timestamp = QDateTime::currentMSecsSinceEpoch();
					}
				}
				shared->pending = false;
			});
		}
		return result;
	}
};
