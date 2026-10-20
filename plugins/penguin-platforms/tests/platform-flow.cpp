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
	void Respond(qsizetype index, int code, const QJsonObject &body = {})
	{
		Require(index >= 0 && index < calls.size() && calls[index].reply, "expected HTTP request missing");
		calls[index].reply->Complete(code, body);
	}

protected:
	QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *body) override
	{
		auto *reply = new Reply(request, this);
		calls.append({request, body ? body->readAll() : QByteArray(), reply});
		return reply;
	}
};

QPushButton *Button(QWidget &pane, const QString &text)
{
	for (auto *button : pane.findChildren<QPushButton *>()) {
		if (button->text() == text || (text == "Connect" && button->text() == "Twitch.Connect")) {
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
		Tick(*pane, 3540 * 1000);
		Require(network.calls[9].body.contains("refresh_token=rotated-refresh"),
			"one-use refresh token was reused");
		const auto pending = network.calls[9].reply;
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
				{{"items", QJsonArray{Broadcast("first", "First title")}},
				 {"nextPageToken", "next+page"}});
		Require(QUrlQuery(network.calls[2].request.url()).queryItemValue("pageToken", QUrl::FullyDecoded) ==
				"next+page",
			"pagination token was not encoded correctly");
		network.Respond(2, 200, {{"items", QJsonArray{Broadcast("second", "Second title")}}});
		auto *broadcasts = pane.findChild<QComboBox *>();
		Require(broadcasts && broadcasts->count() == 2, "all broadcast pages must be loaded");
		broadcasts->setCurrentIndex(1);
		Line(pane, "Title")->setText("A new title");
		Button(pane, "Update")->click();
		Tick(pane, 3540 * 1000);
		network.Respond(4, 200, Tokens("refreshed-google-access"));
		Require(!Button(pane, "Update")->isEnabled(), "Google refresh must not unlock an in-flight update");
		network.Respond(5, 200,
				{{"items",
				  QJsonArray{Broadcast("first", "First title"), Broadcast("second", "Second title")}}});
		Require(broadcasts->currentData().toString() == "second" &&
				Line(pane, "Title")->text() == "A new title",
			"token refresh must preserve selected broadcast and edited title");
		const QJsonObject original{{"title", "Second title"},           {"categoryId", "20"},
					   {"description", "Keep description"}, {"tags", QJsonArray{"one", "two"}},
					   {"defaultLanguage", "en"},           {"defaultAudioLanguage", "en"},
					   {"channelId", "read-only"},          {"publishedAt", "read-only"}};
		network.Respond(3, 200, {{"items", QJsonArray{QJsonObject{{"snippet", original}}}}});
		Require(network.calls[6].request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() == "PUT",
			"YouTube videos.update must use PUT");
		const auto update = QJsonDocument::fromJson(network.calls[6].body).object();
		auto expected = original;
		expected["title"] = "A new title";
		expected.remove("channelId");
		expected.remove("publishedAt");
		Require(update.value("id").toString() == "second" && update.value("snippet").toObject() == expected,
			"title update must preserve mutable snippet fields and omit read-only fields");
		Line(pane, "Title")->setText("A later unsaved edit");
		network.Respond(6, 200);
		Require(Line(pane, "Title")->text() == "A later unsaved edit", "completion must not erase later edits");
		broadcasts->setCurrentIndex(0);
		broadcasts->setCurrentIndex(1);
		Require(Line(pane, "Title")->text() == "A new title",
			"successful update must refresh cached broadcast title");
		Button(pane, "YouTube.Refresh")->click();
		network.Respond(7, 200, {{"items", QJsonArray{}}, {"nextPageToken", "repeating-page"}});
		network.Respond(8, 200, {{"items", QJsonArray{}}, {"nextPageToken", "repeating-page"}});
		Require(network.calls.size() == 9 && Button(pane, "YouTube.Refresh")->isEnabled() &&
				broadcasts->count() == 2,
			"a pagination cycle must stop without replacing the last complete broadcast list");
		Button(pane, "Update")->click();
		network.Respond(9, 401);
		Require(!Button(pane, "Update")->isEnabled() && Button(pane, "Connect")->isEnabled() &&
				broadcasts->count() == 0,
			"revoked Google token must disconnect and clear stale broadcasts");
		QSettings saved(configuration, QSettings::IniFormat);
		Require(saved.allKeys().size() == 2, "only the two public client IDs may be saved");
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
	QDesktopServices::unsetUrlHandler("https");
	std::cout
		<< "PASS: device polling/backoff, token rotation, in-flight update exclusion, PKCE loopback, pagination, "
		   "snippet preservation, revoked sessions and callback shutdown\n";
}

#include "platform-flow.moc"
