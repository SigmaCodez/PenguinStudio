// SPDX-License-Identifier: GPL-2.0-or-later
// The production panes run against an in-memory HTTP transport. Only the Google
// browser callback uses a real TCP connection, bound to loopback with no credentials.
#include "../penguin-platforms.cpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace {
QString configuration;
class MemoryStore final : public CredentialStore {
public:
	QByteArray value;
	bool failWrite = false, holdRead = false;
	Done pending;
	void Read(Done done) override
	{
		if (holdRead) {
			pending = std::move(done);
		} else {
			done(true, value);
		}
	}
	void Write(const QByteArray &data, Done done) override
	{
		if (!failWrite) {
			value = data;
		}
		done(!failWrite, {});
	}
	void Remove(Done done) override
	{
		value.clear();
		done(true, {});
	}
};

void Require(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(EXIT_FAILURE);
	}
}

void Until(const std::function<bool()> &condition)
{
	QElapsedTimer clock;
	clock.start();
	while (!condition() && clock.elapsed() < 3000) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
	}
	Require(condition(), "asynchronous callback did not complete");
}

class Reply final : public QNetworkReply {
	QByteArray payload;
	qint64 position = 0;

protected:
	qint64 readData(char *out, qint64 count) override
	{
		const auto available = std::min(count, payload.size() - position);
		if (available <= 0) {
			return -1;
		}
		std::memcpy(out, payload.constData() + position, size_t(available));
		position += available;
		return available;
	}

public:
	bool aborted = false;
	Reply(const QNetworkRequest &request, QObject *parent) : QNetworkReply(parent)
	{
		setRequest(request);
		setUrl(request.url());
		open(QIODevice::ReadOnly | QIODevice::Unbuffered);
	}
	qint64 bytesAvailable() const override { return payload.size() - position + QNetworkReply::bytesAvailable(); }
	void Complete(int code, const QJsonObject &body)
	{
		Require(!isFinished(), "request completed more than once");
		setAttribute(QNetworkRequest::HttpStatusCodeAttribute, code);
		payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
		setFinished(true);
		emit readyRead();
		emit finished();
	}
	void abort() override
	{
		if (isFinished()) {
			return;
		}
		aborted = true;
		setError(QNetworkReply::OperationCanceledError, "Cancelled by test controller");
		Complete(0, {});
	}
};

class Transport final : public QNetworkAccessManager {
public:
	struct Call {
		QNetworkRequest request;
		QByteArray body;
		QPointer<Reply> reply;
	};
	QList<Call> calls;
	int automaticChannelLoads = 0;
	int automaticVideoLoads = 0;
	void Respond(qsizetype index, int code, const QJsonObject &body = {})
	{
		if (!(index >= 0 && index < calls.size() && calls[index].reply)) {
			std::cerr << "Missing response index " << index << " of " << calls.size() << "\n";
		}
		Require(index >= 0 && index < calls.size() && calls[index].reply, "expected HTTP request missing");
		calls[index].reply->Complete(code, body);
		if (body.contains("user_id") && !calls.isEmpty() &&
		    calls.last().request.url().path() == "/helix/channels" &&
		    calls.last().request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() == "GET") {
			auto automatic = calls.takeLast();
			++automaticChannelLoads;
			automatic.reply->Complete(
				200, {{"data", QJsonArray{QJsonObject{{"title", "Current title"},
								      {"game_id", "20"},
								      {"game_name", "Current category"}}}}});
		}
	}

protected:
	QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *body) override
	{
		auto *reply = new Reply(request, this);
		if (QUrlQuery(request.url()).queryItemValue("fields") == "items(snippet(title,categoryId))") {
			++automaticVideoLoads;
			QTimer::singleShot(0, reply, [reply] {
				if (!reply->aborted) {
					reply->Complete(
						200,
						{{"items", QJsonArray{QJsonObject{
								   {"snippet", QJsonObject{{"categoryId", "20"}}}}}}});
				}
			});
			return reply;
		}
		calls.append({request, body ? body->readAll() : QByteArray(), reply});
		return reply;
	}
};

QPushButton *Button(QWidget &pane, const QString &text)
{
	for (auto *button : pane.findChildren<QPushButton *>()) {
		if (button->text() == text || (text == "Connect" && (button->text() == "Twitch.Connect" ||
								     button->text() == "YouTube.Connect"))) {
			return button;
		}
	}
	Require(false, "button not found");
	return nullptr;
}

QLineEdit *Line(QWidget &pane, const QString &label)
{
	auto *layout = static_cast<QFormLayout *>(pane.layout());
	for (auto *line : pane.findChildren<QLineEdit *>()) {
		auto *fieldLabel = qobject_cast<QLabel *>(layout->labelForField(line));
		if (fieldLabel && fieldLabel->text() == label) {
			return line;
		}
	}
	Require(false, "text field not found");
	return nullptr;
}

void Tick(QWidget &pane, int interval)
{
	for (auto *timer : pane.findChildren<QTimer *>(QString(), Qt::FindDirectChildrenOnly)) {
		if (timer->isActive() && timer->interval() == interval) {
			timer->stop();
			Require(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection),
				"timer invocation failed");
			return;
		}
	}
	Require(false, "expected active timer not found");
}

QJsonObject Tokens(const QString &access, const QString &refresh = {})
{
	QJsonObject result{{"access_token", access}, {"expires_in", 3600}};
	if (!refresh.isEmpty()) {
		result.insert("refresh_token", refresh);
	}
	return result;
}

QJsonObject Validation()
{
	return {{"client_id", "test-client"},
		{"user_id", "123"},
		{"scopes", QJsonArray{"channel:manage:broadcast", "user:read:chat", "user:write:chat"}}};
}

QJsonObject Broadcast(const QString &id, const QString &name)
{
	return {{"id", id}, {"snippet", QJsonObject{{"title", name}}}};
}
} // namespace

// QDesktopServices invokes a Qt slot; no external browser is opened by this test.
class Browser final : public QObject {
	Q_OBJECT
public:
	QUrl lastUrl;
public slots:
	void Open(const QUrl &url) { lastUrl = url; }
};

extern "C" {
bool text_lookup_getstr(lookup_t *, const char *key, const char **out)
{
	*out = std::strcmp(key, "Twitch.Device") == 0 ? "Twitch.Device %1" : key;
	return true;
}
char *obs_module_get_config_path(obs_module_t *, const char *)
{
	const auto path = configuration.toUtf8();
	return ::strdup(path.constData());
}
void bfree(void *memory)
{
	std::free(memory);
}
}

int main(int argc, char **argv)
{
	QApplication application(argc, argv);
	QTemporaryDir directory;
	Require(directory.isValid(), "test configuration directory unavailable");
	configuration = directory.filePath("platforms.ini");
	Browser browser;
	QDesktopServices::setUrlHandler("https", &browser, "Open");
	{
		Transport network;
		auto pane = std::make_unique<TwitchPane>(nullptr, &network);
		Line(*pane, "AppID")->setText("test-client");
		Button(*pane, "Connect")->click();
		network.Respond(0, 200,
				{{"verification_uri", "https://www.twitch.tv/activate"},
				 {"device_code", "device-code"},
				 {"user_code", "TEST"},
				 {"expires_in", 1800},
				 {"interval", 5}});
		Require(browser.lastUrl.host() == "www.twitch.tv", "Twitch browser authorization not opened");
		Tick(*pane, 5000);
		network.Respond(1, 400, {{"message", "authorization_pending"}});
		Tick(*pane, 5000);
		network.Respond(2, 429, {{"error", "slow_down"}});
		Tick(*pane, 10000);
		network.Respond(3, 200, Tokens("twitch-access", "first+refresh"));
		network.Respond(4, 200, Validation());
		Require(Button(*pane, "Update")->isEnabled(), "validated Twitch session unavailable");
		Require(network.automaticChannelLoads == 1 && Line(*pane, "Title")->text() == "Current title",
			"Twitch details must load immediately after connection");
		Line(*pane, "Title")->setText("Test title");
		Button(*pane, "Update")->click();
		Require(network.calls[5].request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() ==
				"PATCH",
			"Twitch title request must use PATCH");
		Require(network.calls[5].request.rawHeader("Authorization") == "Bearer twitch-access",
			"access token not attached");
		Require(network.calls[5].request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt() ==
				QNetworkRequest::ManualRedirectPolicy,
			"credential-bearing requests must not follow redirects");
		Tick(*pane, 55 * 60 * 1000);
		network.Respond(6, 200, Validation());
		Require(!Button(*pane, "Update")->isEnabled(),
			"validation must not unlock an in-flight metadata update");
		network.Respond(5, 204);
		Tick(*pane, 3540 * 1000);
		Require(network.calls[7].body.contains("refresh_token=first%2Brefresh"),
			"refresh token form encoding incorrect");
		network.Respond(7, 200, Tokens("new-access", "rotated-refresh"));
		network.Respond(8, 200, Validation());
		Button(*pane, "Twitch.Load")->click();
		Require(network.calls[9].request.url().path() == "/helix/channels",
			"Channel information request missing");
		Line(*pane, "Title")->setText("Local edit while loading");
		network.Respond(9, 200,
				{{"data", QJsonArray{QJsonObject{{"title", "Remote title"}, {"game_id", "509658"}}}}});
		Require(Line(*pane, "Title")->text() == "Local edit while loading",
			"Loading overwrote an unsaved title");
		Require(static_cast<CategoryPicker *>(pane->findChild<QComboBox *>("twitchCategory"))->Id() == "509658",
			"Channel category was not loaded");
		Button(*pane, "Twitch.Load")->click();
		network.Respond(10, 200,
				{{"data", QJsonArray{QJsonObject{{"title", "Remote title"}, {"game_id", "509658"}}}}});
		Require(Line(*pane, "Title")->text() == "Remote title", "Current channel title was not loaded");
		Tick(*pane, 3540 * 1000);
		Require(network.calls[11].body.contains("refresh_token=rotated-refresh"),
			"one-use refresh token was reused");
		const auto pending = network.calls[11].reply;
		Button(*pane, "Disconnect")->click();
		Require(pending && pending->aborted && !Button(*pane, "Update")->isEnabled(),
			"disconnect must cancel pending requests");
		Button(*pane, "Connect")->click();
		const auto duringShutdown = network.calls.last().reply;
		pane.reset();
		Require(duringShutdown && duringShutdown->aborted, "pane destruction must abort callbacks safely");
	}
	{
		Transport network;
		YoutubePane pane(nullptr, &network);
		Require(Button(pane, "Connect")->text() == "YouTube.Connect",
			"YouTube must expose the browser sign-in action");
		Require(Line(pane, "AppID")->isHidden() && Line(pane, "ClientSecret")->isHidden(),
			"Application credentials must not appear in the normal sign-in flow");
		Button(pane, "YouTube.AppSettings")->click();
		Require(!Line(pane, "AppID")->isHidden() && !Line(pane, "ClientSecret")->isHidden(),
			"Custom application settings must remain accessible");
		Button(pane, "YouTube.AppSettings")->click();
		Line(pane, "AppID")->setText("google-client");
		Line(pane, "ClientSecret")->setText("memory-only-secret");
		Button(pane, "Connect")->click();
		const QUrlQuery authorization(browser.lastUrl);
		const QUrl callback(authorization.queryItemValue("redirect_uri"));
		Require(callback.host() == "127.0.0.1" && callback.port() > 0, "Google callback must use loopback");
		QTcpSocket socket;
		socket.connectToHost(callback.host(), quint16(callback.port()));
		Until([&]() { return socket.state() == QAbstractSocket::ConnectedState; });
		socket.write("GET /oauth/callback?state=" + authorization.queryItemValue("state").toUtf8() +
			     "&code=code%2Bvalue HTTP/1.1\r\nHost: localhost\r\n\r\n");
		Until([&]() { return network.calls.size() == 1; });
		const QUrlQuery tokenParameters(QString::fromUtf8(network.calls[0].body));
		Require(tokenParameters.queryItemValue("code", QUrl::FullyDecoded) == "code+value",
			"callback code corrupted");
		Require(PenguinOAuth::Challenge(tokenParameters.queryItemValue("code_verifier")) ==
				authorization.queryItemValue("code_challenge").toLatin1(),
			"PKCE challenge does not match verifier");
		network.Respond(0, 200, Tokens("google-access", "google-refresh"));
		network.Respond(1, 200,
				{{"items", QJsonArray{QJsonObject{{"id", "20"},
								  {"snippet", QJsonObject{{"title", "Gaming"},
											  {"assignable", true}}}}}}});
		network.Respond(2, 200,
				{{"items", QJsonArray{Broadcast("first", "First title")}},
				 {"nextPageToken", "next+page"}});
		Require(QUrlQuery(network.calls[3].request.url()).queryItemValue("pageToken", QUrl::FullyDecoded) ==
				"next+page",
			"pagination token was not encoded correctly");
		network.Respond(3, 200, {{"items", QJsonArray{Broadcast("second", "Second title")}}});
		auto *broadcasts = pane.findChild<QComboBox *>();
		Require(broadcasts && broadcasts->count() == 2, "all broadcast pages must be loaded");
		Require(network.automaticVideoLoads == 1, "YouTube selected broadcast details must load automatically");
		broadcasts->setCurrentIndex(1);
		Line(pane, "Title")->setText("A new title");
		Button(pane, "Update")->click();
		Tick(pane, 3540 * 1000);
		network.Respond(5, 200, Tokens("refreshed-google-access"));
		Require(!Button(pane, "Update")->isEnabled(), "Google refresh must not unlock an in-flight update");
		network.Respond(6, 200,
				{{"items",
				  QJsonArray{Broadcast("first", "First title"), Broadcast("second", "Second title")}}});
		Require(broadcasts->currentData().toString() == "second" &&
				Line(pane, "Title")->text() == "A new title",
			"token refresh must preserve selected broadcast and edited title");
		const QJsonObject original{{"title", "Second title"},           {"categoryId", "20"},
					   {"description", "Keep description"}, {"tags", QJsonArray{"one", "two"}},
					   {"defaultLanguage", "en"},           {"defaultAudioLanguage", "en"},
					   {"channelId", "read-only"},          {"publishedAt", "read-only"}};
		network.Respond(4, 200, {{"items", QJsonArray{QJsonObject{{"snippet", original}}}}});
		Require(network.calls[7].request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() == "PUT",
			"YouTube videos.update must use PUT");
		const auto update = QJsonDocument::fromJson(network.calls[7].body).object();
		auto expected = original;
		expected["title"] = "A new title";
		expected.remove("channelId");
		expected.remove("publishedAt");
		Require(update.value("id").toString() == "second" && update.value("snippet").toObject() == expected,
			"title update must preserve mutable snippet fields and omit read-only fields");
		Line(pane, "Title")->setText("A later unsaved edit");
		network.Respond(7, 200);
		Require(Line(pane, "Title")->text() == "A later unsaved edit", "completion must not erase later edits");
		broadcasts->setCurrentIndex(0);
		broadcasts->setCurrentIndex(1);
		Require(Line(pane, "Title")->text() == "A new title",
			"successful update must refresh cached broadcast title");
		Button(pane, "YouTube.Refresh")->click();
		network.Respond(8, 200, {{"items", QJsonArray{}}, {"nextPageToken", "repeating-page"}});
		network.Respond(9, 200, {{"items", QJsonArray{}}, {"nextPageToken", "repeating-page"}});
		Require(network.calls.size() == 10 && Button(pane, "YouTube.Refresh")->isEnabled() &&
				broadcasts->count() == 2,
			"a pagination cycle must stop without replacing the last complete broadcast list");
		Button(pane, "Update")->click();
		network.Respond(10, 401);
		Require(!Button(pane, "Update")->isEnabled() && Button(pane, "Connect")->isEnabled() &&
				broadcasts->count() == 0,
			"revoked Google token must disconnect and clear stale broadcasts");
		QSettings saved(configuration, QSettings::IniFormat);
		for (const auto &key : saved.allKeys()) {
			Require(key.endsWith("/clientId") || key.endsWith("/sessionEnabled"),
				"credentials leaked to settings");
		}
	}
	{
		Transport network;
		auto pane = std::make_unique<YoutubePane>(nullptr, &network);
		Button(*pane, "Connect")->click();
		const QUrl callback(QUrlQuery(browser.lastUrl).queryItemValue("redirect_uri"));
		QTcpSocket partial;
		partial.connectToHost(callback.host(), quint16(callback.port()));
		Until([&]() { return partial.state() == QAbstractSocket::ConnectedState; });
		partial.write("GET /oauth/");
		QCoreApplication::processEvents();
		pane.reset();
		Until([&]() { return partial.state() == QAbstractSocket::UnconnectedState; });
		Require(network.calls.isEmpty(), "partial callback must never exchange a token during shutdown");
	}

	{
		QSettings settings(configuration, QSettings::IniFormat);
		settings.clear();
		MemoryStore store;
		store.value =
			QJsonDocument(QJsonObject{{"client", "test-client"}, {"refresh", "saved-refresh"}}).toJson();
		Transport network;
		auto pane = std::make_unique<TwitchPane>(nullptr, &network, nullptr, &store);
		Line(*pane, "AppID")->setText("test-client");
		Until([&]() { return network.calls.size() == 1; });
		Require(network.calls[0].body.contains("refresh_token=saved-refresh"),
			"saved Twitch session did not refresh on startup");
		network.Respond(0, 200, Tokens("access", "rotated-persistent-refresh"));
		network.Respond(1, 200, Validation());
		Require(QJsonDocument::fromJson(store.value).object().value("refresh") == "rotated-persistent-refresh",
			"rotated token not saved");
		auto *picker = static_cast<CategoryPicker *>(pane->findChild<QComboBox *>("twitchCategory"));
		const auto type = [&](const QString &query) {
			picker->lineEdit()->setText(query);
			QMetaObject::invokeMethod(picker->lineEdit(), "textEdited", Q_ARG(QString, query));
			Tick(*pane, 300);
		};
		type("Old");
		type("New");
		network.Respond(3, 200, {{"data", QJsonArray{QJsonObject{{"name", "New game"}, {"id", "42"}}}}});
		network.Respond(2, 200, {{"data", QJsonArray{QJsonObject{{"name", "Old game"}, {"id", "41"}}}}});
		Require(picker->count() == 1 && picker->itemText(0) == "New game",
			"stale category search replaced new results");
		picker->setCurrentIndex(0);
		Require(picker->Id() == "42", "category display name did not resolve to ID");
		picker->setEditText("unselected category");
		Line(*pane, "Title")->setText("Test");
		Button(*pane, "Update")->click();
		Require(network.calls.size() == 4, "unselected category sent an invalid metadata update");
		pane.reset();
		Require(!store.value.isEmpty(), "shutdown erased stored sign-in");
		Transport restarted;
		auto restored = std::make_unique<TwitchPane>(nullptr, &restarted, nullptr, &store);
		Line(*restored, "AppID")->setText("test-client");
		Until([&]() { return restarted.calls.size() == 1; });
		Require(restarted.calls[0].body.contains("rotated-persistent-refresh"),
			"restart reused an obsolete refresh token");
		Button(*restored, "Disconnect")->click();
		Require(store.value.isEmpty(), "disconnect did not erase saved sign-in");
	}
	{
		QSettings settings(configuration, QSettings::IniFormat);
		settings.clear();
		MemoryStore store;
		store.holdRead = true;
		Transport network;
		TwitchPane pane(nullptr, &network, nullptr, &store);
		Line(pane, "AppID")->setText("test-client");
		Until([&]() { return bool(store.pending); });
		Button(pane, "Disconnect")->click();
		store.pending(
			true,
			QJsonDocument(QJsonObject{{"client", "test-client"}, {"refresh", "late-refresh"}}).toJson());
		QCoreApplication::processEvents();
		Require(network.calls.isEmpty(), "late keyring read resurrected a disconnected account");
	}

	{
		QSettings settings(configuration, QSettings::IniFormat);
		settings.clear();
		MemoryStore store;
		store.value = QJsonDocument(QJsonObject{{"client", "google-client"},
							{"refresh", "google-saved"},
							{"secret", "saved-desktop-secret"}})
				      .toJson();
		Transport network;
		YoutubePane pane(nullptr, &network, &store);
		Line(pane, "AppID")->setText("google-client");
		Until([&]() { return network.calls.size() == 1; });
		Require(network.calls[0].body.contains("refresh_token=google-saved") &&
				network.calls[0].body.contains("client_secret=saved-desktop-secret"),
			"Google saved credentials did not restore");
		store.failWrite = true;
		network.Respond(0, 200, Tokens("google-access", "new-refresh"));
		Require(pane.findChild<QLabel *>("penguinCredentialStatus")->text() == "Session.Unavailable",
			"keyring save failure was hidden");
		Require(Button(pane, "Update")->isEnabled(), "keyring failure unnecessarily disconnected live account");
		network.Respond(
			1, 200,
			{{"items",
			  QJsonArray{
				  QJsonObject{{"id", "20"},
					      {"snippet", QJsonObject{{"title", "Gaming"}, {"assignable", true}}}},
				  QJsonObject{{"id", "27"},
					      {"snippet", QJsonObject{{"title", "Education"}, {"assignable", true}}}},
				  QJsonObject{{"id", "99"},
					      {"snippet", QJsonObject{{"title", "Hidden"}, {"assignable", false}}}}}}});
		network.Respond(2, 200, {{"items", QJsonArray{Broadcast("selected-video", "Original title")}}});
		auto *category = static_cast<CategoryPicker *>(pane.findChild<QComboBox *>("youtubeCategory"));
		Require(category->count() == 2, "unassignable YouTube categories were offered");
		category->Select("27", "Education");
		Button(pane, "Update")->click();
		network.Respond(3, 200,
				{{"items", QJsonArray{QJsonObject{
						   {"snippet", QJsonObject{{"title", "Original title"},
									   {"categoryId", "20"},
									   {"description", "Preserve this"}}}}}}});
		const auto snippet =
			QJsonDocument::fromJson(network.calls[4].body).object().value("snippet").toObject();
		Require(snippet.value("categoryId") == "27" && snippet.value("description") == "Preserve this",
			"YouTube category update lost existing metadata");
		network.Respond(4, 200);
		Button(pane, "Disconnect")->click();
		Require(store.value.isEmpty(), "Google saved account was not removed");
	}
	QDesktopServices::unsetUrlHandler("https");
	{
		QSettings settings(configuration, QSettings::IniFormat);
		settings.clear();
		MemoryStore store;
		store.value = QJsonDocument(QJsonObject{{"client", "test-client"}, {"refresh", "saved"}}).toJson();
		Transport network;
		TwitchPane pane(nullptr, &network, nullptr, &store);
		Line(pane, "AppID")->setText("test-client");
		Until([&] { return network.calls.size() == 1; });
		network.Respond(0, 200, Tokens("access", "refresh"));
		network.Respond(1, 200, Validation());
		Line(pane, "Title")->setText("Unsaved title before opening");
		Line(pane, "Title")->setModified(true);
		auto *picker = pane.findChild<QComboBox *>("twitchCategory");
		picker->setEditText("Unsaved category");
		QMetaObject::invokeMethod(picker, "activated", Q_ARG(int, 0));
		pane.show();
		Require(network.calls.size() == 3, "Opening Twitch details must refresh automatically");
		pane.hide(); pane.show();
		Require(network.calls.size() == 3, "Opening an in-flight details pane must not duplicate the request");
		network.Respond(2, 200, {{"data", QJsonArray{QJsonObject{{"title", "External title"}, {"game_id", "42"}, {"game_name", "External category"}}}}});
		Require(Line(pane, "Title")->text() == "Unsaved title before opening" && picker->currentText() == "Unsaved category",
			"Automatic refresh must preserve edits made before the request");
		pane.hide();
	}

	// The first API result may be a completed broadcast. Restore an explicit
	// choice, otherwise prefer a live broadcast across all result pages.
	for (const QString saved : {QString(), QString("old"), QString("missing")}) {
		QSettings settings(configuration, QSettings::IniFormat);
		settings.clear();
		settings.setValue("youtube/selectedBroadcast", saved);
		settings.sync();
		MemoryStore store;
		store.value = QJsonDocument(QJsonObject{{"client", "google-client"}, {"refresh", "saved"},
			{"secret", "desktop-test-secret"}}).toJson();
		Transport network;
		YoutubePane pane(nullptr, &network, &store);
		Line(pane, "AppID")->setText("google-client");
		QString chatVideo;
		pane.chatLocation = [&](const QString &id) { chatVideo = id; };
		Until([&] { return network.calls.size() == 1; });
		network.Respond(0, 200, Tokens("access", "refresh"));
		network.Respond(1, 200, {{"items", QJsonArray{}}});
		auto old = Broadcast("old", "Completed broadcast");
		old.insert("status", QJsonObject{{"lifeCycleStatus", "complete"}});
		auto live = Broadcast("live", "Current broadcast");
		live.insert("status", QJsonObject{{"lifeCycleStatus", "live"}});
		network.Respond(2, 200, {{"items", QJsonArray{old}}, {"nextPageToken", "page-two"}});
		network.Respond(3, 200, {{"items", QJsonArray{live}}});
		auto *combo = pane.findChild<QComboBox *>("youtubeBroadcast");
		const QString expected = saved == "old" ? "old" : "live";
		Require(combo && combo->currentData().toString() == expected && chatVideo == expected,
			"YouTube must restore selection or prefer live broadcasts and update chat consistently");
		Require(Line(pane, "Title")->text() == combo->currentText(), "Restored broadcast title mismatch");
		Require(pane.findChild<QLabel *>("youtubeBroadcastIdentity")->text() == "Video: " + expected, "Visible video ID must match chat");
		combo->setCurrentIndex(combo->findData("live"));
		QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, combo->currentIndex()));
		Require(QSettings(configuration, QSettings::IniFormat).value("youtube/selectedBroadcast").toString() == "live",
			"Explicit broadcast selection must persist for restart");
	}

	std::cout
		<< "PASS: device polling/backoff, token rotation, in-flight update exclusion, PKCE loopback, pagination, "
		   "snippet preservation, revoked sessions and callback shutdown\n";
}

#include "platform-flow.moc"
