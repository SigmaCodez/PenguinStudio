// SPDX-License-Identifier: GPL-2.0-or-later
// In-memory transport only: these tests never connect accounts or moderate real users.
#include "../plugins/penguin-platforms/twitch-chat.hpp"
#include "../plugins/penguin-platforms/youtube-chat.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <cstdlib>
#include <cstring>
#include <iostream>

static void Require(bool ok, const char *message)
{
	if (!ok) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}
class Reply final : public QNetworkReply {
	QByteArray data;
	qint64 offset = 0;
	qint64 readData(char *target, qint64 count) override
	{
		count = std::min(count, data.size() - offset);
		if (count <= 0) {
			return -1;
		}
		std::memcpy(target, data.constData() + offset, size_t(count));
		offset += count;
		return count;
	}

public:
	Reply(QObject *parent, const QNetworkRequest &request) : QNetworkReply(parent)
	{
		setRequest(request);
		setUrl(request.url());
		open(QIODevice::ReadOnly);
	}
	qint64 bytesAvailable() const override { return data.size() - offset + QNetworkReply::bytesAvailable(); }
	void Finish(int code, QJsonObject body)
	{
		setAttribute(QNetworkRequest::HttpStatusCodeAttribute, code);
		data = QJsonDocument(body).toJson();
		setFinished(true);
		emit readyRead();
		emit finished();
	}
	void abort() override
	{
		if (!isFinished()) {
			Finish(0, {});
		}
	}
};
class Transport final : public QNetworkAccessManager {
public:
	struct Call {
		QNetworkRequest request;
		QJsonObject body;
		QPointer<Reply> reply;
	};
	QList<Call> calls;
	QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *body) override
	{
		auto *reply = new Reply(this, request);
		calls.append(
			{request, body ? QJsonDocument::fromJson(body->readAll()).object() : QJsonObject(), reply});
		return reply;
	}
	void Finish(int status, QJsonObject body) { calls.last().reply->Finish(status, body); }
};
static void Confirm()
{
	QTimer::singleShot(0, [] {
		for (auto *widget : QApplication::topLevelWidgets()) {
			if (auto *box = qobject_cast<QMessageBox *>(widget)) {
				box->button(QMessageBox::Yes)->click();
			}
		}
	});
}
struct PenguinChatTest {
	static void Run()
	{
		Transport twitchNetwork;
		TwitchChat twitch(nullptr, [](const char *key) { return QString(key); }, &twitchNetwork);
		twitch.token = "fixture";
		twitch.user = "owner";
		twitch.client = "fixture-app";
		Confirm();
		twitch.Moderate("timeout", "message", "viewer");
		Require(twitchNetwork.calls.last().request.url().path() == "/helix/moderation/bans",
			"wrong Twitch timeout endpoint");
		Require(twitchNetwork.calls.last().body.value("data").toObject().value("duration").toInt() == 600,
			"wrong timeout duration");
		twitchNetwork.Finish(200, {});
		Confirm();
		twitch.Moderate("delete", "message+id", "viewer");
		Require(twitchNetwork.calls.last().request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() ==
				"DELETE",
			"delete must use DELETE");
		Require(twitchNetwork.calls.last().request.url().toString().contains("message_id=message%2Bid"),
			"message id must be encoded");
		twitchNetwork.Finish(403, {});
		Require(!twitch.token.isEmpty() && !twitch.moderating,
			"moderation denial must not destroy chat session");
		twitch.LoadParticipants();
		twitchNetwork.Finish(200, {{"data", QJsonArray{QJsonObject{{"user_name", "Viewer"}}}},
					   {"total", 2},
					   {"pagination", QJsonObject{{"cursor", "next"}}}});
		Require(twitchNetwork.calls.last().request.url().query().contains("after=next"),
			"chatters pagination not followed");
		twitch.Stop();
		Require(twitch.history->count() == 0 && twitch.token.isEmpty(), "Twitch stop must erase session");

		Transport youtubeNetwork;
		YoutubeChat youtube(nullptr, &youtubeNetwork);
		youtube.SetSession("fixture-token", "fixture-account");
		Require(youtubeNetwork.calls.size() == 1, "YouTube sign-in should load active broadcasts");
		youtubeNetwork.Finish(
			200, {{"items", QJsonArray{QJsonObject{{"snippet", QJsonObject{{"title", "Fixture live"},
										       {"liveChatId", "chat-id"}}}}}}});
		Require(youtubeNetwork.calls.last().request.url().path() == "/youtube/v3/liveChat/messages",
			"must load selected live chat");
		QJsonObject message{{"id", "one"},
				    {"authorDetails", QJsonObject{{"displayName", "Alice"}, {"channelId", "viewer"}}},
				    {"snippet", QJsonObject{{"type", "textMessageEvent"},
							    {"displayMessage", "<img src='https://invalid/'>"},
							    {"publishedAt",
							     QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}}}};
		youtubeNetwork.Finish(200, {{"items", QJsonArray{message, message}},
					    {"nextPageToken", "next-page"},
					    {"pollingIntervalMillis", 45000}});
		Require(youtube.history->count() == 1 && youtube.history->item(0)->text().contains("<img"),
			"YouTube messages must be plain text and deduplicated");
		Require(youtube.poll.interval() >= 45000, "must obey server polling interval");
		youtube.poll.stop();
		youtube.SetSession("refreshed", "fixture-account");
		Require(youtube.history->count() == 1 && youtube.token == "refreshed",
			"token refresh must preserve chat history");
		Confirm();
		youtube.Moderate("timeout", "one", "viewer");
		Require(youtubeNetwork.calls.last().body.value("snippet").toObject().value("type").toString() ==
				"temporary",
			"YouTube timeout body incorrect");
		youtubeNetwork.Finish(200, {{"id", "ban-id"}});
		Require(youtube.history->count() == 0, "successful ban must remove user messages");
		Confirm();
		youtube.Moderate("unban", "one", "viewer");
		Require(youtubeNetwork.calls.last().request.url().query() == "id=ban-id",
			"YouTube unban requires ban id, not user id");
		youtubeNetwork.Finish(204, {});
		Require(!youtube.bans.contains("viewer"), "unban must clear stored ban id");
		// A queued reply for an old account must never repopulate a disconnected view.
		youtube.Read();
		auto pending = youtubeNetwork.calls.last().reply;
		youtube.Stop();
		Require(youtube.history->count() == 0 && youtube.active.isEmpty() && !youtube.poll.isActive(),
			"disconnect must clear YouTube data and stop polling");
		Require(pending->isFinished(), "disconnect must abort pending requests");
	}
};
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	PenguinChatTest::Run();
	std::cout << "Chat actions, permissions, pagination, token refresh and disconnect passed\n";
}
