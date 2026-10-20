// SPDX-License-Identifier: GPL-2.0-or-later
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.hpp>

#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QPixmap>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QWizard>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("penguin-outputs", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Penguin.Description");
}

namespace {
QString Text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

void ShowMessage(QWidget *parent, QMessageBox::Icon icon, const QString &title, const QString &message)
{
	auto *dialog = new QMessageBox(icon, title, message, QMessageBox::Ok, parent);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowModality(Qt::ApplicationModal);
	dialog->show();
}

constexpr size_t MaxDestinations = 8;
constexpr char DockId[] = "penguin-broadcast-destinations";

struct Preset {
	const char *name;
	const char *server;
	const char *dashboard;
};

// Account-specific endpoints must be copied from the provider dashboard.
const Preset Presets[] = {
	{"Twitch", "rtmps://ingest.global-contribute.live-video.net/app", "https://dashboard.twitch.tv/settings/stream"},
	{"YouTube", "rtmps://a.rtmps.youtube.com:443/live2", "https://studio.youtube.com"},
	{"Kick", "", "https://dashboard.kick.com"},
	{"Facebook Live", "rtmps://rtmp-api.facebook.com:443/rtmp/", "https://www.facebook.com/live/producer"},
	{"TikTok LIVE", "", "https://www.tiktok.com/studio"},
	{"Custom RTMP(S)", "", ""},
};

enum class Layout { Landscape, PortraitCrop, PortraitFit };

struct Destination {
	QString name;
	QString platform = QStringLiteral("Twitch");
	QString server;
	QString key;
	QString encoder = QStringLiteral("obs_x264");
	Layout layout = Layout::Landscape;
	int bitrate = 6000;
};

QJsonObject SavedDestination(const Destination &destination)
{
	// Keep persistence allowlisted: stream keys must never enter the JSON document.
	return {{QStringLiteral("name"), destination.name},
		{QStringLiteral("platform"), destination.platform},
		{QStringLiteral("server"), destination.server},
		{QStringLiteral("encoder"), destination.encoder},
		{QStringLiteral("layout"), static_cast<int>(destination.layout)},
		{QStringLiteral("bitrate"), destination.bitrate}};
}

bool HasEncoder(const QString &id, const char *codec)
{
	const QByteArray bytes = id.toUtf8();
	const char *candidate = nullptr;
	for (size_t i = 0; obs_enum_encoder_types(i, &candidate); ++i) {
		if (bytes == candidate && strcmp(obs_get_encoder_codec(candidate), codec) == 0)
			return true;
	}
	return false;
}

QString Validate(const Destination &destination, bool requireKey)
{
	if (destination.name.trimmed().isEmpty())
		return Text("Penguin.InvalidName");
	const QUrl url(destination.server, QUrl::StrictMode);
	if (!url.isValid() || url.host().isEmpty() ||
	    (url.scheme() != QStringLiteral("rtmp") && url.scheme() != QStringLiteral("rtmps")) ||
	    !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment())
		return Text("Penguin.InvalidServer");
	if (requireKey && destination.key.trimmed().isEmpty())
		return Text("Penguin.InvalidKey");
	if (!HasEncoder(destination.encoder, "h264"))
		return Text("Penguin.MissingEncoder");
	return {};
}

QString ConfigPath()
{
	char *raw = obs_module_config_path("destinations.json");
	const QString result = QString::fromUtf8(raw ? raw : "");
	bfree(raw);
	return result;
}

class ConnectionWizard final : public QWizard {
	QLineEdit *name;
	QComboBox *platform;
	QLineEdit *server;
	QLineEdit *key;
	QComboBox *layout;
	QComboBox *encoder;
	QSpinBox *bitrate;

public:
	explicit ConnectionWizard(QWidget *parent, bool welcome, const Destination *existing = nullptr) : QWizard(parent)
	{
		setWindowTitle(Text(welcome ? "Penguin.Wizard" : "Penguin.Add"));
		setWizardStyle(QWizard::ModernStyle);
		const QPixmap logo(QStringLiteral(":/res/images/penguin-studio.png"));
		if (!logo.isNull())
			setPixmap(QWizard::LogoPixmap, logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		resize(660, 540);
		if (welcome) {
			auto *intro = new QWizardPage;
			intro->setTitle(Text("Penguin.Welcome"));
			auto *introLayout = new QVBoxLayout(intro);
			for (const char *text : {"Penguin.Intro", "Penguin.KeysNote", "Penguin.CapacityNote"}) {
				auto *label = new QLabel(Text(text));
				label->setWordWrap(true);
				introLayout->addWidget(label);
			}
			addPage(intro);
		}
		auto *page = new QWizardPage;
		page->setTitle(Text("Penguin.Connect"));
		auto *form = new QFormLayout(page);
		auto *connectionNote = new QLabel(Text("Penguin.ConnectionNote"));
		connectionNote->setWordWrap(true);
		form->addRow(connectionNote);
		name = new QLineEdit;
		platform = new QComboBox;
		for (const auto &preset : Presets)
			platform->addItem(QString::fromUtf8(preset.name));
		server = new QLineEdit;
		server->setPlaceholderText(QStringLiteral("rtmps://server.example/app"));
		key = new QLineEdit;
		key->setEchoMode(QLineEdit::Password);
		layout = new QComboBox;
		layout->addItems({Text("Penguin.Landscape"), Text("Penguin.PortraitCrop"), Text("Penguin.PortraitFit")});
		encoder = new QComboBox;
		const char *id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); ++i) {
			if (obs_get_encoder_type(id) == OBS_ENCODER_VIDEO && strcmp(obs_get_encoder_codec(id), "h264") == 0 &&
			    !(obs_get_encoder_caps(id) & OBS_ENCODER_CAP_DEPRECATED))
				encoder->addItem(QString::fromUtf8(obs_encoder_get_display_name(id)), QString::fromUtf8(id));
		}
		bitrate = new QSpinBox;
		bitrate->setRange(500, 50000);
		bitrate->setSuffix(QStringLiteral(" kbps"));
		bitrate->setSingleStep(500);
		bitrate->setValue(6000);
		form->addRow(Text("Penguin.Name"), name);
		form->addRow(Text("Penguin.Platform"), platform);
		form->addRow(Text("Penguin.Server"), server);
		form->addRow(Text("Penguin.Key"), key);
		form->addRow(Text("Penguin.Orientation"), layout);
		form->addRow(Text("Penguin.Encoder"), encoder);
		form->addRow(Text("Penguin.Bitrate"), bitrate);
		auto *dashboard = new QPushButton(Text("Penguin.Dashboard"));
		form->addRow(dashboard);
		auto *note = new QLabel(Text("Penguin.KeysNote"));
		note->setWordWrap(true);
		form->addRow(note);
		connect(platform, &QComboBox::currentIndexChanged, this, [this, dashboard](int index) {
			name->setText(QString::fromUtf8(Presets[index].name));
			server->setText(QString::fromUtf8(Presets[index].server));
			key->clear();
			dashboard->setEnabled(*Presets[index].dashboard != '\0');
		});
		connect(dashboard, &QPushButton::clicked, this, [this]() {
			QDesktopServices::openUrl(QUrl(QString::fromUtf8(Presets[platform->currentIndex()].dashboard)));
		});
		addPage(page);
		const Destination initial = existing ? *existing : Destination{};
		int index = platform->findText(initial.platform);
		platform->setCurrentIndex(index < 0 ? platform->count() - 1 : index);
		name->setText(existing ? initial.name : QStringLiteral("Twitch"));
		server->setText(existing ? initial.server : QString::fromUtf8(Presets[0].server));
		key->setText(initial.key);
		layout->setCurrentIndex(static_cast<int>(initial.layout));
		const int encoderIndex = encoder->findData(initial.encoder);
		encoder->setCurrentIndex(encoderIndex >= 0 ? encoderIndex : 0);
		bitrate->setValue(initial.bitrate);
		dashboard->setEnabled(*Presets[platform->currentIndex()].dashboard != '\0');
	}

	Destination Value() const
	{
		return {name->text().trimmed(), platform->currentText(), server->text().trimmed(), key->text().trimmed(),
			encoder->currentData().toString(), static_cast<Layout>(layout->currentIndex()), bitrate->value()};
	}

	bool validateCurrentPage() override
	{
		if (currentPage()->isFinalPage()) {
			const QString problem = encoder->count() ? Validate(Value(), false) : Text("Penguin.InvalidEncoder");
			if (!problem.isEmpty()) {
				ShowMessage(this, QMessageBox::Warning, Text("Penguin.Invalid"), problem);
				return false;
			}
		}
		return QWizard::validateCurrentPage();
	}
};

struct RunningOutput {
	obs_output_t *output = nullptr;
	obs_service_t *service = nullptr;
	obs_encoder_t *video = nullptr;
	obs_encoder_t *audio = nullptr;
	obs_canvas_t *canvas = nullptr;
	obs_scene_t *scene = nullptr;
	obs_sceneitem_t *programItem = nullptr;
	std::atomic<bool> started{false};
	std::atomic<bool> stopped{false};
	std::atomic<bool> reconnecting{false};
	std::atomic<int> stopCode{0};
	bool stopping = false;
	bool startRequested = false;
	Layout layout = Layout::Landscape;

	static void Started(void *data, calldata_t *)
	{
		static_cast<RunningOutput *>(data)->started.store(true);
	}
	static void Stopped(void *data, calldata_t *parameters)
	{
		auto *self = static_cast<RunningOutput *>(data);
		self->stopCode.store(static_cast<int>(calldata_int(parameters, "code")));
		self->stopped.store(true);
	}
	static void Reconnecting(void *data, calldata_t *)
	{
		static_cast<RunningOutput *>(data)->reconnecting.store(true);
	}
	static void Reconnected(void *data, calldata_t *)
	{
		static_cast<RunningOutput *>(data)->reconnecting.store(false);
	}

	void ConnectSignals()
	{
		auto *handler = obs_output_get_signal_handler(output);
		signal_handler_connect(handler, "start", Started, this);
		signal_handler_connect(handler, "stop", Stopped, this);
		signal_handler_connect(handler, "reconnect", Reconnecting, this);
		signal_handler_connect(handler, "reconnect_success", Reconnected, this);
	}

	~RunningOutput()
	{
		if (output) {
			auto *handler = obs_output_get_signal_handler(output);
			signal_handler_disconnect(handler, "start", Started, this);
			signal_handler_disconnect(handler, "stop", Stopped, this);
			signal_handler_disconnect(handler, "reconnect", Reconnecting, this);
			signal_handler_disconnect(handler, "reconnect_success", Reconnected, this);
			if (startRequested && !stopped.load())
				obs_output_force_stop(output);
			// libobs destruction joins the output's capture/network threads. Video must outlive it.
			obs_output_release(output);
		}
		obs_encoder_release(video);
		obs_encoder_release(audio);
		obs_service_release(service);
		if (canvas)
			obs_canvas_set_channel(canvas, 0, nullptr);
		obs_scene_release(scene);
		obs_canvas_release(canvas);
	}

	bool UpdateProgram()
	{
		if (!canvas)
			return true;
		OBSSourceAutoRelease source = obs_get_output_source(0);
		if (!source)
			return false;
		if (programItem && obs_sceneitem_get_source(programItem) != source) {
			obs_sceneitem_remove(programItem);
			programItem = nullptr;
		}
		if (!programItem)
			programItem = obs_scene_add(scene, source);
		if (!programItem)
			return false;
		obs_sceneitem_defer_update_begin(programItem);
		obs_sceneitem_set_alignment(programItem, OBS_ALIGN_CENTER);
		obs_sceneitem_set_bounds_alignment(programItem, OBS_ALIGN_CENTER);
		obs_sceneitem_set_bounds_type(programItem,
					      layout == Layout::PortraitFit ? OBS_BOUNDS_SCALE_INNER : OBS_BOUNDS_SCALE_OUTER);
		const vec2 bounds{1080.0f, 1920.0f};
		const vec2 position{540.0f, 960.0f};
		obs_sceneitem_set_bounds(programItem, &bounds);
		obs_sceneitem_set_pos(programItem, &position);
		obs_sceneitem_defer_update_end(programItem);
		return true;
	}

	QString Initialize(const Destination &destination)
	{
		obs_video_info info{};
		if (!obs_get_video_info(&info))
			return Text("Penguin.VideoUnavailable");
		if (info.colorspace == VIDEO_CS_2100_PQ || info.colorspace == VIDEO_CS_2100_HLG)
			return Text("Penguin.SdrRequired");
		layout = destination.layout;
		video_t *videoOutput = obs_get_video();
		const QByteArray id = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
		const QByteArray name = QByteArray("penguin-") + id;
		if (layout != Layout::Landscape) {
			info.base_width = info.output_width = 1080;
			info.base_height = info.output_height = 1920;
			info.output_format = VIDEO_FORMAT_NV12;
			canvas = obs_canvas_create_private(name.constData(), &info, DEVICE);
			if (!canvas || !(videoOutput = obs_canvas_get_video(canvas)))
				return Text("Penguin.CanvasFailed");
			scene = obs_canvas_scene_create(canvas, name.constData());
			if (!scene || !UpdateProgram())
				return Text("Penguin.NoProgram");
			obs_canvas_set_channel(canvas, 0, obs_scene_get_source(scene));
		}
		OBSDataAutoRelease serviceSettings = obs_data_create();
		obs_data_set_string(serviceSettings, "server", destination.server.toUtf8().constData());
		obs_data_set_string(serviceSettings, "key", destination.key.toUtf8().constData());
		service = obs_service_create("rtmp_custom", name.constData(), serviceSettings, nullptr);
		if (!service)
			return Text("Penguin.Failed");
		OBSDataAutoRelease videoSettings = obs_encoder_defaults(destination.encoder.toUtf8().constData());
		if (!videoSettings)
			return Text("Penguin.MissingEncoder");
		obs_data_set_string(videoSettings, "rate_control", "CBR");
		obs_data_set_int(videoSettings, "bitrate", destination.bitrate);
		obs_data_set_int(videoSettings, "keyint_sec", 2);
		if (destination.encoder == QStringLiteral("obs_x264")) {
			obs_data_set_string(videoSettings, "preset", "veryfast");
			obs_data_set_string(videoSettings, "profile", "high");
		}
		video = obs_video_encoder_create(destination.encoder.toUtf8().constData(), name.constData(), videoSettings,
						 nullptr);
		if (!video)
			return Text("Penguin.MissingEncoder");
		obs_encoder_set_video(video, videoOutput);
		obs_encoder_set_preferred_video_format(video, VIDEO_FORMAT_NV12);
		OBSDataAutoRelease audioSettings = obs_data_create();
		obs_data_set_int(audioSettings, "bitrate", 160);
		const char *audioId = nullptr;
		for (const char *candidate : {"ffmpeg_aac", "libfdk_aac"}) {
			if (HasEncoder(QString::fromUtf8(candidate), "aac")) {
				audioId = candidate;
				break;
			}
		}
		if (!audioId)
			return Text("Penguin.AudioFailed");
		audio = obs_audio_encoder_create(audioId, name.constData(), audioSettings, 0, nullptr);
		if (!audio)
			return Text("Penguin.AudioFailed");
		obs_encoder_set_audio(audio, obs_get_audio());
		output = obs_output_create("rtmp_output", name.constData(), nullptr, nullptr);
		if (!output)
			return Text("Penguin.Failed");
		obs_output_set_video_encoder(output, video);
		obs_output_set_audio_encoder(output, audio, 0);
		obs_output_set_service(output, service);
		obs_output_set_reconnect_settings(output, 20, 2);
		ConnectSignals();
		return {};
	}
};

struct Entry {
	Destination destination;
	std::unique_ptr<RunningOutput> running;
	QString status;
};

// Keep the item model accessible to keyboard and assistive-technology users,
// while presenting each destination as one compact, three-line sidebar card.
class DestinationDelegate final : public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const override
	{
		return {0, option.fontMetrics.height() * 3 + 28};
	}

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
	{
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		const bool selected = option.state & QStyle::State_Selected;
		const QRect card = option.rect.adjusted(2, 3, -2, -3);
		painter->setPen(Qt::NoPen);
		painter->setBrush(option.palette.color(selected ? QPalette::Highlight : QPalette::AlternateBase));
		painter->drawRoundedRect(card, 8, 8);
		const QRect content = card.adjusted(12, 8, -12, -8);
		const int lineHeight = option.fontMetrics.height();
		const auto color = selected ? QPalette::HighlightedText : QPalette::Text;
		QFont nameFont = option.font;
		nameFont.setBold(true);
		painter->setFont(nameFont);
		painter->setPen(option.palette.color(color));
		painter->drawText(QRect(content.x(), content.y(), content.width(), lineHeight), Qt::AlignVCenter,
				  QFontMetrics(nameFont).elidedText(index.data().toString(), Qt::ElideRight, content.width()));
		painter->setFont(option.font);
		for (int column = 1; column <= 2; ++column) {
			painter->setPen(option.palette.color(selected ? QPalette::HighlightedText :
								       column == 1 ? QPalette::PlaceholderText : QPalette::Text));
			const QString text = index.siblingAtColumn(column).data().toString();
			painter->drawText(QRect(content.x(), content.y() + column * (lineHeight + 2), content.width(), lineHeight),
					  Qt::AlignVCenter, option.fontMetrics.elidedText(text, Qt::ElideRight, content.width()));
		}
		if (option.state & QStyle::State_HasFocus) {
			painter->setBrush(Qt::NoBrush);
			painter->setPen(option.palette.color(QPalette::Link));
			painter->drawRoundedRect(card.adjusted(1, 1, -1, -1), 7, 7);
		}
		painter->restore();
	}
};

class OutputDock final : public QWidget {
	QTreeWidget *destinations;
	QWidget *emptyState;
	QLabel *destinationCount;
	QPushButton *edit;
	QPushButton *remove;
	QPushButton *start;
	QPushButton *stop;
	QTimer timer;
	std::vector<std::unique_ptr<Entry>> entries;
	QPointer<ConnectionWizard> activeWizard;
	bool setupShown = false;
	bool shuttingDown = false;

	int Selected() const { return destinations->indexOfTopLevelItem(destinations->currentItem()); }
	Entry *SelectedEntry() const
	{
		const int index = Selected();
		return index >= 0 && static_cast<size_t>(index) < entries.size() ? entries[index].get() : nullptr;
	}

	void Load()
	{
		QFile file(ConfigPath());
		if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024)
			return;
		const QJsonObject config = QJsonDocument::fromJson(file.readAll()).object();
		setupShown = config.value(QStringLiteral("setupShown")).toBool();
		for (const auto &value : config.value(QStringLiteral("destinations")).toArray()) {
			if (entries.size() == MaxDestinations)
				break;
			const QJsonObject object = value.toObject();
			auto entry = std::make_unique<Entry>();
			auto &d = entry->destination;
			d.name = object.value(QStringLiteral("name")).toString();
			d.platform = object.value(QStringLiteral("platform")).toString();
			d.server = object.value(QStringLiteral("server")).toString();
			d.encoder = object.value(QStringLiteral("encoder")).toString(QStringLiteral("obs_x264"));
			d.layout = static_cast<Layout>(std::clamp(object.value(QStringLiteral("layout")).toInt(), 0, 2));
			d.bitrate = std::clamp(object.value(QStringLiteral("bitrate")).toInt(6000), 500, 50000);
			entry->status = Text("Penguin.NeedsKey");
			entries.push_back(std::move(entry));
		}
	}

	void Save()
	{
		QJsonArray saved;
		for (const auto &entry : entries) {
			saved.append(SavedDestination(entry->destination));
		}
		const QString path = ConfigPath();
		QSaveFile file(path);
		const QByteArray bytes = QJsonDocument(QJsonObject{{QStringLiteral("version"), 1},
							       {QStringLiteral("setupShown"), setupShown},
							       {QStringLiteral("destinations"), saved}})
						 .toJson();
		if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath()) || !file.open(QIODevice::WriteOnly) ||
		    !file.setPermissions(QFile::ReadOwner | QFile::WriteOwner) || file.write(bytes) != bytes.size() ||
		    !file.commit())
			ShowMessage(this, QMessageBox::Warning, Text("Penguin.Outputs"), Text("Penguin.ConfigError"));
	}

	void Refresh()
	{
		const int selected = Selected();
		destinations->clear();
		for (const auto &entry : entries) {
			const auto &d = entry->destination;
			const char *format = d.layout == Layout::Landscape      ? "Penguin.Landscape"
					     : d.layout == Layout::PortraitCrop ? "Penguin.PortraitCrop"
									    : "Penguin.PortraitFit";
			auto *item = new QTreeWidgetItem(destinations, {d.name, Text(format), entry->status});
			const QString description = QStringLiteral("%1\n%2 · %3\n%4")
						    .arg(d.name, d.platform, Text(format), entry->status);
			item->setToolTip(0, description);
			item->setData(0, Qt::AccessibleTextRole, description);
		}
		destinations->setVisible(!entries.empty());
		emptyState->setVisible(entries.empty());
		destinationCount->setText(QStringLiteral("%1 / %2").arg(entries.size()).arg(MaxDestinations));
		if (selected >= 0 && selected < destinations->topLevelItemCount())
			destinations->setCurrentItem(destinations->topLevelItem(selected));
		UpdateButtons();
	}

	void UpdateButtons()
	{
		setProperty("penguinDestinationCount", static_cast<int>(entries.size()));
		int active = 0;
		for (const auto &candidate : entries)
			active += candidate->running ? 1 : 0;
		setProperty("penguinActiveCount", active);
		const Entry *entry = SelectedEntry();
		edit->setEnabled(entry && !entry->running);
		remove->setEnabled(entry && !entry->running);
		start->setEnabled(entry && !entry->running);
		stop->setEnabled(entry && entry->running && !entry->running->stopping);
	}

	void Configure(bool welcome, Entry *existing = nullptr)
	{
		if (shuttingDown || (existing && existing->running))
			return;
		if (activeWizard) {
			activeWizard->raise();
			activeWizard->activateWindow();
			return;
		}
		if (!existing && entries.size() >= MaxDestinations) {
			ShowMessage(this, QMessageBox::Information, Text("Penguin.Outputs"), Text("Penguin.Limit"));
			return;
		}
		auto *wizard = new ConnectionWizard(this, welcome, existing ? &existing->destination : nullptr);
		activeWizard = wizard;
		wizard->setAttribute(Qt::WA_DeleteOnClose);
		wizard->setWindowModality(Qt::ApplicationModal);
		if (welcome)
			setupShown = true;
		// A nested exec() could let main-window shutdown delete a stack-owned child.
		// The parent's lifetime now owns the wizard and automatically cancels this callback.
		connect(wizard, &QDialog::finished, this, [this, wizard, welcome, existing](int result) mutable {
			activeWizard = nullptr;
			if (shuttingDown)
				return;
			if (result == QDialog::Accepted) {
				if (existing && std::none_of(entries.begin(), entries.end(),
							    [existing](const auto &entry) { return entry.get() == existing; }))
					return;
				if (!existing) {
					entries.push_back(std::make_unique<Entry>());
					existing = entries.back().get();
				}
				existing->destination = wizard->Value();
				existing->status = Text(existing->destination.key.isEmpty() ? "Penguin.NeedsKey" : "Penguin.Idle");
			}
			Save();
			Refresh();
			if (welcome && result == QDialog::Accepted) {
				if (auto *container = qobject_cast<QDockWidget *>(parentWidget())) {
					container->show();
				}
			}
		});
		wizard->show();
	}

	void Start(Entry &entry)
	{
		if (entry.running || shuttingDown)
			return;
		QString error = Validate(entry.destination, true);
		auto running = std::make_unique<RunningOutput>();
		if (error.isEmpty())
			error = running->Initialize(entry.destination);
		if (error.isEmpty()) {
			running->startRequested = obs_output_start(running->output);
			if (!running->startRequested)
				error = Text("Penguin.Failed");
		}
		if (!error.isEmpty()) {
			entry.status = error;
			ShowMessage(this, QMessageBox::Warning, entry.destination.name, error);
		} else {
			entry.status = Text("Penguin.Starting");
			entry.running = std::move(running);
		}
		Refresh();
	}

	void Stop(Entry &entry)
	{
		if (!entry.running || entry.running->stopping)
			return;
		entry.running->stopping = true;
		// A connecting output is not active yet; the ordinary stop call ignores it.
		if (!entry.running->started.load() || entry.running->reconnecting.load())
			obs_output_force_stop(entry.running->output);
		else
			obs_output_stop(entry.running->output);
		entry.status = Text("Penguin.Stopping");
		Refresh();
	}

	void Tick()
	{
		bool changed = false;
		for (auto &entry : entries) {
			if (!entry->running)
				continue;
			auto &running = *entry->running;
			QString state;
			if (running.stopped.load()) {
				const int code = running.stopCode.load();
				state = code == OBS_OUTPUT_SUCCESS ? Text("Penguin.Stopped") : Text("Penguin.Error").arg(code);
				entry->running.reset();
			} else if (running.stopping) {
				state = Text("Penguin.Stopping");
			} else if (running.reconnecting.load()) {
				state = Text("Penguin.Reconnecting");
			} else {
				state = Text(running.started.load() ? "Penguin.Live" : "Penguin.Starting");
			}
			changed |= entry->status != state;
			entry->status = state;
		}
		if (changed)
			Refresh();
	}

	void MaybeShowSetup()
	{
		if (setupShown || shuttingDown)
			return;
		// Wait for main-window activation and any pending hardware setup before
		// opening another modal surface. Background/tray starts must not steal focus.
		if (window()->property("penguinStartupDialogPending").toBool() || !window()->isActiveWindow() ||
		    QApplication::activeModalWidget()) {
			QTimer::singleShot(1000, this, [this]() { MaybeShowSetup(); });
			return;
		}
		Configure(true);
	}

public:
	explicit OutputDock(QWidget *parent) : QWidget(parent)
	{
		setObjectName(QStringLiteral("penguinOutputPanel"));
		auto *box = new QVBoxLayout(this);
		box->setContentsMargins(14, 14, 14, 14);
		box->setSpacing(10);
		auto *heading = new QHBoxLayout;
		auto *title = new QLabel(Text("Penguin.Outputs"));
		title->setObjectName(QStringLiteral("penguinDestinationHeading"));
		title->setWordWrap(true);
		destinationCount = new QLabel;
		destinationCount->setObjectName(QStringLiteral("penguinDestinationCount"));
		heading->addWidget(title, 1);
		heading->addWidget(destinationCount);
		box->addLayout(heading);
		auto *hint = new QLabel(Text("Penguin.DestinationHint"));
		hint->setProperty("penguinHint", true);
		hint->setWordWrap(true);
		box->addWidget(hint);
		destinations = new QTreeWidget;
		destinations->setObjectName(QStringLiteral("penguinDestinationList"));
		destinations->setRootIsDecorated(false);
		destinations->setHeaderLabels({Text("Penguin.Destination"), Text("Penguin.Format"), Text("Penguin.Status")});
		destinations->setHeaderHidden(true);
		destinations->setColumnHidden(1, true);
		destinations->setColumnHidden(2, true);
		destinations->header()->setStretchLastSection(false);
		destinations->header()->setSectionResizeMode(0, QHeaderView::Stretch);
		destinations->setItemDelegate(new DestinationDelegate(destinations));
		destinations->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		destinations->setSelectionMode(QAbstractItemView::SingleSelection);
		destinations->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
		destinations->setMinimumHeight(110);
		box->addWidget(destinations, 1);
		emptyState = new QWidget;
		auto *emptyLayout = new QVBoxLayout(emptyState);
		emptyLayout->setContentsMargins(12, 20, 12, 20);
		emptyLayout->addStretch();
		auto *emptyTitle = new QLabel(Text("Penguin.EmptyTitle"));
		emptyTitle->setObjectName(QStringLiteral("penguinDestinationEmptyTitle"));
		auto *emptyText = new QLabel(Text("Penguin.EmptyText"));
		emptyText->setProperty("penguinHint", true);
		for (auto *label : {emptyTitle, emptyText}) {
			label->setAlignment(Qt::AlignCenter);
			label->setWordWrap(true);
			emptyLayout->addWidget(label);
		}
		emptyLayout->addStretch();
		box->addWidget(emptyState, 1);
		auto *add = new QPushButton(Text("Penguin.Add"));
		add->setObjectName(QStringLiteral("penguinAddDestination"));
		add->setProperty("penguinPrimary", true);
		box->addWidget(add);
		auto *actions = new QHBoxLayout;
		edit = new QPushButton(Text("Penguin.EditShort"));
		edit->setToolTip(Text("Penguin.Edit"));
		remove = new QPushButton(Text("Penguin.Remove"));
		for (auto *button : {edit, remove})
			actions->addWidget(button);
		box->addLayout(actions);
		auto *outputs = new QGridLayout;
		start = new QPushButton(Text("Penguin.StartShort"));
		start->setToolTip(Text("Penguin.Start"));
		stop = new QPushButton(Text("Penguin.StopShort"));
		stop->setToolTip(Text("Penguin.Stop"));
		auto *startAll = new QPushButton(Text("Penguin.StartAll"));
		auto *stopAll = new QPushButton(Text("Penguin.StopAll"));
		startAll->setObjectName(QStringLiteral("penguinStartAll"));
		stopAll->setObjectName(QStringLiteral("penguinStopAll"));
		outputs->addWidget(start, 0, 0);
		outputs->addWidget(stop, 0, 1);
		outputs->addWidget(startAll, 1, 0);
		outputs->addWidget(stopAll, 1, 1);
		outputs->setColumnStretch(0, 1);
		outputs->setColumnStretch(1, 1);
		box->addLayout(outputs);
		auto *note = new QLabel(Text("Penguin.SessionKeys"));
		note->setProperty("penguinHint", true);
		note->setToolTip(Text("Penguin.KeysNote"));
		note->setWordWrap(true);
		box->addWidget(note);
		connect(add, &QPushButton::clicked, this, [this]() { Configure(false); });
		connect(edit, &QPushButton::clicked, this, [this]() {
			if (auto *entry = SelectedEntry())
				Configure(false, entry);
		});
		connect(remove, &QPushButton::clicked, this, [this]() {
			if (auto *entry = SelectedEntry(); entry && !entry->running) {
				entries.erase(entries.begin() + Selected());
				Save();
				Refresh();
			}
		});
		connect(start, &QPushButton::clicked, this, [this]() {
			if (auto *entry = SelectedEntry())
				Start(*entry);
		});
		connect(stop, &QPushButton::clicked, this, [this]() {
			if (auto *entry = SelectedEntry())
				Stop(*entry);
		});
		connect(startAll, &QPushButton::clicked, this, [this]() {
			for (const auto &entry : entries) {
				if (!entry->running) {
					const QString error = Validate(entry->destination, true);
					if (!error.isEmpty()) {
						ShowMessage(this, QMessageBox::Warning, entry->destination.name, error);
						return;
					}
				}
			}
			for (auto &entry : entries)
				Start(*entry);
		});
		connect(stopAll, &QPushButton::clicked, this, [this]() {
			for (auto &entry : entries)
				Stop(*entry);
		});
		connect(destinations, &QTreeWidget::itemSelectionChanged, this, [this]() { UpdateButtons(); });
		connect(&timer, &QTimer::timeout, this, [this]() { Tick(); });
		timer.start(250);
		Load();
		Refresh();
	}

	void StopImmediately()
	{
		for (auto &entry : entries) {
			if (entry->running) {
				entry->running.reset();
				entry->status = Text("Penguin.CancelledByChange");
			}
		}
		Refresh();
	}

	void Event(obs_frontend_event event)
	{
		if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING && !setupShown) {
			QTimer::singleShot(800, this, [this]() { MaybeShowSetup(); });
		} else if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGING ||
			   event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING ||
			   event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP) {
			StopImmediately();
		} else if (event == OBS_FRONTEND_EVENT_EXIT || event == OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN) {
			shuttingDown = true;
			if (activeWizard)
				activeWizard->reject();
			timer.stop();
			StopImmediately();
		} else if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED || event == OBS_FRONTEND_EVENT_TRANSITION_CHANGED) {
			for (auto &entry : entries) {
				if (entry->running && !entry->running->UpdateProgram())
					Stop(*entry);
			}
		}
	}
};

QPointer<OutputDock> dock;
bool frontendShutdown = false;

void FrontendEvent(obs_frontend_event event, void *)
{
	if (dock)
		dock->Event(event);
	if (event == OBS_FRONTEND_EVENT_EXIT)
		frontendShutdown = true;
}
} // namespace

bool obs_module_load(void)
{
	frontendShutdown = false;
	return true;
}

void obs_module_post_load(void)
{
	auto *parent = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!parent)
		return;
	dock = new OutputDock(parent);
	if (!obs_frontend_add_dock_by_id(DockId, obs_module_text("Penguin.Outputs"), dock.data())) {
		delete dock.data();
		return;
	}
	obs_frontend_add_event_callback(FrontendEvent, nullptr);
}

void obs_module_unload(void)
{
	// OBS destroys its frontend callbacks immediately after the EXIT event.
	if (frontendShutdown)
		return;
	obs_frontend_remove_event_callback(FrontendEvent, nullptr);
	if (dock) {
		dock->StopImmediately();
		obs_frontend_remove_dock(DockId);
	}
}
