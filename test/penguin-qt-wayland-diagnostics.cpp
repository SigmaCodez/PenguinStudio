// SPDX-License-Identifier: GPL-2.0-or-later
#include "../frontend/utility/QtWaylandDiagnostics.hpp"

#include <QString>

#include <cstdio>
#include <cstdlib>

namespace {
int checks = 0;

void Require(bool condition, const char *description)
{
	++checks;
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s\n", description);
		std::exit(EXIT_FAILURE);
	}
}
} // namespace

int main()
{
	const QMessageLogContext context("qt-source.cpp", 1, "qtFunction", "qt.qpa.wayland.textinput");
	const QString leave = QStringLiteral(
		"virtual void QtWaylandClient::QWaylandTextInputv3::zwp_text_input_v3_leave(wl_surface*) "
		"Got leave event for surface 0x0 with focusing surface 0x1234ab");
	const QString disable = QStringLiteral(
		"virtual void QtWaylandClient::QWaylandTextInputv3::disableSurface(wl_surface*) "
		"Try to disable surface 0x0 with focusing surface 0x1234ab");
	Require(IsBenignQtWaylandTeardownWarning(QtWarningMsg, context, leave, "6.11.1",
						QStringLiteral("wayland")),
		"Qt 6.11.1 retains the confirmed null-surface leave diagnostic");
	Require(!IsBenignQtWaylandTeardownWarning(QtWarningMsg, context, disable, "6.11.1",
						 QStringLiteral("wayland")),
		"Qt 6.11.1 no longer has the separate disableSurface warning");
	Require(!IsBenignQtWaylandTeardownWarning(QtWarningMsg, context,
						QString(leave).replace("surface 0x0", "surface 0x5678"), "6.11.1",
						QStringLiteral("wayland")),
		"Qt 6.11.1 real surface mismatches remain warnings");
	const char *version = "6.10.2";
	const QString platform = QStringLiteral("wayland");
	const auto matches = [&](const QString &message) {
		return IsBenignQtWaylandTeardownWarning(QtWarningMsg, context, message, version, platform);
	};

	for (const auto &message : {leave, disable}) {
		Require(matches(message), "exact null-surface diagnostic is benign");
		Require(IsBenignQtWaylandTeardownWarning(QtWarningMsg, context, message, version,
						       QStringLiteral("wayland-egl")),
			"Wayland EGL uses the same diagnostic");
		for (QtMsgType severity : {QtDebugMsg, QtInfoMsg, QtCriticalMsg, QtFatalMsg})
			Require(!IsBenignQtWaylandTeardownWarning(severity, context, message, version, platform),
				"other severities must retain their original handling");
		for (const auto &otherVersion : {"6.10.1", "6.10.3", "6.11.0", "6.11.2", "6.11.1.1", "6.9.2", "6.10.2.1", ""})
			Require(!IsBenignQtWaylandTeardownWarning(QtWarningMsg, context, message, otherVersion, platform),
				"other runtime versions must retain their original handling");
		Require(!IsBenignQtWaylandTeardownWarning(QtWarningMsg, context, message, nullptr, platform),
			"missing runtime version must retain its original handling");
		for (const auto &otherPlatform : {"xcb", "offscreen", "windows", "cocoa", "wayland-extra", ""})
			Require(!IsBenignQtWaylandTeardownWarning(QtWarningMsg, context, message, version,
								QString::fromLatin1(otherPlatform)),
				"other platforms must retain their original handling");
		for (const char *category : {"default", "qt.qpa.wayland", "qt.qpa.wayland.textinput.extra", ""}) {
			const QMessageLogContext otherContext("", 0, "", category);
			Require(!IsBenignQtWaylandTeardownWarning(QtWarningMsg, otherContext, message, version, platform),
				"other categories must retain their original handling");
		}
		QMessageLogContext nullCategory;
		nullCategory.category = nullptr;
		Require(!IsBenignQtWaylandTeardownWarning(QtWarningMsg, nullCategory, message, version, platform),
			"missing category must retain its original handling");

		QString changed = message;
		changed.replace(QStringLiteral("surface 0x0 with"), QStringLiteral("surface 0x1234 with"));
		Require(!matches(changed), "real non-null focus mismatches remain warnings");
		changed = message;
		changed.replace(QStringLiteral("surface 0x0 with"), QStringLiteral("surface 0x01 with"));
		Require(!matches(changed), "non-null surface with leading zero remains a warning");
		changed = message;
		changed.replace(QStringLiteral("surface 0x0 with"), QStringLiteral("surface 0x00 with"));
		Require(!matches(changed), "near-match null surface formatting remains a warning");
		Require(!matches(QStringLiteral("prefix ") + message), "extra prefix remains a warning");
		Require(!matches(message + QStringLiteral(" suffix")), "extra suffix remains a warning");
		Require(!matches(message + QLatin1Char('\n')), "trailing newline remains a warning");
		changed = message;
		changed.replace(QStringLiteral("QWaylandTextInputv3"), QStringLiteral("QWaylandTextInputv2"));
		Require(!matches(changed), "other text-input implementations remain warnings");

		const QString prefix = message.left(message.lastIndexOf(QStringLiteral("0x")) + 2);
		for (const auto &pointer : {"", "0", "0000", "xyz", "-1", "1g", " 1", "1 ", "ABC"})
			Require(!matches(prefix + QString::fromLatin1(pointer)),
				"invalid or null cached pointers remain warnings");
		Require(!matches(prefix + QString(qsizetype(sizeof(quintptr) * 2 + 1), QLatin1Char('f'))),
			"oversized cached pointer remains a warning");
		for (const auto &pointer : {"1", "a", "00ff"})
			Require(matches(prefix + QString::fromLatin1(pointer)), "non-null hexadecimal cached pointer matches");
		Require(matches(prefix + QString(qsizetype(sizeof(quintptr) * 2), QLatin1Char('f'))),
			"maximum pointer width matches");
	}
	Require(!matches(QString()), "empty messages remain unchanged");
	Require(!matches(QStringLiteral("Got leave event for surface 0x0 with focusing surface 0x1")),
		"missing function signature remains unchanged");
	std::printf("PASS: %d Qt Wayland diagnostic classification checks\n", checks);
}
