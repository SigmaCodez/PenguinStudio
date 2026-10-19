// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QLoggingCategory>
#include <QStringView>

// Qt 6.10.2 reports a destroyed wl_surface proxy as a focus mismatch when the
// compositor's queued leave event arrives with a null surface. Qt upstream
// identifies this as normal teardown in QTBUG-120533:
// https://github.com/qt/qtbase/commit/ac8b2e9b40bf19aa889345f7b617392bef906040
// Keep the original message at debug level; do not hide real focus mismatches.
inline bool IsBenignQtWaylandTeardownWarning(QtMsgType type, const QMessageLogContext &context,
					    QStringView message, const char *runtimeQtVersion,
					    QStringView platformName)
{
	if (type != QtWarningMsg || !context.category || qstrcmp(context.category, "qt.qpa.wayland.textinput") != 0 ||
	    !runtimeQtVersion || qstrcmp(runtimeQtVersion, "6.10.2") != 0 ||
	    (platformName != QLatin1StringView("wayland") && platformName != QLatin1StringView("wayland-egl")))
		return false;

	constexpr QLatin1StringView leavePrefix(
		"virtual void QtWaylandClient::QWaylandTextInputv3::zwp_text_input_v3_leave(wl_surface*) "
		"Got leave event for surface 0x0 with focusing surface 0x");
	constexpr QLatin1StringView disablePrefix(
		"virtual void QtWaylandClient::QWaylandTextInputv3::disableSurface(wl_surface*) "
		"Try to disable surface 0x0 with focusing surface 0x");

	QStringView pointer;
	if (message.startsWith(leavePrefix))
		pointer = message.sliced(leavePrefix.size());
	else if (message.startsWith(disablePrefix))
		pointer = message.sliced(disablePrefix.size());
	else
		return false;

	if (pointer.isEmpty() || pointer.size() > qsizetype(sizeof(quintptr) * 2))
		return false;

	bool nonzero = false;
	for (QChar digit : pointer) {
		const auto value = digit.unicode();
		if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f')))
			return false;
		nonzero |= value != '0';
	}
	return nonzero;
}
