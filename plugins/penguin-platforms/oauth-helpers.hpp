// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QList>
#include <QPair>
#include <QRandomGenerator>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

namespace PenguinOAuth {

inline QString RandomToken()
{
	QByteArray bytes(32, '\0');
	for (qsizetype i = 0; i < bytes.size(); ++i)
		bytes[i] = char(QRandomGenerator::system()->generate() & 0xff);
	return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

inline QByteArray Form(const QList<QPair<QString, QString>> &values)
{
	// OAuth form bodies must encode literal '+' as %2B, distinct from spaces.
	QByteArray result;
	for (const auto &value : values) {
		if (!result.isEmpty())
			result += '&';
		result += QUrl::toPercentEncoding(value.first) + '=' + QUrl::toPercentEncoding(value.second);
	}
	return result;
}

inline QByteArray Challenge(const QString &verifier)
{
	return QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256)
		.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

enum class CallbackStatus { Invalid, Denied, Accepted };

struct CallbackResult {
	CallbackStatus status = CallbackStatus::Invalid;
	QString code;
};

inline CallbackResult ParseCallback(const QByteArray &requestLine, const QString &expectedState)
{
	const auto parts = requestLine.trimmed().split(' ');
	if (parts.size() != 3 || parts[0] != "GET" ||
	    (parts[2] != "HTTP/1.1" && parts[2] != "HTTP/1.0") || expectedState.isEmpty())
		return {};

	const QUrl url = QUrl::fromEncoded(parts[1], QUrl::StrictMode);
	if (!url.isValid() || !url.isRelative() || !url.authority().isEmpty() || url.hasFragment() ||
	    url.path() != "/oauth/callback")
		return {};

	const QUrlQuery query(url);
	const auto states = query.allQueryItemValues("state", QUrl::FullyDecoded);
	const auto codes = query.allQueryItemValues("code", QUrl::FullyDecoded);
	if (states.size() != 1 || states.front() != expectedState || codes.size() > 1)
		return {};
	if (query.hasQueryItem("error") || codes.isEmpty() || codes.front().isEmpty())
		return {CallbackStatus::Denied, {}};
	return {CallbackStatus::Accepted, codes.front()};
}

} // namespace PenguinOAuth
