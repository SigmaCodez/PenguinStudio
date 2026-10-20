// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QVersionNumber>

namespace PenguinUpdates {
struct Release {
	QString version, name, digest;
	QUrl url, notes;
	qint64 size = 0;
	bool preview = false;
};
inline QString PackageName(const QString &version, const QString &target)
{
	if (target == "ubuntu24.04" || target == "ubuntu26.04") {
		return "penguin-studio_" + version + "-1+" + target + "_amd64.deb";
	}
	if (target == "fedora44") {
		return "penguin-studio-" + version + "-1.x86_64.rpm";
	}
	if (target == "flatpak-user" || target == "flatpak-system") {
		return "penguin-studio-" + version + "-x86_64.flatpak";
	}
	return {};
}
inline Release Select(const QJsonArray &releases, const QString &current, const QString &target, bool previews)
{
	Release selected;
	const QRegularExpression tag("^penguin-v([0-9]+\\.[0-9]+\\.[0-9]+)$");
	const QRegularExpression hash("^sha256:[a-fA-F0-9]{64}$");
	for (const auto &entry : releases) {
		const auto release = entry.toObject();
		const auto match = tag.match(release.value("tag_name").toString());
		if (!match.hasMatch() || release.value("draft").toBool() ||
		    (release.value("prerelease").toBool() && !previews)) {
			continue;
		}
		const auto version = match.captured(1);
		if (QVersionNumber::fromString(version) <= QVersionNumber::fromString(current) ||
		    (!selected.version.isEmpty() &&
		     QVersionNumber::fromString(version) <= QVersionNumber::fromString(selected.version))) {
			continue;
		}
		Release candidate;
		candidate.version = version;
		candidate.preview = release.value("prerelease").toBool();
		candidate.notes = QUrl("https://github.com/SigmaCodez/PenguinStudio/releases/tag/penguin-v" + version);
		const auto expectedName = PackageName(version, target);
		for (const auto &value : release.value("assets").toArray()) {
			const auto asset = value.toObject();
			if (expectedName.isEmpty() || asset.value("name").toString() != expectedName ||
			    asset.value("state").toString() != "uploaded") {
				continue;
			}
			const auto digest = asset.value("digest").toString();
			const auto size = asset.value("size").toDouble();
			const QUrl url(asset.value("browser_download_url").toString());
			const QUrl expected("https://github.com/SigmaCodez/PenguinStudio/releases/download/penguin-v" +
					    version + "/" + expectedName);
			if (!hash.match(digest).hasMatch() || size <= 0 || size > 1024.0 * 1024 * 1024 ||
			    url != expected) {
				continue;
			}
			candidate.name = expectedName;
			candidate.digest = digest.mid(7).toLower();
			candidate.size = qint64(size);
			candidate.url = url;
		}
		selected = candidate;
	}
	return selected;
}
} // namespace PenguinUpdates
