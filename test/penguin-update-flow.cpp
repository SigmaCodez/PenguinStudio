// SPDX-License-Identifier: GPL-2.0-or-later
// Production updater, simulated HTTP responses: never executes a package manager.
#include "../frontend/utility/PenguinLinuxUpdater.cpp"
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {
config_t *configuration = nullptr;
bool outputActive = false;
void Require(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}
class Reply final : public QNetworkReply {
	QByteArray data;
	qint64 position = 0;
	qint64 readData(char *out, qint64 length) override
	{
		const auto count = std::min(length, data.size() - position);
		if (count <= 0) {
			return -1;
		}
		std::memcpy(out, data.constData() + position, size_t(count));
		position += count;
		return count;
	}

public:
	Reply(const QNetworkRequest &request, QObject *parent) : QNetworkReply(parent)
	{
		setRequest(request);
		setUrl(request.url());
		open(QIODevice::ReadOnly | QIODevice::Unbuffered);
	}
	qint64 bytesAvailable() const override { return data.size() - position + QNetworkReply::bytesAvailable(); }
	void Complete(int code, const QByteArray &body)
	{
		setAttribute(QNetworkRequest::HttpStatusCodeAttribute, code);
		data = body;
		emit readyRead();
		if (!isFinished()) {
			setFinished(true);
			emit finished();
		}
	}
	void abort() override
	{
		if (isFinished()) {
			return;
		}
		setError(QNetworkReply::OperationCanceledError, "Test abort");
		setFinished(true);
		emit finished();
	}
};
class Transport final : public QNetworkAccessManager {
	QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *) override
	{
		auto *reply = new Reply(request, this);
		calls.append(reply);
		return reply;
	}

public:
	QList<QPointer<Reply>> calls;
};
QPushButton *Button(Updater &dialog, const char *key)
{
	for (auto *button : dialog.findChildren<QPushButton *>()) {
		if (button->text() == key) {
			return button;
		}
	}
	Require(false, "Missing updater action");
	return nullptr;
}
bool HasStatus(Updater &dialog, const char *key)
{
	for (auto *label : dialog.findChildren<QLabel *>()) {
		if (label->text() == key) {
			return true;
		}
	}
	return false;
}
QByteArray Release()
{
	const auto name = PenguinUpdates::PackageName("99.9.9", "ubuntu26.04");
	QJsonObject asset{{"name", name},
			  {"size", 4},
			  {"state", "uploaded"},
			  {"digest", "sha256:" + QString(64, 'a')},
			  {"browser_download_url",
			   "https://github.com/SigmaCodez/PenguinStudio/releases/download/penguin-v99.9.9/" + name}};
	return QJsonDocument(QJsonArray{QJsonObject{{"tag_name", "penguin-v99.9.9"}, {"assets", QJsonArray{asset}}}})
		.toJson();
}
} // namespace
extern "C" {
config_t *obs_frontend_get_app_config()
{
	return configuration;
}
const char *obs_frontend_get_locale_string(const char *key)
{
	return std::strcmp(key, "Penguin.Update.Available") == 0 ? "Penguin Studio %1 is available." : key;
}
bool obs_frontend_virtualcam_active()
{
	return false;
}
void obs_enum_outputs(bool (*callback)(void *, obs_output_t *), void *data)
{
	callback(data, nullptr);
}
bool obs_output_active(const obs_output_t *)
{
	return outputActive;
}
}
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	QTemporaryDir root;
	qputenv("XDG_CACHE_HOME", root.path().toUtf8());
	Require(config_open(&configuration, root.filePath("app.ini").toUtf8(), CONFIG_OPEN_ALWAYS) == CONFIG_SUCCESS,
		"Could not open test configuration");
	QWidget parent;
	{
		Transport network;
		Updater dialog(&parent, &network, "ubuntu26.04");
		dialog.Check(false);
		Require(network.calls.size() == 1, "Release check missing");
		Require(network.calls[0]->request().attribute(QNetworkRequest::RedirectPolicyAttribute).toInt() ==
				QNetworkRequest::ManualRedirectPolicy,
			"Release metadata redirect allowed");
		network.calls[0]->Complete(200, Release());
		auto *install = Button(dialog, "Penguin.Update.Install");
		Require(install->isEnabled(), "Compatible update not offered");
		outputActive = true;
		install->click();
		Require(network.calls.size() == 1 && HasStatus(dialog, "Penguin.Update.StopOutputs"),
			"Update allowed during output");
		outputActive = false;
		install->click();
		Require(network.calls.size() == 2, "Download missing");
		network.calls[1]->Complete(200, "evil");
		Require(HasStatus(dialog, "Penguin.Update.DownloadFailed"), "Bad checksum did not block installation");
		Require(dialog.findChild<QProcess *>()->state() == QProcess::NotRunning,
			"Installer ran for unverified package");
		install->click();
		network.calls[2]->Complete(200, "oversized package");
		Require(HasStatus(dialog, "Penguin.Update.DownloadFailed"), "Oversized download not rejected");
		dialog.Check(false);
		network.calls[3]->Complete(200, "not json");
		Require(HasStatus(dialog, "Penguin.Update.CheckFailed") && !install->isEnabled(),
			"Malformed metadata accepted");
		dialog.Check(false);
		network.calls[4]->Complete(200, "[]");
		Require(HasStatus(dialog, "Penguin.Update.Current") && !install->isEnabled(),
			"Empty channel incorrectly offers install");
	}
	config_close(configuration);
	std::cout << "Updater HTTP errors, checksum rejection, size cap and active-output exclusion passed\n";
}
