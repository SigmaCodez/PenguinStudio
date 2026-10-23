// SPDX-License-Identifier: GPL-2.0-or-later
#include "../plugins/penguin-platforms/twitch-chat.hpp"
#include <QApplication>
#include <cstdlib>
#include <iostream>

void Require(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}
void FlushChat()
{
	QEventLoop loop;
	QTimer::singleShot(75, &loop, &QEventLoop::quit);
	loop.exec();
}
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	TwitchChat chat(nullptr, [](const char *key) { return QString::fromUtf8(key); });
	auto *history = chat.findChild<QListWidget *>("penguinChatHistory");
	Require(history != nullptr, "native chat history missing");
	const QJsonObject event{{"message_id", "one"},
				{"chatter_user_id", "42"},
				{"chatter_user_name", "Alice"},
				{"message", QJsonObject{{"text", "<script>alert('text only')</script>"}}}};
	chat.ApplyEvent("channel.chat.message", event);
	chat.ApplyEvent("channel.chat.message", event);
	FlushChat();
	Require(history->count() == 1 && history->item(0)->text() == "Alice: <script>alert('text only')</script>",
		"chat must be plain text and deduplicated");
	chat.ApplyEvent("channel.chat.message_delete", {{"message_id", "one"}});
	FlushChat();
	Require(history->count() == 0, "deleted message retained");
	chat.ApplyEvent("channel.chat.message", event);
	chat.ApplyEvent("channel.chat.clear_user_messages", {{"target_user_id", "42"}});
	FlushChat();
	Require(history->count() == 0, "moderated user messages retained");
	chat.ApplyEvent("channel.chat.message", event);
	chat.ApplyEvent("channel.chat.clear", {});
	FlushChat();
	Require(history->count() == 0, "cleared chat retained");
	for (int i = 0; i < 600; ++i) {
		auto message = event;
		message["message_id"] = QString::number(i);
		chat.ApplyEvent("channel.chat.message", message);
	}
	FlushChat();
	Require(history->count() == 500, "chat history must be bounded");
	chat.resize(420, 640);
	chat.show();
	QApplication::processEvents();
	history->verticalScrollBar()->setValue(history->verticalScrollBar()->maximum() / 2);
	const int readingPosition = history->verticalScrollBar()->value();
	auto next = event;
	next["message_id"] = "later";
	chat.ApplyEvent("channel.chat.message", next);
	FlushChat();
	QApplication::processEvents();
	Require(readingPosition > 0 && history->verticalScrollBar()->value() == readingPosition,
		"new messages must not reset a reader's scroll position");
	Require(TwitchChat::IsEndpoint(QUrl("wss://eventsub.wss.twitch.tv/ws?reconnect=123")),
		"legitimate reconnect rejected");
	for (const auto *url :
	     {"ws://eventsub.wss.twitch.tv/ws", "wss://evil.example/ws", "wss://eventsub.wss.twitch.tv.evil.example/ws",
	      "wss://user@eventsub.wss.twitch.tv/ws", "wss://eventsub.wss.twitch.tv:8443/ws"}) {
		Require(!TwitchChat::IsEndpoint(QUrl(url)), "unsafe reconnect accepted");
	}
	chat.Stop();
	FlushChat();
	Require(history->count() == 0, "disconnect must clear account messages");
	std::cout << "Native chat moderation, bounded history and reconnect validation passed\n";
}
