// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QDesktopServices>
#include <functional>
#include <QDir>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QToolButton>
#include <QUrlQuery>
#include <QVBoxLayout>
#if __has_include(<QWebEngineView>)
#include <QWebEngineDownloadRequest>
#include <QWebEngineNewWindowRequest>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>

namespace PenguinChat {
// Website cookies are deliberately separate from API credentials and from the
// user's system browser. Never inject API tokens into website URLs or cookies.
class WebsitePage final : public QWebEnginePage {
protected:
	bool acceptNavigationRequest(const QUrl &url, NavigationType, bool mainFrame) override
	{
		if (url == QUrl("about:blank")) {
			return true;
		}
		if (url.scheme() != "https") {
			return false;
		}
		if (mainFrame && url.host() == "accounts.google.com") {
			if (googleSignInRequired) {
				googleSignInRequired();
			}
			return false;
		}
		return true;
	}

public:
	std::function<void()> googleSignInRequired;
	WebsitePage(QWebEngineProfile *profile, QObject *parent) : QWebEnginePage(profile, parent)
	{
		connect(this, &QWebEnginePage::newWindowRequested, this, [this](QWebEngineNewWindowRequest &request) {
			if (request.isUserInitiated() && request.requestedUrl().scheme() == "https") {
				if (request.requestedUrl().host() == "accounts.google.com" && googleSignInRequired) {
					googleSignInRequired();
				} else {
					QDesktopServices::openUrl(request.requestedUrl());
				}
			}
		});
	}
};

class WebsiteChat final : public QWidget {
	QString provider;
	QUrl target;
	QWebEngineProfile *profile = nullptr;
	QWebEngineView *browser = nullptr;
	QStackedWidget *stack;
	QLabel *message;
	QWidget *api;
	bool original = true;
	bool signInPromptPending = false;
	QAction *integratedAction = nullptr;
	void EnsureBrowser()
	{
		if (!isVisible() || !original || target.isEmpty()) {
			return;
		}
		if (!browser) {
			// Match the OpenGL renderer already used by the broadcasting workspace.
			if (qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND")) {
				qputenv("QSG_RHI_BACKEND", "opengl");
			}
			const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
					     "/penguin-web-chat/" + provider;
			QDir().mkpath(base);
			profile = new QWebEngineProfile("penguin-chat-" + provider, this);
			profile->setPersistentStoragePath(base);
			profile->setCachePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
					      "/penguin-web-chat/" + provider);
			profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
			profile->setHttpCacheMaximumSize(32 * 1024 * 1024);
			connect(profile, &QWebEngineProfile::downloadRequested, this,
				[](QWebEngineDownloadRequest *download) { download->cancel(); });
			browser = new QWebEngineView(this);
			browser->setObjectName("penguinOriginalChat");
			auto *page = new WebsitePage(profile, browser);
			page->googleSignInRequired = [this] {
				if (signInPromptPending) return;
				signInPromptPending = true;
				QTimer::singleShot(0, this, [this] {
					QMessageBox prompt(QMessageBox::Information, "YouTube chat sign-in",
						"Signing in through your browser does not sign in this embedded website. "
						"Use integrated chat to send messages and moderate with the YouTube account "
						"connected in Stream details, or use the original chat in your browser.",
						QMessageBox::NoButton, this);
					auto *integrated = prompt.addButton("Use integrated chat", QMessageBox::AcceptRole);
					auto *external = prompt.addButton("Open chat in browser", QMessageBox::ActionRole);
					prompt.addButton(QMessageBox::Cancel);
					prompt.exec();
					if (prompt.clickedButton() == integrated) integratedAction->setChecked(true);
					else if (prompt.clickedButton() == external && !target.isEmpty()) QDesktopServices::openUrl(target);
					signInPromptPending = false;
				});
			};
			browser->setPage(page);
			browser->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
			browser->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
			browser->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, false);
			connect(browser, &QWebEngineView::loadFinished, this, [this](bool ok) {
				message->setText(
					ok ? QString()
					   : "The platform chat could not load. Retry or open it in your browser.");
				message->setVisible(!ok);
			});
			stack->addWidget(browser);
		}
		stack->setCurrentWidget(browser);
		browser->page()->setLifecycleState(QWebEnginePage::LifecycleState::Active);
		browser->page()->setVisible(true);
		if (browser->url() != target) {
			browser->setUrl(target);
		}
	}

protected:
	void showEvent(QShowEvent *event) override
	{
		QWidget::showEvent(event);
		EnsureBrowser();
	}
	void hideEvent(QHideEvent *event) override
	{
		QWidget::hideEvent(event);
		if (browser) {
			browser->page()->setVisible(false);
			browser->page()->setLifecycleState(QWebEnginePage::LifecycleState::Frozen);
		}
	}

public:
	static constexpr bool SupportsOriginal = true;
	std::function<void(bool)> useIntegrated;
	WebsiteChat(const QString &service, QWidget *integrated, QWidget *parent = nullptr)
		: QWidget(parent),
		  provider(service),
		  api(integrated)
	{
		setObjectName("penguinWebsiteChat-" + service);
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(8, 8, 8, 8);
		layout->setSpacing(6);
		auto *row = new QHBoxLayout;
		auto *label = new QLabel("Platform chat", this);
		row->addWidget(label);
		row->addStretch();
		auto *options = new QToolButton(this);
		options->setText("⋯");
		options->setAccessibleName("Chat options");
		options->setPopupMode(QToolButton::InstantPopup);
		auto *menu = new QMenu(options);
		menu->addAction("Reload chat", this, [this] {
			if (browser) {
				browser->reload();
			} else {
				EnsureBrowser();
			}
		});
		menu->addAction("Open in browser", this, [this] {
			if (!target.isEmpty()) {
				QDesktopServices::openUrl(target);
			}
		});
		auto *mode = menu->addAction("Use integrated chat");
		integratedAction = mode;
		mode->setCheckable(true);
		connect(mode, &QAction::toggled, this, [this, label](bool checked) {
			original = !checked;
			if (useIntegrated) {
				useIntegrated(checked);
			}
			label->setText(checked ? "Integrated chat" : "Platform chat");
			if (checked) {
				stack->setCurrentWidget(api);
				if (browser) {
					browser->page()->setVisible(false);
					browser->page()->setLifecycleState(QWebEnginePage::LifecycleState::Frozen);
				}
				message->hide();
			} else {
				message->setVisible(target.isEmpty());
				if (target.isEmpty()) {
					stack->setCurrentIndex(0);
				}
				EnsureBrowser();
			}
		});
		menu->addSeparator();
		menu->addAction("About chat sign-in", this, [this] {
			message->setText("Website sign-in is separate from your connected streaming account. "
					 "Google may require your normal browser. Use integrated chat to send messages "
					 "and moderate with your connected account.");
			message->show();
		});
		options->setMenu(menu);
		row->addWidget(options);
		layout->addLayout(row);
		message = new QLabel("Connect your account in Stream details to load its chat.", this);
		message->setWordWrap(true);
		message->setTextFormat(Qt::PlainText);
		layout->addWidget(message);
		stack = new QStackedWidget(this);
		stack->addWidget(new QWidget(stack));
		stack->addWidget(api);
		layout->addWidget(stack, 1);
	}
	~WebsiteChat() override
	{
		delete browser;
		delete profile;
	}
	void SetLocation(const QString &identifier)
	{
		QUrl next;
		if (!identifier.isEmpty()) {
			next = provider == "twitch"
				       ? QUrl("https://www.twitch.tv/popout/" +
					      QString::fromLatin1(QUrl::toPercentEncoding(identifier)) +
					      "/chat?popout=&darkpopout=")
				       : QUrl("https://www.youtube.com/live_chat?is_popout=1&dark_theme=1&v=" +
					      QString::fromLatin1(QUrl::toPercentEncoding(identifier)));
		}
		if (target == next) {
			return;
		}
		target = next;
		if (target.isEmpty()) {
			if (browser) {
				browser->setUrl(QUrl("about:blank"));
			}
			if (original) {
				stack->setCurrentIndex(0);
			}
			message->setText(
				"Connect your account and select a broadcast in Stream details to load its chat.");
			message->setVisible(original);
		} else {
			message->hide();
			EnsureBrowser();
		}
	}
};
} // namespace PenguinChat

#else
namespace PenguinChat {
class WebsiteChat final : public QWidget {
public:
	static constexpr bool SupportsOriginal = false;
	std::function<void(bool)> useIntegrated;
	WebsiteChat(const QString &, QWidget *api, QWidget *parent = nullptr) : QWidget(parent)
	{
		auto *layout = new QVBoxLayout(this);
		auto *hint = new QLabel(
			"This package includes integrated chat. Original platform chat requires Qt WebEngine.", this);
		hint->setWordWrap(true);
		layout->addWidget(hint);
		layout->addWidget(api, 1);
	}
	void SetLocation(const QString &) {}
};
} // namespace PenguinChat
#endif
