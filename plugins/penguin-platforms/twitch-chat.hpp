// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebSocket>
#include <functional>
#include <algorithm>
#include <iterator>

// A native chat client: no embedded browser, X11 window or browser cookies.
class TwitchChat final : public QWidget {
	struct Message {
		QString id, user, text;
	};
	QNetworkAccessManager network{this};
	QPointer<QWebSocket> socket;
	QTimer retry{this}, heartbeat{this};
	QPlainTextEdit *history = new QPlainTextEdit(this);
	QLineEdit *input = new QLineEdit(this);
	QPushButton *send = new QPushButton(this);
	QLabel *status = new QLabel(this);
	QList<Message> messages;
	QString token, client, user, session;
	quint64 generation = 0;
	int backoff = 1000, keepalive = 30;
	bool ready = false, sending = false;
	std::function<QString(const char *)> text;

	void Render()
	{
		QStringList lines;
		for (const auto &message : messages) {
			lines.push_back(message.text);
		}
		history->setPlainText(lines.join('\n'));
		history->moveCursor(QTextCursor::End);
	}

	void Request(const QString &path, const QJsonObject &body, std::function<void(int, const QJsonObject &)> done)
	{
		QNetworkRequest request{QUrl("https://api.twitch.tv/helix/" + path)};
		request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
		request.setRawHeader("Client-Id", client.toUtf8());
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
		request.setTransferTimeout(20000);
		auto *reply = network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
		const auto epoch = generation;
		connect(reply, &QNetworkReply::readyRead, this, [reply]() {
			if (reply->bytesAvailable() > 256 * 1024) {
				reply->abort();
			}
		});
		connect(reply, &QNetworkReply::finished, this, [this, reply, epoch, done]() {
			const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			const auto data = QJsonDocument::fromJson(reply->readAll()).object();
			reply->deleteLater();
			if (epoch != generation) {
				return;
			}
			if (code == 401 || code == 403) {
				Stop();
				status->setText(text("Chat.Authorization"));
				return;
			}
			done(code, data);
		});
	}

	void Retry()
	{
		if (token.isEmpty() || retry.isActive()) {
			return;
		}
		ready = false;
		send->setEnabled(false);
		heartbeat.stop();
		status->setText(text("Chat.Reconnecting"));
		retry.start(backoff);
		backoff = std::min(backoff * 2, 60000);
	}

	void Subscribe(int index = 0)
	{
		static const QStringList types{"channel.chat.message", "channel.chat.message_delete",
					       "channel.chat.clear", "channel.chat.clear_user_messages"};
		if (index == types.size()) {
			ready = true;
			backoff = 1000;
			send->setEnabled(!sending);
			status->setText(text("Chat.Connected"));
			return;
		}
		const auto expectedSession = session;
		Request("eventsub/subscriptions",
			{{"type", types[index]},
			 {"version", "1"},
			 {"condition", QJsonObject{{"broadcaster_user_id", user}, {"user_id", user}}},
			 {"transport", QJsonObject{{"method", "websocket"}, {"session_id", session}}}},
			[this, index, expectedSession](int code, const QJsonObject &) {
				if (session != expectedSession) {
					return;
				}
				if (code == 202) {
					Subscribe(index + 1);
				} else {
					Retry();
				}
			});
	}

	void Open(const QUrl &url = QUrl("wss://eventsub.wss.twitch.tv/ws"), bool migration = false)
	{
		if (token.isEmpty()) {
			return;
		}
		// Reconnect URLs come from Twitch, but still must stay on its TLS endpoint.
		if (!IsEndpoint(url)) {
			Retry();
			return;
		}
		auto *next = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
		next->setMaxAllowedIncomingFrameSize(256 * 1024);
		next->setMaxAllowedIncomingMessageSize(256 * 1024);
		const auto epoch = generation;
		if (!migration && socket) {
			disconnect(socket, nullptr, this, nullptr);
			socket->abort();
			socket->deleteLater();
		}
		if (!migration) {
			socket = next;
		}
		QTimer::singleShot(15000, next, [next]() {
			if (!next->property("welcomed").toBool()) {
				next->abort();
			}
		});
		connect(next, &QWebSocket::disconnected, this, [this, next, epoch]() {
			if (epoch == generation) {
				Retry();
			}
			next->deleteLater();
		});
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
		connect(next, &QWebSocket::errorOccurred, this,
#else
		connect(next, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
#endif
			[this, epoch](QAbstractSocket::SocketError) {
				if (epoch == generation)
					Retry();
			});
		connect(next, &QWebSocket::textMessageReceived, this,
			[this, next, epoch, migration](const QString &message) {
				if (epoch != generation) {
					return;
				}
				const auto root = QJsonDocument::fromJson(message.toUtf8()).object();
				const auto type = root.value("metadata").toObject().value("message_type").toString();
				const auto payload = root.value("payload").toObject();
				if (type == "session_welcome") {
					const auto info = payload.value("session").toObject();
					if (info.value("id").toString().isEmpty()) {
						Retry();
						return;
					}
					next->setProperty("welcomed", true);
					if (migration && socket && socket != next) {
						disconnect(socket, nullptr, this, nullptr);
						socket->abort();
						socket->deleteLater();
					}
					socket = next;
					session = info.value("id").toString();
					keepalive =
						std::clamp(info.value("keepalive_timeout_seconds").toInt(30), 10, 600);
					retry.stop();
					if (!migration) {
						Subscribe();
					}
				} else if (type == "session_reconnect") {
					Open(QUrl(payload.value("session").toObject().value("reconnect_url").toString()),
					     true);
				} else if (type == "notification") {
					ApplyEvent(payload.value("subscription").toObject().value("type").toString(),
						   payload.value("event").toObject());
				} else if (type == "revocation") {
					Stop();
					status->setText(text("Chat.Authorization"));
					return;
				}
				heartbeat.start((keepalive + 5) * 1000);
			});
		next->open(url);
	}

public:
	static bool IsEndpoint(const QUrl &url)
	{
		return url.isValid() && url.scheme() == "wss" && url.host() == "eventsub.wss.twitch.tv" &&
		       url.userInfo().isEmpty() && (url.port() == -1 || url.port() == 443) && url.path() == "/ws" &&
		       !url.hasFragment();
	}

	TwitchChat(QWidget *parent, std::function<QString(const char *)> translate)
		: QWidget(parent),
		  text(std::move(translate))
	{
		setObjectName("penguinTwitchChat");
		auto *layout = new QVBoxLayout(this);
		history->setReadOnly(true);
		history->setObjectName("penguinChatHistory");
		history->document()->setMaximumBlockCount(500);
		input->setPlaceholderText(text("Chat.Message"));
		input->setMaxLength(1000);
		send->setText(text("Chat.Send"));
		status->setWordWrap(true);
		status->setTextFormat(Qt::PlainText);
		layout->addWidget(status);
		layout->addWidget(history, 1);
		layout->addWidget(input);
		layout->addWidget(send);
		retry.setSingleShot(true);
		heartbeat.setSingleShot(true);
		connect(&retry, &QTimer::timeout, this, [this]() { Open(); });
		connect(&heartbeat, &QTimer::timeout, this, [this]() { Retry(); });
		connect(send, &QPushButton::clicked, this, [this]() {
			const auto message = input->text().trimmed();
			if (!ready || sending || message.isEmpty() || message.toUcs4().size() > 500) {
				return;
			}
			sending = true;
			send->setEnabled(false);
			Request("chat/messages", {{"broadcaster_id", user}, {"sender_id", user}, {"message", message}},
				[this, message](int code, const QJsonObject &data) {
					sending = false;
					const auto result = data.value("data").toArray();
					const bool sent = code == 200 && !result.isEmpty() &&
							  result.first().toObject().value("is_sent").toBool();
					if (sent && input->text().trimmed() == message) {
						input->clear();
					}
					status->setText(text(sent ? "Chat.Connected" : "Chat.SendFailed"));
					QTimer::singleShot(code == 429 ? 30000 : 1500, this,
							   [this]() { send->setEnabled(ready && !sending); });
				});
		});
		connect(input, &QLineEdit::returnPressed, send, &QPushButton::click);
		Stop();
	}

	void SetSession(const QString &id, const QString &access, const QString &account)
	{
		if (client == id && user == account && !token.isEmpty()) {
			token = access;
			return;
		}
		Stop();
		client = id;
		token = access;
		user = account;
		status->setText(text("Chat.Reconnecting"));
		Open();
	}

	void Stop()
	{
		++generation;
		retry.stop();
		heartbeat.stop();
		for (auto *ws : findChildren<QWebSocket *>()) {
			disconnect(ws, nullptr, this, nullptr);
			ws->abort();
			ws->deleteLater();
		}
		for (auto *reply : network.findChildren<QNetworkReply *>()) {
			reply->abort();
		}
		socket = nullptr;
		token.clear();
		client.clear();
		user.clear();
		session.clear();
		messages.clear();
		history->clear();
		input->clear();
		ready = sending = false;
		send->setEnabled(false);
		status->setText(text("Chat.Disconnected"));
	}

	~TwitchChat() override { Stop(); }

	// Event payloads are untrusted text. Never render them as HTML or follow URLs.
	void ApplyEvent(const QString &type, const QJsonObject &event)
	{
		if (type == "channel.chat.message") {
			const auto id = event.value("message_id").toString();
			if (id.isEmpty()) {
				return;
			}
			for (const auto &message : messages) {
				if (message.id == id) {
					return;
				}
			}
			messages.push_back(
				{id, event.value("chatter_user_id").toString(),
				 event.value("chatter_user_name").toString().left(100) + ": " +
					 event.value("message").toObject().value("text").toString().left(2000)});
			if (messages.size() > 500) {
				messages.removeFirst();
			}
		} else if (type == "channel.chat.clear") {
			messages.clear();
		} else if (type == "channel.chat.message_delete" || type == "channel.chat.clear_user_messages") {
			for (auto it = messages.begin(); it != messages.end();) {
				const bool remove = type == "channel.chat.message_delete"
							    ? it->id == event.value("message_id").toString()
							    : it->user == event.value("target_user_id").toString();
				it = remove ? messages.erase(it) : std::next(it);
			}
		}
		Render();
	}
};
