// SPDX-License-Identifier: GPL-2.0-or-later
// Native metadata controls: no embedded browser or X11 dependency.
#include "oauth-helpers.hpp"
#include "twitch-chat.hpp"
#include "youtube-chat.hpp"
#include "category-picker.hpp"
#include "credential-store.hpp"
#if __has_include("youtube-app-config.hpp")
#include "youtube-app-config.hpp"
#endif
#ifndef PENGUIN_YOUTUBE_CLIENT_ID
#define PENGUIN_YOUTUBE_CLIENT_ID ""
#define PENGUIN_YOUTUBE_CLIENT_SECRET ""
#endif
#include <QLocale>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/bmem.h>

#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSettings>
#include <QSet>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("penguin-platforms", "en-US")

namespace {
QString Text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

using PenguinOAuth::Form;
using PenguinOAuth::RandomToken;

class PlatformPane : public QWidget {
protected:
	QNetworkAccessManager ownedNetwork{this};
	QNetworkAccessManager &network;
	QTimer refreshTimer{this};
	QFormLayout *form = new QFormLayout(this);
	QLineEdit *client = new QLineEdit(this);
	QLineEdit *title = new QLineEdit(this);
	QLabel *status = new QLabel(this);
	QPushButton *connectButton = new QPushButton(Text("Connect"), this);
	QPushButton *disconnectButton = new QPushButton(Text("Disconnect"), this);
	QPushButton *updateButton = new QPushButton(Text("Update"), this);
	QString accessToken;
	QString refreshToken;
	QString activeClient;
	bool updatePending = false;
	quint64 generation = 0;
	QString provider;
	QString configPath;
	CredentialStore *credentials = nullptr;
	QLabel *credentialStatus = new QLabel(this);
	virtual QString SessionSecret() const { return {}; }
	virtual void RestoreSecret(const QString &) {}
	void SaveSession()
	{
		if (!credentials || refreshToken.isEmpty()) {
			return;
		}
		const auto attempt = generation;
		const auto value = QJsonDocument(QJsonObject{{"client", activeClient},
							     {"refresh", refreshToken},
							     {"secret", SessionSecret()}})
					   .toJson(QJsonDocument::Compact);
		QPointer<PlatformPane> self(this);
		credentials->Write(value, [self, attempt](bool ok, const QByteArray &) {
			if (!self || self->generation != attempt) {
				return;
			}
			self->credentialStatus->setText(Text(ok ? "Session.Saved" : "Session.Unavailable"));
			if (ok) {
				QSettings settings(self->configPath, QSettings::IniFormat);
				settings.setValue(self->provider + "/sessionEnabled", true);
			}
		});
	}
	void RestoreSession()
	{
		if (!credentials) {
			return;
		}
		QSettings settings(configPath, QSettings::IniFormat);
		if (!settings.value(provider + "/sessionEnabled", true).toBool()) {
			return;
		}
		const auto attempt = generation;
		QPointer<PlatformPane> self(this);
		credentials->Read([self, attempt](bool ok, const QByteArray &value) {
			if (!self || self->generation != attempt) {
				return;
			}
			if (!ok) {
				self->credentialStatus->setText(Text("Session.Unavailable"));
				return;
			}
			if (value.isEmpty()) {
				return;
			}
			const auto data = QJsonDocument::fromJson(value).object();
			if (data.value("client").toString() != self->client->text().trimmed() ||
			    data.value("refresh").toString().isEmpty()) {
				return;
			}
			self->activeClient = data.value("client").toString();
			self->refreshToken = data.value("refresh").toString();
			self->RestoreSecret(data.value("secret").toString());
			self->client->setEnabled(false);
			self->connectButton->setEnabled(false);
			self->status->setText(Text("Session.Restoring"));
			self->refreshTimer.start(1);
		});
	}
	void ForgetSession()
	{
		QSettings settings(configPath, QSettings::IniFormat);
		settings.setValue(provider + "/sessionEnabled", false);
		Reset();
		if (!credentials) {
			return;
		}
		const auto attempt = generation;
		QPointer<PlatformPane> self(this);
		credentials->Remove([self, attempt](bool ok, const QByteArray &) {
			if (self && self->generation == attempt) {
				self->credentialStatus->setText(Text(ok ? "Session.Removed" : "Session.RemoveFailed"));
			}
		});
	}

	using Completion = std::function<void(int, const QJsonObject &)>;

	void Request(const QString &url, const QByteArray &method, const QByteArray &body,
		     const QByteArray &contentType, bool authenticated, Completion done)
	{
		QNetworkRequest request{QUrl(url)};
		request.setTransferTimeout(20000);
		// Credentials must never be forwarded to a redirect destination.
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
		if (!contentType.isEmpty()) {
			request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
		}
		if (authenticated) {
			request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
			if (provider == "twitch") {
				request.setRawHeader("Client-Id", activeClient.toUtf8());
			}
		}
		auto *reply = network.sendCustomRequest(request, method, body);
		reply->setReadBufferSize(2 * 1024 * 1024);
		const auto requestGeneration = generation;
		connect(reply, &QNetworkReply::readyRead, this, [reply]() {
			if (reply->bytesAvailable() > 1024 * 1024) {
				reply->abort();
			}
		});
		connect(reply, &QNetworkReply::finished, this, [this, reply, requestGeneration, done]() {
			const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			const auto data = QJsonDocument::fromJson(reply->readAll()).object();
			const bool transportError = reply->error() != QNetworkReply::NoError && code < 400;
			reply->deleteLater();
			if (generation == requestGeneration) {
				done(transportError ? 0 : code, data);
			}
		});
	}

	void PostForm(const QString &url, const QList<QPair<QString, QString>> &values, Completion done)
	{
		Request(url, "POST", Form(values), "application/x-www-form-urlencoded", false, std::move(done));
	}

	void SetConnected(bool connected)
	{
		client->setEnabled(!connected);
		connectButton->setEnabled(!connected);
		updateButton->setEnabled(connected && !updatePending);
		status->setText(Text(connected ? "Connected" : "Disconnected"));
	}

	void BeginUpdate()
	{
		updatePending = true;
		updateButton->setEnabled(false);
	}

	void FinishUpdate()
	{
		updatePending = false;
		updateButton->setEnabled(!accessToken.isEmpty());
	}

	virtual void Reset()
	{
		++generation;
		refreshTimer.stop();
		accessToken.clear();
		refreshToken.clear();
		activeClient.clear();
		updatePending = false;
		for (auto *reply : network.findChildren<QNetworkReply *>()) {
			reply->abort();
		}
		SetConnected(false);
	}

	bool Begin()
	{
		Reset();
		activeClient = client->text().trimmed();
		if (activeClient.isEmpty()) {
			status->setText(Text("ClientRequired"));
			return false;
		}
		QSettings settings(configPath, QSettings::IniFormat);
		settings.setValue(provider + "/clientId", activeClient);
		client->setEnabled(false);
		connectButton->setEnabled(false);
		status->setText(Text("Connecting"));
		return true;
	}

	void Fail(const char *key = "Failed")
	{
		Reset();
		status->setText(Text(key));
	}

	bool AcceptTokens(const QJsonObject &data)
	{
		const QString token = data.value("access_token").toString();
		const int lifetime = data.value("expires_in").toInt();
		if (token.isEmpty() || lifetime <= 0) {
			Fail();
			return false;
		}
		accessToken = token;
		if (!data.value("refresh_token").toString().isEmpty()) {
			refreshToken = data.value("refresh_token").toString();
		}
		refreshTimer.start(std::clamp(lifetime - 60, 1, 86400) * 1000);
		return true;
	}

	void FinishForm()
	{
		form->addRow(Text("Title"), title);
		form->addRow(updateButton);
		status->setWordWrap(true);
		status->setObjectName(QStringLiteral("penguinPlatformStatus"));
		status->setTextFormat(Qt::PlainText);
		status->setTextInteractionFlags(Qt::TextSelectableByMouse);
		form->addRow(status);
		credentialStatus->setWordWrap(true);
		credentialStatus->setObjectName("penguinCredentialStatus");
		form->addRow(credentialStatus);
		for (int row = 0; row < form->rowCount(); ++row) {
			auto *item = form->itemAt(row, QFormLayout::LabelRole);
			if (auto *label = item ? qobject_cast<QLabel *>(item->widget()) : nullptr) {
				label->setWordWrap(true);
				label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			}
		}
		SetConnected(false);
	}

public:
	// The optional transport is borrowed, and must outlive this pane (used by offline controller tests).
	PlatformPane(QString name, QWidget *parent, QNetworkAccessManager *transport = nullptr,
		     CredentialStore *store = nullptr)
		: QWidget(parent),
		  network(transport ? *transport : ownedNetwork),
		  provider(std::move(name))
	{
		setObjectName(QStringLiteral("penguinPlatformPane"));
		form->setContentsMargins(12, 14, 12, 14);
		form->setVerticalSpacing(10);
		form->setRowWrapPolicy(QFormLayout::WrapAllRows);
		// Full-width label rows wrap only when the dock actually runs out of space.
		form->setLabelAlignment({});
		form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
		form->setFormAlignment(Qt::AlignTop);
		connectButton->setObjectName(provider + "ConnectAccount");
		connectButton->setProperty("penguinPrimary", true);
		updateButton->setProperty("penguinPrimary", true);
		char *path = obs_module_config_path("platforms.ini");
		configPath = QString::fromUtf8(path);
		bfree(path);
		QDir().mkpath(QFileInfo(configPath).absolutePath());
		credentials = store;
		if (!credentials && !transport) {
			credentials = new KeychainStore(
				provider + "/" +
					QString::fromLatin1(QCryptographicHash::hash(
								    QFileInfo(configPath).absoluteFilePath().toUtf8(),
								    QCryptographicHash::Sha256)
								    .toHex()),
				this);
		}
		QSettings settings(configPath, QSettings::IniFormat);
		client->setText(settings.value(provider + "/clientId").toString());
		form->addRow(Text("AppID"), client);
		form->addRow(connectButton);
		form->addRow(disconnectButton);
		refreshTimer.setSingleShot(true);
		connect(disconnectButton, &QPushButton::clicked, this, [this]() { ForgetSession(); });
	}

	~PlatformPane() override
	{
		++generation;
		for (auto *reply : network.findChildren<QNetworkReply *>()) {
			disconnect(reply, nullptr, this, nullptr);
			reply->abort();
		}
	}
};

class TwitchPane final : public PlatformPane {
	QTimer pollTimer{this};
	QTimer validateTimer{this};
	QString deviceCode;
	QString broadcaster;
	qint64 deadline = 0;
	int pollInterval = 5000;
	CategoryPicker *category = new CategoryPicker(this);
	QTimer categorySearch{this};
	quint64 categoryRevision = 0;
	QPointer<TwitchChat> chat;
	QPushButton *loadDetails = nullptr;
	static constexpr const char *scopes =
		"channel:manage:broadcast user:read:chat user:write:chat moderator:read:chatters moderator:manage:chat_messages moderator:manage:banned_users moderator:manage:chat_settings";

	void Reset() override
	{
		if (loadDetails) {
			loadDetails->setEnabled(false);
		}
		if (chat) {
			chat->Stop();
		}
		categorySearch.stop();
		++categoryRevision;
		category->clear();
		category->setEnabled(false);
		pollTimer.stop();
		validateTimer.stop();
		deviceCode.clear();
		broadcaster.clear();
		PlatformPane::Reset();
	}

	void Validate()
	{
		// Twitch's validation endpoint accepts Bearer authentication as well as OAuth.
		Request("https://id.twitch.tv/oauth2/validate", "GET", {}, {}, true,
			[this](int code, const QJsonObject &data) {
				if (code != 200 || data.value("client_id").toString() != activeClient ||
				    data.value("user_id").toString().isEmpty() ||
				    !data.value("scopes").toArray().contains("channel:manage:broadcast") ||
				    !data.value("scopes").toArray().contains("user:read:chat") ||
				    !data.value("scopes").toArray().contains("user:write:chat")) {
					Fail();
					return;
				}
				broadcaster = data.value("user_id").toString();
				SetConnected(true);
				category->setEnabled(true);
				SaveSession();
				if (loadDetails) {
					loadDetails->setEnabled(true);
				}
				if (chat) {
					chat->SetSession(activeClient, accessToken, broadcaster);
				}
				validateTimer.start(55 * 60 * 1000);
			});
	}

	void Poll()
	{
		if (QDateTime::currentMSecsSinceEpoch() >= deadline) {
			Fail("Expired");
			return;
		}
		PostForm("https://id.twitch.tv/oauth2/token",
			 {{"client_id", activeClient},
			  {"scopes", scopes},
			  {"device_code", deviceCode},
			  {"grant_type", "urn:ietf:params:oauth:grant-type:device_code"}},
			 [this](int code, const QJsonObject &data) {
				 if (code == 200) {
					 deviceCode.clear();
					 if (AcceptTokens(data)) {
						 Validate();
					 }
					 return;
				 }
				 const QString error = data.value("error").toString(data.value("message").toString());
				 if (error == "slow_down") {
					 pollInterval += 5000;
				 }
				 if (error == "authorization_pending" || error == "slow_down") {
					 pollTimer.start(pollInterval);
				 } else {
					 Fail("Expired");
				 }
			 });
	}

public:
	explicit TwitchPane(QWidget *parent, QNetworkAccessManager *transport = nullptr, TwitchChat *chatPane = nullptr,
			    CredentialStore *store = nullptr)
		: PlatformPane("twitch", parent, transport, store),
		  chat(chatPane)
	{
		client->setText(QStringLiteral("wx372widbw00vu69va6dhedawepy9x"));
		client->setVisible(false);
		if (auto *label = form->labelForField(client)) {
			label->hide();
		}
		connectButton->setText(Text("Twitch.Connect"));
		form->addRow(Text("Twitch.Category"), category);
		category->setObjectName("twitchCategory");
		category->setEnabled(false);
		category->lineEdit()->setPlaceholderText(Text("Category.Search"));
		categorySearch.setSingleShot(true);
		categorySearch.setInterval(300);
		connect(category->lineEdit(), &QLineEdit::textEdited, this, [this]() {
			++categoryRevision;
			categorySearch.start();
		});
		connect(&categorySearch, &QTimer::timeout, this, [this]() {
			const auto query = category->currentText().trimmed();
			const auto revision = categoryRevision;
			if (query.isEmpty() || accessToken.isEmpty()) {
				return;
			}
			Request("https://api.twitch.tv/helix/search/categories?first=20&query=" +
					QString::fromLatin1(QUrl::toPercentEncoding(query)),
				"GET", {}, {}, true, [this, query, revision](int code, const QJsonObject &data) {
					if (revision != categoryRevision ||
					    query != category->currentText().trimmed()) {
						return;
					}
					if (code != 200) {
						status->setText(Text("Category.Failed"));
						return;
					}
					QList<QPair<QString, QString>> choices;
					for (const auto &entry : data.value("data").toArray()) {
						const auto item = entry.toObject();
						if (!item.value("id").toString().isEmpty()) {
							choices.append({item.value("name").toString(),
									item.value("id").toString()});
						}
					}
					category->Choices(choices, true);
				});
		});
		FinishForm();
		auto *load = new QPushButton(Text("Twitch.Load"), this);
		loadDetails = load;
		load->setEnabled(false);
		form->insertRow(form->rowCount() - 1, load);
		connect(load, &QPushButton::clicked, this, [this, load]() {
			if (broadcaster.isEmpty() || accessToken.isEmpty() || updatePending) {
				return;
			}
			load->setEnabled(false);
			const auto oldTitle = title->text();
			const auto oldCategory = category->currentText();
			Request("https://api.twitch.tv/helix/channels?broadcaster_id=" + broadcaster, "GET", {}, {},
				true, [this, load, oldTitle, oldCategory](int code, const QJsonObject &data) {
					load->setEnabled(true);
					const auto items = data.value("data").toArray();
					if (code != 200 || items.isEmpty()) {
						status->setText(Text("Failed"));
						return;
					}
					const auto info = items.first().toObject();
					// Do not replace text the user edited while the request was in flight.
					if (title->text() == oldTitle) {
						title->setText(info.value("title").toString());
					}
					if (category->currentText() == oldCategory) {
						category->Select(info.value("game_id").toString(),
								 info.value("game_name").toString());
					}
				});
		});
		pollTimer.setSingleShot(true);
		connect(&pollTimer, &QTimer::timeout, this, [this]() { Poll(); });
		connect(&validateTimer, &QTimer::timeout, this, [this]() { Validate(); });
		connect(connectButton, &QPushButton::clicked, this, [this]() {
			if (!Begin()) {
				return;
			}
			PostForm("https://id.twitch.tv/oauth2/device",
				 {{"client_id", activeClient}, {"scopes", scopes}},
				 [this](int code, const QJsonObject &data) {
					 const QUrl verification(data.value("verification_uri").toString());
					 if (code != 200 || verification.scheme() != "https" ||
					     verification.host() != "www.twitch.tv" ||
					     data.value("device_code").toString().isEmpty() ||
					     data.value("expires_in").toInt() <= 0) {
						 Fail();
						 return;
					 }
					 deviceCode = data.value("device_code").toString();
					 deadline = QDateTime::currentMSecsSinceEpoch() +
						    qint64(data.value("expires_in").toInt()) * 1000;
					 pollInterval = std::clamp(data.value("interval").toInt(5), 1, 60) * 1000;
					 status->setText(Text("Twitch.Device").arg(data.value("user_code").toString()));
					 if (!QDesktopServices::openUrl(verification)) {
						 Fail();
						 return;
					 }
					 pollTimer.start(pollInterval);
				 });
		});
		connect(&refreshTimer, &QTimer::timeout, this, [this]() {
			if (refreshToken.isEmpty()) {
				Fail("Expired");
				return;
			}
			PostForm("https://id.twitch.tv/oauth2/token",
				 {{"client_id", activeClient},
				  {"refresh_token", refreshToken},
				  {"grant_type", "refresh_token"}},
				 [this](int code, const QJsonObject &data) {
					 if (code == 200 && AcceptTokens(data)) {
						 Validate();
					 } else {
						 Fail("Expired");
					 }
				 });
		});
		connect(updateButton, &QPushButton::clicked, this, [this]() {
			const auto value = title->text().trimmed();
			if (value.isEmpty() || value.toUcs4().size() > 140) {
				status->setText(Text(value.isEmpty() ? "TitleRequired" : "Twitch.TitleLength"));
				return;
			}
			QJsonObject body{{"title", value}};
			if (!category->Valid()) {
				status->setText(Text("Category.Select"));
				return;
			}
			if (!category->Id().isEmpty()) {
				body.insert("game_id", category->Id());
			}
			BeginUpdate();
			Request("https://api.twitch.tv/helix/channels?broadcaster_id=" + broadcaster, "PATCH",
				QJsonDocument(body).toJson(QJsonDocument::Compact), "application/json", true,
				[this](int code, const QJsonObject &) {
					if (code == 401) {
						Fail("Expired");
						return;
					}
					FinishUpdate();
					status->setText(Text(code == 204 ? "Updated" : "Failed"));
				});
		});
		QTimer::singleShot(0, this, [this]() { RestoreSession(); });
	}
};

class YoutubePane final : public PlatformPane {
	QPointer<YoutubeChat> chat;
	QTcpServer listener{this};
	QTimer authorizationTimeout{this};
	QLineEdit *secret = new QLineEdit(this);
	QComboBox *broadcasts = new QComboBox(this);
	QPushButton *loadButton = new QPushButton(Text("YouTube.Refresh"), this);
	QString state;
	QString verifier;
	QString redirect;
	QString activeSecret;
	quint64 broadcastLoad = 0;
	CategoryPicker *category = new CategoryPicker(this);
	QString SessionSecret() const override { return activeSecret; }
	void RestoreSecret(const QString &value) override
	{
		activeSecret = value;
		secret->setText(value);
		secret->setEnabled(false);
	}
	void LoadCategories()
	{
		auto region = QLocale().name().section('_', 1, 1);
		if (region.size() != 2) {
			region = "US";
		}
		Request("https://www.googleapis.com/youtube/v3/videoCategories?part=snippet&regionCode=" + region,
			"GET", {}, {}, true, [this](int code, const QJsonObject &data) {
				if (code != 200) {
					status->setText(Text("Category.Failed"));
					return;
				}
				QList<QPair<QString, QString>> choices;
				for (const auto &entry : data.value("items").toArray()) {
					const auto item = entry.toObject();
					const auto snippet = item.value("snippet").toObject();
					if (snippet.value("assignable").toBool()) {
						choices.append({snippet.value("title").toString(),
								item.value("id").toString()});
					}
				}
				category->Choices(choices, false);
				category->setEnabled(true);
			});
	}
	QList<QPair<QString, QString>> pendingBroadcasts;
	QSet<QString> pageTokens;

	void Reset() override
	{
		if (chat) {
			chat->Stop();
		}
		category->clear();
		category->setEnabled(false);
		++broadcastLoad;
		pendingBroadcasts.clear();
		pageTokens.clear();
		listener.close();
		authorizationTimeout.stop();
		for (auto *socket : listener.findChildren<QTcpSocket *>()) {
			disconnect(socket, nullptr, this, nullptr);
			socket->abort();
			socket->deleteLater();
		}
		state.clear();
		verifier.clear();
		activeSecret.clear();
		secret->setEnabled(true);
		broadcasts->clear();
		loadButton->setEnabled(false);
		PlatformPane::Reset();
	}

	void Token(const QList<QPair<QString, QString>> &parameters)
	{
		auto values = parameters;
		values.append({"client_id", activeClient});
		if (!activeSecret.isEmpty()) {
			values.append({"client_secret", activeSecret});
		}
		PostForm("https://oauth2.googleapis.com/token", values, [this](int code, const QJsonObject &data) {
			if (code != 200 || !AcceptTokens(data)) {
				Fail("Expired");
				return;
			}
			SetConnected(true);
			if (chat) {
				chat->SetSession(accessToken, activeClient);
			}
			SaveSession();
			if (category->count() == 0) {
				LoadCategories();
			}
			loadButton->setEnabled(true);
			LoadBroadcasts();
		});
	}

	void LoadBroadcastPage(const QString &pageToken, quint64 load, const QString &editedTitle)
	{
		QString url =
			"https://www.googleapis.com/youtube/v3/liveBroadcasts?part=id,snippet&mine=true&broadcastType=all&maxResults=50";
		if (!pageToken.isEmpty()) {
			url += "&pageToken=" + QString::fromLatin1(QUrl::toPercentEncoding(pageToken));
		}
		Request(url, "GET", {}, {}, true, [this, load, editedTitle](int code, const QJsonObject &data) {
			if (load != broadcastLoad) {
				return;
			}
			if (code != 200) {
				loadButton->setEnabled(true);
				if (code == 401) {
					Fail("Expired");
				} else {
					status->setText(Text("Failed"));
				}
				return;
			}
			for (const auto &entry : data.value("items").toArray()) {
				const auto item = entry.toObject();
				pendingBroadcasts.append({item.value("snippet").toObject().value("title").toString(),
							  item.value("id").toString()});
			}
			const auto next = data.value("nextPageToken").toString();
			if (!next.isEmpty()) {
				if (pageTokens.contains(next)) {
					loadButton->setEnabled(true);
					status->setText(Text("Failed"));
					return;
				}
				pageTokens.insert(next);
				LoadBroadcastPage(next, load, editedTitle);
				return;
			}
			// Avoid resetting the selected broadcast or an edited title on token refresh.
			const auto selectedId = broadcasts->currentData().toString();
			QSignalBlocker block(broadcasts);
			broadcasts->clear();
			for (const auto &item : pendingBroadcasts) {
				broadcasts->addItem(item.first, item.second);
			}
			const int previous = broadcasts->findData(selectedId);
			if (previous >= 0) {
				broadcasts->setCurrentIndex(previous);
				// Preserve edits made while the request was in flight as well.
			} else if (title->text() == editedTitle) {
				title->setText(broadcasts->currentText());
			}
			pendingBroadcasts.clear();
			pageTokens.clear();
			loadButton->setEnabled(true);
			if (!broadcasts->count()) {
				status->setText(Text("YouTube.NoBroadcast"));
			}
		});
	}

	void LoadBroadcasts()
	{
		loadButton->setEnabled(false);
		pendingBroadcasts.clear();
		pageTokens.clear();
		LoadBroadcastPage({}, ++broadcastLoad, title->text());
	}

	void AcceptConnection()
	{
		while (listener.hasPendingConnections()) {
			auto *socket = listener.nextPendingConnection();
			if (!socket) {
				continue;
			}
			socket->setReadBufferSize(8192);
			const auto attempt = generation;
			connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
			QTimer::singleShot(5000, socket, [socket]() { socket->abort(); });
			connect(socket, &QTcpSocket::readyRead, this, [this, socket, attempt]() {
				if (attempt != generation || !listener.isListening()) {
					socket->abort();
					return;
				}
				if (!socket->canReadLine()) {
					if (socket->bytesAvailable() >= 8192) {
						socket->abort();
					}
					return;
				}
				const auto callback = PenguinOAuth::ParseCallback(socket->readLine(8192), state);
				if (callback.status == PenguinOAuth::CallbackStatus::Invalid) {
					socket->write(
						"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
					socket->disconnectFromHost();
					return;
				}
				const QByteArray body = Text("YouTube.Return").toUtf8();
				socket->write(
					"HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: " +
					QByteArray::number(body.size()) + "\r\n\r\n" + body);
				socket->disconnectFromHost();
				listener.close();
				authorizationTimeout.stop();
				state.clear();
				if (callback.status == PenguinOAuth::CallbackStatus::Denied) {
					Fail("Expired");
					return;
				}
				Token({{"code", callback.code},
				       {"code_verifier", verifier},
				       {"redirect_uri", redirect},
				       {"grant_type", "authorization_code"}});
				verifier.clear();
			});
		}
	}

public:
	explicit YoutubePane(QWidget *parent, QNetworkAccessManager *transport = nullptr,
			     CredentialStore *store = nullptr, YoutubeChat *chatPane = nullptr)
		: PlatformPane("youtube", parent, transport, store),
		  chat(chatPane)
	{
		// Preserve the client associated with an existing saved sign-in.
		if (client->text().trimmed().isEmpty()) {
			client->setText(QStringLiteral(PENGUIN_YOUTUBE_CLIENT_ID));
		}
		if (client->text().trimmed() == QStringLiteral(PENGUIN_YOUTUBE_CLIENT_ID)) {
			secret->setText(QStringLiteral(PENGUIN_YOUTUBE_CLIENT_SECRET));
		}
		connectButton->setText(Text("YouTube.Connect"));
		connectButton->setObjectName("youtubeConnectAccount");
		secret->setEchoMode(QLineEdit::Password);
		form->addRow(Text("ClientSecret"), secret);
		auto *appSettings = new QPushButton(Text("YouTube.AppSettings"), this);
		appSettings->setObjectName("youtubeAppSettings");
		appSettings->setCheckable(true);
		form->addRow(appSettings);
		const auto showAppSettings = [this](bool visible) {
			form->setRowVisible(client, visible);
			form->setRowVisible(secret, visible);
		};
		showAppSettings(false);
		connect(appSettings, &QPushButton::toggled, this, showAppSettings);
		form->addRow(Text("YouTube.Broadcast"), broadcasts);
		form->addRow(loadButton);
		category->setObjectName("youtubeCategory");
		category->setEnabled(false);
		category->lineEdit()->setPlaceholderText(Text("Category.Search"));
		form->addRow(Text("YouTube.Category"), category);
		auto *categoryNote = new QLabel(Text("YouTube.CategoryNote"), this);
		categoryNote->setWordWrap(true);
		form->addRow(categoryNote);
		loadButton->setEnabled(false);
		FinishForm();
		authorizationTimeout.setSingleShot(true);
		connect(&authorizationTimeout, &QTimer::timeout, this, [this]() { Fail("Expired"); });
		connect(&listener, &QTcpServer::newConnection, this, [this]() { AcceptConnection(); });
		connect(loadButton, &QPushButton::clicked, this, [this]() { LoadBroadcasts(); });
		connect(broadcasts, &QComboBox::currentTextChanged, title, &QLineEdit::setText);
		connect(connectButton, &QPushButton::clicked, this, [this]() {
			if (client->text().trimmed().isEmpty()) {
				status->setText(Text("YouTube.AppUnavailable"));
				return;
			}
			if (!Begin()) {
				return;
			}
			if (!listener.listen(QHostAddress::LocalHost, 0)) {
				Fail("YouTube.Callback");
				return;
			}
			activeSecret = secret->text().trimmed();
			secret->setEnabled(false);
			state = RandomToken();
			verifier = RandomToken();
			redirect = QString("http://127.0.0.1:%1/oauth/callback").arg(listener.serverPort());
			const auto challenge = PenguinOAuth::Challenge(verifier);
			QUrl url("https://accounts.google.com/o/oauth2/v2/auth");
			QUrlQuery query;
			query.addQueryItem("client_id", activeClient);
			query.addQueryItem("redirect_uri", redirect);
			query.addQueryItem("response_type", "code");
			query.addQueryItem("scope", "https://www.googleapis.com/auth/youtube.force-ssl");
			query.addQueryItem("access_type", "offline");
			query.addQueryItem("prompt", "consent");
			query.addQueryItem("state", state);
			query.addQueryItem("code_challenge", QString::fromLatin1(challenge));
			query.addQueryItem("code_challenge_method", "S256");
			url.setQuery(query);
			if (!QDesktopServices::openUrl(url)) {
				Fail();
				return;
			}
			authorizationTimeout.start(5 * 60 * 1000);
		});
		connect(&refreshTimer, &QTimer::timeout, this, [this]() {
			if (refreshToken.isEmpty()) {
				Fail("Expired");
			} else {
				Token({{"refresh_token", refreshToken}, {"grant_type", "refresh_token"}});
			}
		});
		connect(updateButton, &QPushButton::clicked, this, [this]() {
			const QString value = title->text().trimmed();
			if (value.isEmpty() || value.toUcs4().size() > 100) {
				status->setText(Text(value.isEmpty() ? "TitleRequired" : "YouTube.TitleLength"));
				return;
			}
			if (!category->Valid()) {
				status->setText(Text("Category.Select"));
				return;
			}
			const auto categoryId = category->Id();
			const auto id = broadcasts->currentData().toString();
			if (id.isEmpty()) {
				status->setText(Text("YouTube.NoBroadcast"));
				return;
			}
			BeginUpdate();
			Request("https://www.googleapis.com/youtube/v3/videos?part=snippet&id=" +
					QString::fromLatin1(QUrl::toPercentEncoding(id)),
				"GET", {}, {}, true, [this, id, value, categoryId](int code, const QJsonObject &data) {
					if (code == 401) {
						Fail("Expired");
						return;
					}
					const auto items = data.value("items").toArray();
					if (code != 200 || items.isEmpty()) {
						FinishUpdate();
						status->setText(Text("Failed"));
						return;
					}
					const auto original = items[0].toObject().value("snippet").toObject();
					// videos.update replaces mutable snippet fields; preserve all of them.
					QJsonObject snippet{{"title", value}};
					for (const auto *key : {"categoryId", "description", "tags", "defaultLanguage",
								"defaultAudioLanguage"}) {
						if (original.contains(key)) {
							snippet.insert(key, original.value(key));
						}
					}
					if (!categoryId.isEmpty()) {
						snippet.insert("categoryId", categoryId);
					}
					if (!snippet.contains("categoryId")) {
						FinishUpdate();
						status->setText(Text("Failed"));
						return;
					}
					Request("https://www.googleapis.com/youtube/v3/videos?part=snippet", "PUT",
						QJsonDocument(QJsonObject{{"id", id}, {"snippet", snippet}})
							.toJson(QJsonDocument::Compact),
						"application/json", true,
						[this, id, value](int result, const QJsonObject &) {
							if (result == 401) {
								Fail("Expired");
								return;
							}
							if (result == 200) {
								const int index = broadcasts->findData(id);
								if (index >= 0) {
									QSignalBlocker block(broadcasts);
									broadcasts->setItemText(index, value);
								}
							}
							FinishUpdate();
							status->setText(Text(result == 200 ? "Updated" : "Failed"));
						});
				});
		});
		QTimer::singleShot(0, this, [this]() { RestoreSession(); });
	}
};

constexpr char DockId[] = "penguin-stream-details";
QPointer<QWidget> dock;
QPointer<TwitchChat> chatDock;
QPointer<YoutubeChat> youtubeChatDock;
constexpr const char *ChatDockId = "penguinTwitchChat";
bool frontendShutdown = false;

void FrontendEvent(enum obs_frontend_event event, void *)
{
	if (event != OBS_FRONTEND_EVENT_EXIT) {
		return;
	}
	// Dock state has already been saved. Dispose callbacks while the API still exists.
	frontendShutdown = true;
	if (chatDock) {
		chatDock->Stop();
		if (youtubeChatDock) {
			youtubeChatDock->Stop();
		}
		obs_frontend_remove_dock(ChatDockId);
	}
	if (dock) {
		obs_frontend_remove_dock(DockId);
	}
}

void CreateDock()
{
	auto *parent = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!parent || dock) {
		return;
	}
	dock = new QWidget(parent);
	dock->setObjectName(QStringLiteral("penguinPlatformPanel"));
	auto *layout = new QVBoxLayout(dock);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(12);
	auto *intro = new QLabel(Text("Dock.Intro"), dock);
	intro->setProperty("penguinHint", true);
	intro->setWordWrap(true);
	layout->addWidget(intro);
	auto *tabs = new QTabWidget(dock);
	tabs->setObjectName(QStringLiteral("penguinPlatformTabs"));
	tabs->setDocumentMode(true);
	const auto addPlatform = [tabs](PlatformPane *pane, const QString &name) {
		auto *scroll = new QScrollArea(tabs);
		scroll->setObjectName(QStringLiteral("penguinPlatformScroll"));
		scroll->setFrameShape(QFrame::NoFrame);
		scroll->setWidgetResizable(true);
		scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scroll->setWidget(pane);
		tabs->addTab(scroll, name);
	};
	auto *chatTabs = new QTabWidget(parent);
	chatTabs->setDocumentMode(true);
	chatDock = new TwitchChat(chatTabs, Text);
	youtubeChatDock = new YoutubeChat(chatTabs);
	chatTabs->addTab(chatDock, "Twitch");
	chatTabs->addTab(youtubeChatDock, "YouTube");
	auto *dashboard = new QPushButton("Dashboard ↗", chatTabs);
	dashboard->setToolTip("Open Twitch or YouTube for platform features not available through their APIs.");
	chatTabs->setCornerWidget(dashboard);
	QObject::connect(dashboard, &QPushButton::clicked, chatTabs, [chatTabs] {
		QDesktopServices::openUrl(QUrl(chatTabs->currentIndex() == 0 ? "https://dashboard.twitch.tv/"
									     : "https://studio.youtube.com/"));
	});
	if (!obs_frontend_add_dock_by_id(ChatDockId, obs_module_text("Chat.Title"), chatTabs)) {
		delete chatTabs;
		chatDock = nullptr;
	}
	addPlatform(new TwitchPane(tabs, nullptr, chatDock), QStringLiteral("Twitch"));
	addPlatform(new YoutubePane(tabs, nullptr, nullptr, youtubeChatDock), QStringLiteral("YouTube"));
	layout->addWidget(tabs, 1);
	auto *accountAction = new QAction(parent);
	accountAction->setObjectName("penguinConnectAccountAction");
	QObject::connect(accountAction, &QAction::triggered, dock, [tabs, accountAction, parent]() {
		const QString provider = accountAction->data().toString();
		const int index = provider == "twitch" ? 0 : provider == "youtube" ? 1 : -1;
		if (index < 0 || tabs->count() != 2) {
			return;
		}
		QPointer<QTabWidget> original = tabs;
		QPointer<QWidget> page = tabs->widget(index);
		const auto label = tabs->tabText(index);
		tabs->removeTab(index);
		auto *owner = QApplication::activeModalWidget();
		auto *dialog = new QDialog(owner ? owner : parent);
		dialog->setAttribute(Qt::WA_DeleteOnClose);
		dialog->setWindowTitle(label);
		dialog->resize(500, 620);
		auto *box = new QVBoxLayout(dialog);
		box->addWidget(page);
		page->show();
		auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
		box->addWidget(buttons);
		QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
		const auto restore = [original, page, index, label]() {
			if (original && page && original->indexOf(page) < 0) {
				original->insertTab(index, page, label);
				original->setCurrentIndex(index);
			}
		};
		QObject::connect(dialog, &QDialog::finished, parent, restore);
		QObject::connect(dialog, &QObject::destroyed, parent, restore);
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		if (provider == "twitch") {
			if (auto *button = page->findChild<QPushButton *>("twitchConnectAccount");
			    button && button->isEnabled()) {
				button->click();
			}
		}
	});
	auto *note = new QLabel(Text("SessionNoteShort"), dock);
	note->setProperty("penguinHint", true);
	note->setToolTip(Text("SessionNote"));
	note->setAccessibleDescription(Text("SessionNote"));
	note->setWordWrap(true);
	layout->addWidget(note);
	if (!obs_frontend_add_dock_by_id(DockId, obs_module_text("Dock.Title"), dock.data())) {
		delete dock.data();
		dock = nullptr;
	}
}
} // namespace

bool obs_module_load(void)
{
	frontendShutdown = false;
	return true;
}

void obs_module_post_load(void)
{
	// Register before OBS restores saved QMainWindow dock positions.
	CreateDock();
	if (dock) {
		obs_frontend_add_event_callback(FrontendEvent, nullptr);
	}
}

void obs_module_unload(void)
{
	// OBS destroys the frontend API immediately after its EXIT event.
	if (frontendShutdown) {
		return;
	}
	obs_frontend_remove_event_callback(FrontendEvent, nullptr);
	if (chatDock) {
		chatDock->Stop();
		if (youtubeChatDock) {
			youtubeChatDock->Stop();
		}
		obs_frontend_remove_dock(ChatDockId);
	}
	if (dock) {
		obs_frontend_remove_dock(DockId);
	}
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Penguin Studio native Twitch and YouTube stream metadata controls";
}
