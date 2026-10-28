// SPDX-License-Identifier: GPL-2.0-or-later
#include "PenguinLinuxUpdater.hpp"
#include "PenguinUpdatePolicy.hpp"
#include <util/config-file.h>
#include <QApplication>
#include <ui-config.h>
#include <obs-frontend-api.h>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QString Text(const char *key)
{
	return QString::fromUtf8(obs_frontend_get_locale_string(key));
}
bool OutputsActive()
{
	bool active = false;
	obs_enum_outputs(
		[](void *data, obs_output_t *output) {
			if (obs_output_active(output)) {
				*static_cast<bool *>(data) = true;
			}
			return true;
		},
		&active);
	return active || obs_frontend_virtualcam_active();
}
QString Target()
{
	// Flatpak already grants host service access in this project's manifest.
	if (QFile::exists("/.flatpak-info") && QSysInfo::currentCpuArchitecture() == "x86_64") {
		QSettings info("/.flatpak-info", QSettings::IniFormat);
		const auto path = info.value("Instance/app-path").toString();
		if (path.startsWith("/var/lib/flatpak/")) {
			return "flatpak-system";
		}
		if (path.startsWith(QDir::homePath() + "/.local/share/flatpak/")) {
			return "flatpak-user";
		}
		return {};
	}
	if (qEnvironmentVariableIsSet("SNAP") || QSysInfo::currentCpuArchitecture() != "x86_64" ||
	    QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath() != "/usr/bin/obs") {
		return {};
	}
	QSettings os("/etc/os-release", QSettings::IniFormat);
	const auto id = os.value("ID").toString();
	const auto version = os.value("VERSION_ID").toString();
	if (id == "ubuntu" && (version == "24.04" || version == "26.04" || version == "26.10")) {
		return "ubuntu" + version;
	}
	if (id == "fedora" && version == "44") {
		return "fedora44";
	}
	return {};
}
QString UpdateDirectory()
{
	if (!QFile::exists("/.flatpak-info")) {
		return QDir::tempPath() + "/penguin-update-XXXXXX";
	}
	const auto cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	QDir().mkpath(cache);
	return cache + "/update-XXXXXX";
}
class Updater final : public QDialog {
	QNetworkAccessManager ownedNetwork{this};
	QNetworkAccessManager &network;
	QPointer<QNetworkReply> reply;
	QProcess installer{this};
	QTimer notification{this};
	QTemporaryDir directory{UpdateDirectory()};
	QFile package;
	QCryptographicHash hash{QCryptographicHash::Sha256};
	QLabel *status = new QLabel(this);
	QProgressBar *progress = new QProgressBar(this);
	QPushButton *install = new QPushButton(this);
	QPushButton *check = new QPushButton(this);
	QPushButton *notes = new QPushButton(this);
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	PenguinUpdates::Release release;
	QString target;
	bool downloading = false, installing = false;
	qint64 received = 0;

	void SetStatus(const QString &message)
	{
		status->setText(message);
		status->ensurePolished();
		// Qt can retain the short initial label's height when an asynchronous
		// response adds wrapped paragraphs. Reserve the minimum-width text height.
		status->setMinimumHeight(std::max(status->fontMetrics().height() * 2, status->heightForWidth(380)));
		layout()->activate();
		adjustSize();
	}

	void Show()
	{
		show();
		raise();
	}
	void Failed(const char *key)
	{
		downloading = false;
		installing = false;
		buttons->setEnabled(true);
		setWindowModality(Qt::NonModal);
		progress->hide();
		package.close();
		package.remove();
		check->setEnabled(true);
		install->setEnabled(!release.name.isEmpty());
		SetStatus(Text(key));
	}
	void Download()
	{
		if (downloading || installing || release.name.isEmpty()) {
			return;
		}
		if (OutputsActive()) {
			SetStatus(Text("Penguin.Update.StopOutputs"));
			return;
		}
		if (!directory.isValid()) {
			Failed("Penguin.Update.DownloadFailed");
			return;
		}
		// Package bytes are public; apt's unprivileged file reader needs directory traversal.
		QFile::setPermissions(directory.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner |
								QFileDevice::ExeOwner | QFileDevice::ReadGroup |
								QFileDevice::ExeGroup | QFileDevice::ReadOther |
								QFileDevice::ExeOther);
		package.setFileName(directory.filePath(release.name));
		if (!package.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			Failed("Penguin.Update.DownloadFailed");
			return;
		}
		downloading = true;
		received = 0;
		hash.reset();
		install->setEnabled(false);
		check->setEnabled(false);
		progress->setRange(0, 100);
		progress->setValue(0);
		progress->show();
		SetStatus(Text("Penguin.Update.Downloading"));
		QNetworkRequest request(release.url);
		request.setTransferTimeout(60000);
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
				     QNetworkRequest::NoLessSafeRedirectPolicy);
		reply = network.get(request);
		auto *download = reply.data();
		download->setReadBufferSize(1024 * 1024);
		connect(download, &QNetworkReply::readyRead, this, [this, download]() {
			const auto chunk = download->readAll();
			received += chunk.size();
			if (received > release.size || package.write(chunk) != chunk.size()) {
				download->abort();
				return;
			}
			hash.addData(chunk);
			progress->setValue(int(100 * received / release.size));
		});
		connect(download, &QNetworkReply::finished, this, [this, download]() {
			const bool valid = download->error() == QNetworkReply::NoError &&
					   download->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() ==
						   200 &&
					   received == release.size &&
					   hash.result().toHex() == release.digest.toLatin1() && package.flush();
			download->deleteLater();
			reply = nullptr;
			package.close();
			downloading = false;
			if (!valid) {
				Failed("Penguin.Update.DownloadFailed");
				return;
			}
			Install();
		});
	}
	void Install()
	{
		if (OutputsActive()) {
			Failed("Penguin.Update.StopOutputs");
			return;
		}
		const QString manager = target.startsWith("ubuntu") ? "/usr/bin/apt-get" : "/usr/bin/dnf";
		if (!target.startsWith("flatpak-") && (!QFile::exists(manager) || !QFile::exists("/usr/bin/pkexec"))) {
			Failed("Penguin.Update.InstallFailed");
			return;
		}
		installing = true;
		buttons->setEnabled(false);
		setWindowModality(Qt::ApplicationModal);
		Show();
		SetStatus(Text("Penguin.Update.Installing"));
		progress->setRange(0, 0);
		// No shell, downloaded executable or privileged app process. The distro manager
		// resolves dependencies; apt is explicitly forbidden from removing packages.
		QStringList args{manager, "install", "-y"};
		if (target.startsWith("ubuntu")) {
			args << "--no-remove";
		}
		args << package.fileName();
		if (target.startsWith("flatpak-")) {
			installer.setProgram("/usr/bin/flatpak-spawn");
			installer.setArguments({"--host", "/usr/bin/flatpak", "install",
						target == "flatpak-user" ? "--user" : "--system", "--noninteractive",
						"--or-update", package.fileName()});
		} else {
			installer.setProgram("/usr/bin/pkexec");
			installer.setArguments(args);
		}
		installer.start();
	}

protected:
	bool eventFilter(QObject *object, QEvent *event) override
	{
		if (installing && event->type() == QEvent::Close) {
			static_cast<QCloseEvent *>(event)->ignore();
			Show();
			return true;
		}
		return QDialog::eventFilter(object, event);
	}
	void reject() override
	{
		if (!installing) {
			notification.stop();
			if (downloading && reply) {
				reply->abort();
			}
			// Release this dialog's focus while its Wayland surface still exists.
			// Otherwise Qt may disable text input after focus has returned to the parent.
			if (auto *focused = QApplication::focusWidget();
			    focused && (focused == this || isAncestorOf(focused))) {
				focused->clearFocus();
			}
			QDialog::reject();
			setWindowModality(Qt::NonModal);
		}
	}

public:
	explicit Updater(QWidget *parent, QNetworkAccessManager *transport = nullptr,
			 QString installationTarget = Target())
		: QDialog(parent),
		  network(transport ? *transport : ownedNetwork),
		  target(std::move(installationTarget))
	{
		setObjectName("penguinLinuxUpdater");
		setWindowTitle(Text("Penguin.Update.Title"));
		setMinimumWidth(420);
		auto *layout = new QVBoxLayout(this);
		status->setWordWrap(true);
		status->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		status->setTextFormat(Qt::PlainText);
		layout->addWidget(status);
		layout->addWidget(progress);
		progress->hide();
		auto *automatic = new QCheckBox(Text("Penguin.Update.Automatic"), this);
		auto *preview = new QCheckBox(Text("Penguin.Update.Preview"), this);
		auto *config = obs_frontend_get_app_config();
		automatic->setChecked(config_get_bool(config, "General", "EnableAutoUpdates"));
		// Penguin currently ships development previews; users can opt out explicitly.
		config_set_default_bool(config, "PenguinUpdates", "Preview", true);
		preview->setChecked(config_get_bool(config, "PenguinUpdates", "Preview"));
		connect(automatic, &QCheckBox::toggled, this, [config](bool value) {
			config_set_bool(config, "General", "EnableAutoUpdates", value);
			config_save_safe(config, "tmp", nullptr);
		});
		connect(preview, &QCheckBox::toggled, this, [this, config](bool value) {
			config_set_bool(config, "PenguinUpdates", "Preview", value);
			config_save_safe(config, "tmp", nullptr);
			if (!downloading && !installing) {
				release = {};
				install->setEnabled(false);
				Check(true);
			}
		});
		layout->addWidget(automatic);
		layout->addWidget(preview);
		check->setText(Text("Penguin.Update.Check"));
		install->setText(Text("Penguin.Update.Install"));
		notes->setText(Text("Penguin.Update.Releases"));
		install->setEnabled(false);
		install->setProperty("penguinPrimary", true);
		for (auto *button : {check, install, notes}) {
			button->setAutoDefault(false);
		}
		layout->addWidget(check);
		layout->addWidget(install);
		layout->addWidget(notes);
		layout->addWidget(buttons);
		connect(buttons, &QDialogButtonBox::rejected, this, &Updater::reject);
		connect(check, &QPushButton::clicked, this, [this]() { Check(true); });
		connect(install, &QPushButton::clicked, this, [this]() { Download(); });
		connect(notes, &QPushButton::clicked, this, [this]() {
			QDesktopServices::openUrl(release.notes.isEmpty()
							  ? QUrl("https://github.com/SigmaCodez/PenguinStudio/releases")
							  : release.notes);
		});
		connect(&installer, &QProcess::readyReadStandardOutput, this,
			[this]() { installer.readAllStandardOutput(); });
		connect(&installer, &QProcess::readyReadStandardError, this,
			[this]() { installer.readAllStandardError(); });
		connect(&installer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
			if (error == QProcess::FailedToStart) {
				Failed("Penguin.Update.InstallFailed");
			}
		});
		connect(&installer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
			[this](int code, QProcess::ExitStatus exit) {
				installing = false;
				buttons->setEnabled(true);
				setWindowModality(Qt::NonModal);
				if (code != 0 || exit != QProcess::NormalExit) {
					Failed("Penguin.Update.InstallFailed");
					return;
				}
				progress->hide();
				install->setEnabled(false);
				check->setEnabled(false);
				SetStatus(Text("Penguin.Update.Installed"));
				package.remove();
			});
		notification.setInterval(15000);
		connect(&notification, &QTimer::timeout, this, [this]() {
			if (!OutputsActive() && !QApplication::activeModalWidget() && QApplication::activeWindow()) {
				notification.stop();
				Show();
			}
		});
		parent->installEventFilter(this);
		installEventFilter(this);
	}
	void Check(bool manual)
	{
		if (manual) {
			if (auto *modal = QApplication::activeModalWidget(); modal && modal != this) {
				// Settings is modal. Show a new modal dialog above it without changing
				// ownership or closing/discarding the user's pending settings edits.
				hide();
				setWindowModality(Qt::ApplicationModal);
			}
			notification.stop();
			Show();
		}
		if (reply || downloading || installing) {
			return;
		}
		check->setEnabled(false);
		install->setEnabled(false);
		SetStatus(Text("Penguin.Update.Checking"));
		QNetworkRequest request(
			QUrl("https://api.github.com/repos/SigmaCodez/PenguinStudio/releases?per_page=30"));
		request.setRawHeader("Accept", "application/vnd.github+json");
		request.setRawHeader("User-Agent", "Penguin-Studio/" PENGUIN_VERSION);
		request.setTransferTimeout(20000);
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
		reply = network.get(request);
		auto *query = reply.data();
		connect(query, &QNetworkReply::readyRead, this, [query]() {
			if (query->bytesAvailable() > 4 * 1024 * 1024) {
				query->abort();
			}
		});
		connect(query, &QNetworkReply::finished, this, [this, query, manual]() {
			QJsonParseError error;
			const auto document = QJsonDocument::fromJson(query->readAll(), &error);
			const bool valid = query->error() == QNetworkReply::NoError &&
					   query->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
					   error.error == QJsonParseError::NoError && document.isArray();
			query->deleteLater();
			reply = nullptr;
			check->setEnabled(true);
			if (!valid) {
				SetStatus(Text("Penguin.Update.CheckFailed"));
				return;
			}
			auto *config = obs_frontend_get_app_config();
			config_set_int(config, "PenguinUpdates", "LastCheck", QDateTime::currentSecsSinceEpoch());
			config_save_safe(config, "tmp", nullptr);
			release = PenguinUpdates::Select(document.array(), PENGUIN_VERSION, target,
							 config_get_bool(config, "PenguinUpdates", "Preview"));
			if (release.version.isEmpty()) {
				SetStatus(Text("Penguin.Update.Current"));
				return;
			}
			SetStatus(Text("Penguin.Update.Available").arg(release.version) + "\n\n" +
				  Text(release.name.isEmpty() ? "Penguin.Update.External" : "Penguin.Update.Confirm"));
			install->setEnabled(!release.name.isEmpty());
			// Never steal focus during a broadcast, recording or startup wizard.
			if (manual ||
			    (!OutputsActive() && !QApplication::activeModalWidget() && QApplication::activeWindow())) {
				Show();
			} else {
				notification.start();
			}
		});
	}
};
QPointer<Updater> updater;
} // namespace

void CheckPenguinLinuxUpdates(QWidget *parent, bool manual)
{
	if (!manual) {
		auto *config = obs_frontend_get_app_config();
		if (!config_get_bool(config, "General", "EnableAutoUpdates")) {
			return;
		}

	}
	if (!updater) {
		updater = new Updater(parent);
	}
	updater->Check(manual);
}
