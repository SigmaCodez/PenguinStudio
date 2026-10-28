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
		Require(twitch.recentBans.contains("viewer"),
			"completed timeout must remain manageable after messages disappear");
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

		const int beforeInvalid = twitchNetwork.calls.size();
		twitch.Moderate("delete", "", "viewer");
		twitch.Moderate("unknown", "one", "viewer");
		twitch.Moderate("ban", "", "");
		twitch.Moderate("ban", "", "owner");
		Require(twitchNetwork.calls.size() == beforeInvalid,
			"invalid Twitch moderation must not send requests");
		QTimer::singleShot(0, [] {
			for (auto *w : QApplication::topLevelWidgets()) {
				if (auto *b = qobject_cast<QMessageBox *>(w)) {
					b->button(QMessageBox::Cancel)->click();
				}
			}
		});
		twitch.Moderate("ban", "", "viewer");
		Require(twitchNetwork.calls.size() == beforeInvalid, "cancelled moderation must not send requests");
		twitch.messages.push_back({"one", "viewer", "Text", "Viewer", "Text"});
		twitch.messages.push_back({"two", "other", "Other", "Other", "Other"});
		Confirm();
		twitch.Moderate("delete", "one", "viewer");
		twitchNetwork.Finish(204, {});
		Require(twitch.messages.size() == 1 && twitch.messages.first().id == "two",
			"successful Twitch deletion must update only the selected message without EventSub");
		Confirm();
		twitch.Moderate("ban", "", "viewer");
		Require(!twitchNetwork.calls.last().body.value("data").toObject().contains("duration"),
			"permanent Twitch ban must omit timeout duration");
		const int pendingCount = twitchNetwork.calls.size();
		twitch.Moderate("ban", "", "other");
		Require(twitchNetwork.calls.size() == pendingCount,
			"pending moderation must exclude duplicate actions");
		twitchNetwork.Finish(200, {});
		Confirm();
		twitch.Moderate("unban", "", "viewer");
		twitchNetwork.Finish(409, {});
		Require(twitch.recentBans.contains("viewer") && twitch.status->text().contains("in progress"),
			"conflicting unban must retain ban state and explain conflict");
		Confirm();
		twitch.Moderate("unban", "", "viewer");
		twitchNetwork.Finish(204, {});
		Require(!twitch.recentBans.contains("viewer"), "successful Twitch unban must remove the session entry");
		twitch.ChatSettings();
		twitchNetwork.Finish(
			200, {{"data", QJsonArray{QJsonObject{{"slow_mode", false}, {"follower_mode", false}}}}});
		QDialog *settingsDialog = nullptr;
		for (auto *w : QApplication::topLevelWidgets()) {
			if (w->isVisible() && w->windowTitle() == "Twitch chat settings") {
				settingsDialog = qobject_cast<QDialog *>(w);
			}
		}
		Require(settingsDialog, "chat settings dialog missing");
		for (auto *box : settingsDialog->findChildren<QCheckBox *>()) {
			box->setChecked(true);
		}
		settingsDialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
		const auto settingsBody = twitchNetwork.calls.last().body;
		Require(settingsBody.value("emote_mode").toBool() && settingsBody.value("subscriber_mode").toBool() &&
				settingsBody.value("unique_chat_mode").toBool() &&
				settingsBody.value("slow_mode_wait_time").toInt() == 30 &&
				settingsBody.value("follower_mode_duration").toInt() == 0,
			"chat settings payload must preserve supported modes and durations");
		twitchNetwork.Finish(429, {});
		Require(twitch.status->text().contains("limit"),
			"chat settings rate limit must not appear as permissions failure");

		twitch.ChatSettings();
		twitchNetwork.Finish(200,
				     {{"data", QJsonArray{QJsonObject{{"slow_mode", true}, {"follower_mode", true}}}}});
		for (auto *w : QApplication::topLevelWidgets()) {
			if (w->isVisible() && w->windowTitle() == "Twitch chat settings") {
				settingsDialog = qobject_cast<QDialog *>(w);
			}
		}
		for (auto *box : settingsDialog->findChildren<QCheckBox *>()) {
			box->setChecked(false);
		}
		settingsDialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
		Require(!twitchNetwork.calls.last().body.contains("slow_mode_wait_time") &&
				!twitchNetwork.calls.last().body.contains("follower_mode_duration"),
			"disabled chat modes must omit their duration fields");
		twitchNetwork.Finish(200, {});
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
		youtube.SetBroadcast("selected-video");
		youtube.SetSession("fixture-token", "fixture-account");
		Require(youtubeNetwork.calls.size() == 1, "YouTube sign-in should load the selected broadcast");
		Require(youtubeNetwork.calls.last().request.url().query().contains("id=selected-video") &&
				!youtubeNetwork.calls.last().request.url().query().contains("broadcastStatus=active"),
			"Selected upcoming broadcasts must not be excluded by active-only filtering");
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
		Require(youtubeNetwork.calls.last().body.value("snippet").toObject().value("banDurationSeconds").toInt() ==
				600,
			"YouTube timeout must last exactly 10 minutes");
		youtubeNetwork.Finish(200, {{"id", "ban-id"}});
		Require(youtube.history->count() == 0, "successful ban must remove user messages");
		Confirm();
		youtube.Moderate("unban", "one", "viewer");
		Require(youtubeNetwork.calls.last().request.url().query() == "id=ban-id",
			"YouTube unban requires ban id, not user id");
		youtubeNetwork.Finish(204, {});
		Require(!youtube.bans.contains("viewer"), "unban must clear stored ban id");

		const int ytBefore = youtubeNetwork.calls.size();
		youtube.Moderate("delete", "", "viewer");
		youtube.Moderate("unknown", "one", "viewer");
		youtube.Moderate("ban", "one", "");
		youtube.Moderate("unban", "", "missing");
		Require(youtubeNetwork.calls.size() == ytBefore,
			"invalid YouTube actions and unknown bans must not send requests");
		Confirm();
		youtube.Moderate("ban", "one", "viewer");
		Require(youtubeNetwork.calls.last().body.value("snippet").toObject().value("type") == "permanent" &&
				!youtubeNetwork.calls.last().body.value("snippet").toObject().contains(
					"banDurationSeconds"),
			"permanent YouTube ban payload incorrect");
		youtubeNetwork.Finish(200, {});
		Require(!youtube.bans.contains("viewer") && youtube.status->text().contains("no ban ID"),
			"missing ban identifier must not create an unusable undo action");
		for (int code : {400, 403, 404, 409, 429, 500, 0}) {
			Confirm();
			youtube.Moderate("timeout", "one", "viewer");
			youtubeNetwork.Finish(code, {});
			Require(!youtube.moderating && !youtube.bans.contains("viewer") && !youtube.token.isEmpty(),
				"failed YouTube moderation must preserve session and release pending action");
		}
		Require(PenguinChat::ModerationFailure(
				403, {{"error",
				       QJsonObject{{"errors", QJsonArray{QJsonObject{{"reason", "quotaExceeded"}}}}}}})
				.contains("quota"),
			"quota error must not be reported as permission denial");
		Confirm();
		youtube.Moderate("ban", "one", "viewer");
		youtubeNetwork.Finish(200, {{"id", "ban-again"}});
		Confirm();
		youtube.Moderate("unban", "", "viewer");
		youtubeNetwork.Finish(403, {});
		Require(youtube.bans.value("viewer") == "ban-again",
			"denied unban must retain the identifier for a later retry");
		Confirm();
		youtube.Moderate("unban", "", "viewer");
		youtubeNetwork.Finish(204, {});
		youtube.Read();
		youtubeNetwork.Finish(401, {});
		Require(!youtube.ready && !youtube.audienceTimer.isActive(),
			"expired credentials must stop chat actions and audience polling");
		youtube.SetSession("renewed-again", "fixture-account");
		Require(youtubeNetwork.calls.last().request.rawHeader("Authorization") == "Bearer renewed-again",
			"reauthorization must retry with refreshed token");
		youtubeNetwork.Finish(
			200, {{"items", QJsonArray()}, {"nextPageToken", "later"}, {"pollingIntervalMillis", 30000}});
		Require(youtube.ready, "successful reauthorization must restore chat actions");
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
