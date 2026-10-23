// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat-view.hpp"
#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalBlocker>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QQueue>
#include <algorithm>

class YoutubeChat final : public QWidget {
	friend struct PenguinChatTest;
	QNetworkAccessManager ownedNetwork{this};
	QNetworkAccessManager &network;
	QTimer poll{this}, audienceTimer{this};
	QLabel *audience = new QLabel(this);
	QMap<QString, QString> videos;
	QComboBox *broadcasts = new QComboBox(this);
	QListWidget *history = new QListWidget(this);
	QLabel *status = new QLabel(this);
	QLineEdit *input = new QLineEdit(this);
	QPushButton *send = new QPushButton("Send", this);
	QPushButton *refresh = new QPushButton("Load live broadcasts", this);
	QString token, account, chat, page;
	quint64 generation = 0;
	bool sending = false, moderating = false;
	QSet<QString> seen, loadPages;
	QQueue<QString> seenOrder;
	int failures = 0;
	QMap<QString, QPair<QString, QDateTime>> active;
	QMap<QString, QString> bans;
	static QString Encode(const QString &value) { return QString::fromLatin1(QUrl::toPercentEncoding(value)); }
	void Request(const QString &path, const QByteArray &method, const QJsonObject &body,
		     std::function<void(int, const QJsonObject &)> done)
	{
		QNetworkRequest request(QUrl("https://www.googleapis.com/youtube/v3/" + path));
		request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
		request.setTransferTimeout(20000);
		auto *reply = network.sendCustomRequest(
			request, method,
			body.isEmpty() ? QByteArray() : QJsonDocument(body).toJson(QJsonDocument::Compact));
		const auto epoch = generation;
		connect(reply, &QNetworkReply::readyRead, this, [reply] {
			if (reply->bytesAvailable() > 2 * 1024 * 1024) {
				reply->abort();
			}
		});
		connect(reply, &QNetworkReply::finished, this, [this, reply, epoch, done] {
			const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			const auto data = QJsonDocument::fromJson(reply->readAll()).object();
			reply->deleteLater();
			if (epoch != generation) {
				return;
			}
			done(code, data);
		});
	}
	void LoadAudience()
	{
		const auto video = videos.value(chat);
		if (video.isEmpty() || token.isEmpty()) {
			return;
		}
		Request("videos?part=liveStreamingDetails&id=" + Encode(video), "GET", {},
			[this](int code, const QJsonObject &data) {
				const auto items = data.value("items").toArray();
				const auto count = items.isEmpty() ? QJsonValue()
								   : items.first()
									     .toObject()
									     .value("liveStreamingDetails")
									     .toObject()
									     .value("concurrentViewers");
				audience->setText(code == 200 && !count.isUndefined() && !count.isNull()
							  ? count.toVariant().toString() + " watching · YouTube"
							  : "Viewer count unavailable");
			});
	}
	void Read()
	{
		if (token.isEmpty() || chat.isEmpty()) {
			return;
		}
		Request("liveChat/messages?part=id,snippet,authorDetails&maxResults=200&liveChatId=" + Encode(chat) +
				"&pageToken=" + Encode(page),
			"GET", {}, [this](int code, const QJsonObject &data) {
				if (code != 200) {
					if (code == 401 || code == 403 || code == 404) {
						status->setText(
							"Chat unavailable: check sign-in, live status, permissions or YouTube API quota. Reload to retry.");
						send->setEnabled(false);
						return;
					}
					status->setText("Chat connection interrupted. Retrying…");
					poll.start(std::min(60000, 5000 * ++failures));
					return;
				}
				failures = 0;
				page = data.value("nextPageToken").toString();
				ApplyMessages(data.value("items").toArray());
				if (!data.value("offlineAt").toString().isEmpty()) {
					status->setText("This live chat has ended");
					send->setEnabled(false);
					return;
				}
				status->setText("Connected · YouTube live chat");
				send->setEnabled(!sending);
				// Respect YouTube's minimum interval. A 10s floor limits polling quota use.
				poll.start(std::max(10000, data.value("pollingIntervalMillis").toInt(10000)));
			});
	}
	void Select()
	{
		++generation;
		poll.stop();
		page.clear();
		history->clear();
		active.clear();
		bans.clear();
		seen.clear();
		seenOrder.clear();
		sending = moderating = false;
		chat = broadcasts->currentData().toString();
		send->setEnabled(false);
		audienceTimer.stop();
		audience->setText("YouTube · Viewer count updates every minute");
		if (!chat.isEmpty()) {
			Read();
			audienceTimer.start(60000);
		}
	}
	void Load(const QString &cursor = {}, QList<QPair<QString, QString>> entries = {})
	{
		if (token.isEmpty()) {
			return;
		}
		if (cursor.isEmpty()) {
			loadPages.clear();
			videos.clear();
		} else if (loadPages.contains(cursor)) {
			refresh->setEnabled(true);
			broadcasts->setEnabled(true);
			status->setText("Could not finish loading broadcasts. Try again.");
			return;
		}
		loadPages.insert(cursor);
		refresh->setEnabled(false);
		broadcasts->setEnabled(false);
		Request("liveBroadcasts?part=id,snippet&broadcastStatus=active&broadcastType=all&maxResults=50&pageToken=" +
				Encode(cursor),
			"GET", {}, [this, entries](int code, const QJsonObject &data) mutable {
				refresh->setEnabled(true);
				broadcasts->setEnabled(true);
				if (code != 200) {
					status->setText(
						"Cannot load live broadcasts. Check YouTube sign-in and API quota.");
					return;
				}
				for (const auto &value : data.value("items").toArray()) {
					const auto snippet = value.toObject().value("snippet").toObject();
					const auto id = snippet.value("liveChatId").toString();
					if (!id.isEmpty()) {
						entries.append({snippet.value("title").toString(), id});
						videos[id] = value.toObject().value("id").toString();
					}
				}
				const auto next = data.value("nextPageToken").toString();
				if (!next.isEmpty() && entries.size() < 500) {
					Load(next, entries);
					return;
				}
				{
					QSignalBlocker block(broadcasts);
					broadcasts->clear();
					for (const auto &entry : entries) {
						broadcasts->addItem(entry.first, entry.second);
					}
				}
				Select();
				if (entries.isEmpty()) {
					status->setText(
						"No active broadcast with live chat. Start a broadcast on YouTube, then reload.");
				}
			});
	}
	void Moderate(const QString &action, const QString &id, const QString &target)
	{
		if (token.isEmpty() || chat.isEmpty() || moderating) {
			return;
		}
		if (action == "unban" && !bans.contains(target)) {
			status->setText(
				"Only bans created in this chat session can be removed here. Use YouTube Studio for other bans.");
			return;
		}
		const auto epoch = generation;
		if (!PenguinChat::Confirm(this, action == "delete"    ? "Delete this message"
						: action == "timeout" ? "Timeout this user for 10 minutes"
						: action == "ban"     ? "Ban this user"
								      : "Remove this user's ban or timeout") ||
		    epoch != generation) {
			return;
		}
		moderating = true;
		QString path;
		QByteArray method;
		QJsonObject body;
		if (action == "delete") {
			path = "liveChat/messages?id=" + Encode(id);
			method = "DELETE";
		} else if (action == "unban") {
			path = "liveChat/bans?id=" + Encode(bans.value(target));
			method = "DELETE";
		} else {
			path = "liveChat/bans?part=snippet";
			method = "POST";
			QJsonObject snippet{{"liveChatId", chat},
					    {"type", action == "timeout" ? "temporary" : "permanent"},
					    {"bannedUserDetails", QJsonObject{{"channelId", target}}}};
			if (action == "timeout") {
				snippet["banDurationSeconds"] = 600;
			}
			body["snippet"] = snippet;
		}
		Request(path, method, body, [this, action, id, target](int code, const QJsonObject &data) {
			moderating = false;
			if (code < 200 || code >= 300) {
				status->setText("Moderation failed. YouTube requires owner or moderator permissions.");
				return;
			}
			if (action == "ban" || action == "timeout") {
				bans[target] = data.value("id").toString();
				Remove({}, target);
			}
			if (action == "unban") {
				bans.remove(target);
			}
			if (action == "delete") {
				Remove(id, {});
			}
			status->setText("Moderation action completed");
		});
	}
	void Remove(const QString &id, const QString &user)
	{
		for (int i = history->count() - 1; i >= 0; --i) {
			auto *item = history->item(i);
			if ((!id.isEmpty() && item->data(Qt::UserRole).toString() == id) ||
			    (!user.isEmpty() && item->data(Qt::UserRole + 1).toString() == user)) {
				delete history->takeItem(i);
			}
		}
	}

public:
	explicit YoutubeChat(QWidget *parent, QNetworkAccessManager *transport = nullptr)
		: QWidget(parent),
		  network(transport ? *transport : ownedNetwork)
	{
		setObjectName("penguinYouTubeChat");
		auto *layout = new QVBoxLayout(this);
		PenguinChat::Style(this, history);
		status->setTextFormat(Qt::PlainText);
		status->setWordWrap(true);
		broadcasts->setAccessibleName("Active YouTube broadcast");
		layout->addWidget(audience);
		layout->addWidget(broadcasts);
		auto *toolbar = new QHBoxLayout;
		auto *participants = new QPushButton("Recent chatters", this);
		toolbar->addWidget(refresh);
		toolbar->addWidget(participants);
		layout->addLayout(toolbar);
		layout->addWidget(status);
		layout->addWidget(history, 1);
		auto *hint = new QLabel("Right-click a message for moderation", this);
		hint->setWordWrap(true);
		layout->addWidget(hint);
		input->setPlaceholderText("Send a YouTube message…");
		input->setMaxLength(200);
		layout->addWidget(input);
		layout->addWidget(send);
		poll.setSingleShot(true);
		connect(&audienceTimer, &QTimer::timeout, this, [this] { LoadAudience(); });
		connect(&poll, &QTimer::timeout, this, [this] { Read(); });
		connect(refresh, &QPushButton::clicked, this, [this] { Load(); });
		connect(broadcasts, &QComboBox::currentIndexChanged, this, [this] { Select(); });
		connect(participants, &QPushButton::clicked, this, [this] {
			QStringList names;
			const auto cutoff = QDateTime::currentDateTimeUtc().addSecs(-900);
			for (auto it = active.begin(); it != active.end();) {
				if (it.value().second < cutoff) {
					it = active.erase(it);
				} else {
					names.append(it.value().first);
					++it;
				}
			}
			PenguinChat::Participants(
				this, "Recent YouTube chatters",
				"People observed posting in the last 15 minutes. This is not a viewer list or a complete presence list.",
				names);
		});
		connect(history, &QListWidget::customContextMenuRequested, this, [this](const QPoint &point) {
			PenguinChat::MessageMenu(this, history, point,
						 [this](QString action, QString id, QString user) {
							 Moderate(action, id, user);
						 });
		});
		connect(send, &QPushButton::clicked, this, [this] {
			const auto message = input->text().trimmed();
			if (chat.isEmpty() || token.isEmpty() || sending || message.isEmpty()) {
				return;
			}
			sending = true;
			send->setEnabled(false);
			Request("liveChat/messages?part=snippet", "POST",
				{{"snippet",
				  QJsonObject{{"liveChatId", chat},
					      {"type", "textMessageEvent"},
					      {"textMessageDetails", QJsonObject{{"messageText", message}}}}}},
				[this, message](int code, const QJsonObject &) {
					sending = false;
					send->setEnabled(true);
					if (code >= 200 && code < 300) {
						if (input->text().trimmed() == message) {
							input->clear();
						}
					} else {
						status->setText(
							"Message not sent. Check chat permissions, quota and rate limits.");
					}
				});
		});
		connect(input, &QLineEdit::returnPressed, send, &QPushButton::click);
		Stop();
	}
	void SetSession(const QString &access, const QString &identity)
	{
		if (!token.isEmpty() && account == identity) {
			token = access;
			return;
		}
		Stop();
		token = access;
		account = identity;
		refresh->setEnabled(true);
		Load();
	}
	void Stop()
	{
		++generation;
		poll.stop();
		audienceTimer.stop();
		audience->setText("YouTube · Not connected");
		videos.clear();
		token.clear();
		chat.clear();
		account.clear();
		page.clear();
		for (auto *reply : network.findChildren<QNetworkReply *>()) {
			reply->abort();
		}
		QSignalBlocker block(broadcasts);
		broadcasts->clear();
		history->clear();
		active.clear();
		bans.clear();
		seen.clear();
		seenOrder.clear();
		input->clear();
		sending = moderating = false;
		failures = 0;
		send->setEnabled(false);
		refresh->setEnabled(false);
		status->setText("Connect YouTube in Stream details to use live chat.");
	}
	~YoutubeChat() override { Stop(); }
	void ApplyMessages(const QJsonArray &items)
	{
		const bool follow = history->verticalScrollBar()->value() >=
				    history->verticalScrollBar()->maximum() - 4;
		for (const auto &value : items) {
			const auto item = value.toObject();
			const auto snippet = item.value("snippet").toObject();
			const auto type = snippet.value("type").toString();
			if (type == "messageDeletedEvent") {
				Remove(snippet.value("messageDeletedDetails")
					       .toObject()
					       .value("deletedMessageId")
					       .toString(),
				       {});
				continue;
			}
			if (type == "userBannedEvent") {
				Remove({}, snippet.value("userBannedDetails")
						   .toObject()
						   .value("bannedUserDetails")
						   .toObject()
						   .value("channelId")
						   .toString());
				continue;
			}
			const auto id = item.value("id").toString();
			if (id.isEmpty()) {
				continue;
			}
			bool exists = false;
			for (int i = 0; i < history->count(); ++i) {
				if (history->item(i)->data(Qt::UserRole).toString() == id) {
					exists = true;
					break;
				}
			}
			if (exists || seen.contains(id)) {
				continue;
			}
			seen.insert(id);
			seenOrder.enqueue(id);
			if (seenOrder.size() > 2048) {
				seen.remove(seenOrder.dequeue());
			}
			const auto author = item.value("authorDetails").toObject();
			const auto user = author.value("channelId").toString();
			const auto name = author.value("displayName").toString().left(100);
			QString badge = author.value("isChatOwner").toBool()       ? "Owner · "
					: author.value("isChatModerator").toBool() ? "Moderator · "
					: author.value("isChatSponsor").toBool()   ? "Member · "
										   : "";
			const auto text = snippet.value("displayMessage").toString().left(2000);
			if (text.isEmpty()) {
				continue;
			}
			auto *row = new QListWidgetItem(badge + name + ": " + text, history);
			row->setData(Qt::UserRole, id);
			row->setData(Qt::UserRole + 1, user);
			const auto published =
				QDateTime::fromString(snippet.value("publishedAt").toString(), Qt::ISODate);
			if (!user.isEmpty() && published.isValid()) {
				active[user] = {name, published};
			}
			while (history->count() > 500) {
				delete history->takeItem(0);
			}
		}
		// Bound presence memory even if the participant dialog is never opened.
		const auto cutoff = QDateTime::currentDateTimeUtc().addSecs(-900);
		for (auto it = active.begin(); it != active.end();) {
			it = it.value().second < cutoff ? active.erase(it) : std::next(it);
		}
		while (active.size() > 10000) {
			active.erase(active.begin());
		}
		if (follow) {
			history->scrollToBottom();
		}
	}
};
