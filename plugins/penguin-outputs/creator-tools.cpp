// SPDX-License-Identifier: GPL-2.0-or-later
#include "creator-tools.hpp"
#include "../../frontend/utility/PenguinIcons.hpp"
#include <QFrame>
#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QStyle>
#include <QResizeEvent>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.hpp>
#include <util/config-file.h>
#include <graphics/vec4.h>
#include <QAction>
#include <QCheckBox>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QToolButton>
#include <QToolBar>
#include <QHideEvent>
#include <QShowEvent>
#include <QHBoxLayout>
#include <set>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QStorageInfo>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <vector>

namespace {
constexpr const char *VideoGuard = "penguin_privacy_video";
constexpr const char *AudioGuard = "penguin_privacy_audio";
std::atomic_bool privacy{false};
QString lastOutputHelp;
QString ExplainOutputFailure(const char *raw, bool recording)
{
	// Only classification leaves this function. Never echo URLs, keys or local paths.
	const QString message = QString::fromUtf8(raw ? raw : "", raw ? int(strnlen(raw, 4096)) : 0).toLower();
	if (message.isEmpty())
		return {};
	if (message.contains("forbidden") || message.contains("unauthor") ||
	    message.contains("stream key") || message.contains("authentication"))
		return "The output reported an authorization problem. Reconnect your account or review the destination credentials; do not share your stream key.";
	if (recording && (message.contains("no space") || message.contains("disk full") || message.contains("permission denied") || message.contains("cannot write")))
		return "Recording reported a storage problem. Check free space and write access in the recording directory.";
	if (message.contains("encoder") || message.contains("nvenc") || message.contains("vaapi") || message.contains("va-api"))
		return "The output reported an encoder problem. Stop outputs, check driver availability, and try another encoder or a lower output resolution.";
	return recording ? "Recording stopped with an error. Check recording storage and encoder settings, then make a short local test." :
		"Streaming stopped with an error. Check the connection, provider status and destination settings before retrying.";
}
QString RecordingFormatAdvice(const QString &format, bool custom)
{
	if (custom)
		return "Custom recording output: review its container and recovery options in Settings → Output before a long recording.";
	if (format == "mp4" || format == "mov")
		return "Standard MP4/MOV needs a clean recording stop. An interruption may make the file unusable. Consider MKV or Hybrid MP4 in Settings → Output; test compatibility with your editor.";
	if (format == "mkv" || format.startsWith("hybrid_") || format.startsWith("fragmented_"))
		return "The selected container supports recovery after an interrupted recording. This does not protect against disk or hardware failure. Test the file in your editor before a long session.";
	return "Review container recovery and editor compatibility in Settings → Output, then make a short test recording.";
}

QString RecordingDuration(int frames, uint32_t numerator, uint32_t denominator)
{
	if (frames < 0 || !numerator || !denominator)
		return "Unavailable";
	const uint64_t seconds = uint64_t(frames) * denominator / numerator;
	return QString("%1:%2:%3").arg(seconds / 3600, 2, 10, QChar('0'))
		.arg((seconds / 60) % 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
}

std::mutex guardMutex;
struct Guard {
	OBSWeakSource source;
	OBSSource filter;
};
std::vector<Guard> guards;
QPointer<QAction> privacyAction;
QPointer<QAction> creatorAction;
QPointer<QPushButton> privacyControl;
QPointer<QToolButton> privacyToolbar;

void *CreateGuard(obs_data_t *, obs_source_t *source) { return source; }
void DestroyGuard(void *) {}
const char *GuardName(void *) { return "Penguin privacy shield"; }
void RenderGuard(void *data, gs_effect_t *)
{
	auto *source = static_cast<obs_source_t *>(data);
	if (!privacy.load(std::memory_order_acquire)) {
		obs_source_skip_video_filter(source);
		return;
	}
	auto *target = obs_filter_get_target(source);
	if (!target)
		return;
	vec4 color;
	vec4_set(&color, 0.02f, 0.025f, 0.035f, 1.0f);
	auto *effect = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_effect_set_vec4(gs_effect_get_param_by_name(effect, "color"), &color);
	while (gs_effect_loop(effect, "Solid"))
		gs_draw_sprite(nullptr, 0, obs_source_get_base_width(target), obs_source_get_base_height(target));
}
obs_audio_data *SilenceGuard(void *, obs_audio_data *audio)
{
	if (privacy.load(std::memory_order_acquire)) {
		for (auto *plane : audio->data)
			if (plane)
				std::memset(plane, 0, size_t(audio->frames) * sizeof(float));
	}
	return audio;
}
void AttachGuard(obs_source_t *source, const char *id)
{
	if (!source)
		return;
	std::lock_guard<std::mutex> lock(guardMutex);
	for (const auto &entry : guards) {
		auto existing = OBSGetStrongRef(entry.source);
		if (existing == source && std::strcmp(obs_source_get_id(entry.filter), id) == 0)
			return;
	}
	OBSSourceAutoRelease filter = obs_source_create_private(id, "Penguin privacy shield", nullptr);
	if (!filter)
		return;
	obs_source_filter_add(source, filter);
	guards.push_back({OBSGetWeakRef(source), OBSSource(filter)});
}
bool ProtectAudio(void *, obs_source_t *source)
{
	if (source && obs_source_get_type(source) != OBS_SOURCE_TYPE_FILTER &&
	    (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO))
		AttachGuard(source, AudioGuard);
	return true;
}
void ProtectChild(obs_source_t *, obs_source_t *child, void *) { ProtectAudio(nullptr, child); }
void SourceCreated(void *, calldata_t *data)
{
	if (privacy.load(std::memory_order_acquire))
		ProtectAudio(nullptr, static_cast<obs_source_t *>(calldata_ptr(data, "source")));
}
void ChannelChanged(void *, calldata_t *data)
{
	if (!privacy.load(std::memory_order_acquire))
		return;
	auto *source = static_cast<obs_source_t *>(calldata_ptr(data, "source"));
	if (calldata_int(data, "channel") == 0)
		AttachGuard(source, VideoGuard);
	else
		ProtectAudio(nullptr, source);
}
void RemoveOldGuards(obs_source_t *, obs_source_t *filter, void *opaque)
{
	const char *id = obs_source_get_id(filter);
	if (std::strcmp(id, VideoGuard) == 0 || std::strcmp(id, AudioGuard) == 0)
		static_cast<std::vector<OBSSource> *>(opaque)->emplace_back(filter);
}
bool CleanSavedGuards(void *, obs_source_t *source)
{
	std::vector<OBSSource> saved;
	obs_source_enum_filters(source, RemoveOldGuards, &saved);
	for (auto &filter : saved)
		obs_source_filter_remove(source, filter);
	return true;
}
void SetPrivacy(bool active)
{
	if (active) {
		// Arm before enumeration so concurrently created sources are also covered.
		privacy.store(true, std::memory_order_release);
		obs_enum_sources(ProtectAudio, nullptr);
		for (uint32_t i = 0; i < MAX_CHANNELS; ++i) {
			OBSSourceAutoRelease source = obs_get_output_source(i);
			if (i == 0)
				AttachGuard(source, VideoGuard);
			ProtectAudio(nullptr, source);
			if (source)
				obs_source_enum_full_tree(source, ProtectChild, nullptr);
		}
	} else {
		privacy.store(false, std::memory_order_release);
		std::vector<Guard> previous;
		{
			std::lock_guard<std::mutex> lock(guardMutex);
			previous.swap(guards);
		}
		for (auto &entry : previous) {
			auto source = OBSGetStrongRef(entry.source);
			if (source)
				obs_source_filter_remove(source, entry.filter);
		}
	}
	if (privacyAction) {
		QSignalBlocker block(privacyAction);
		privacyAction->setChecked(active);
		privacyAction->setText(active ? "Privacy on" : "Privacy");
		privacyAction->setToolTip(active ? "Resume program picture and sound (Ctrl+Shift+P)" : "Hide program picture and silence audio (Ctrl+Shift+P)");
	}
	if (privacyControl) {
		QSignalBlocker block(privacyControl);
		privacyControl->setChecked(active);
		privacyControl->setText(active ? "Resume output" : "Enable privacy");
	}
}

struct SceneCheck {
	int visible = 0, unavailable = 0, audible = 0, muted = 0;
	std::set<obs_source_t *> visited;
};
bool CheckItem(obs_scene_t *, obs_sceneitem_t *item, void *opaque)
{
	if (!obs_sceneitem_visible(item))
		return true;
	auto &result = *static_cast<SceneCheck *>(opaque);
	auto *source = obs_sceneitem_get_source(item);
	if (!result.visited.insert(source).second)
		return true;
	auto *nested = obs_scene_from_source(source);
	if (!nested)
		nested = obs_group_from_source(source);
	if (nested) {
		obs_scene_enum_items(nested, CheckItem, opaque);
	} else if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) {
		// Audio-only media sources share the ffmpeg source's video capability flag.
		if (std::strcmp(obs_source_get_unversioned_id(source), "ffmpeg_source") == 0 &&
		    obs_source_audio_active(source) && !obs_source_get_width(source) && !obs_source_get_height(source))
			return true;
		++result.visible;
		if (!obs_source_get_width(source) || !obs_source_get_height(source))
			++result.unavailable;
	}
	return true;
}
bool CheckAudio(void *opaque, obs_source_t *source)
{
	if (obs_source_get_type(source) == OBS_SOURCE_TYPE_INPUT &&
	    (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) && obs_source_active(source)) {
		auto &result = *static_cast<SceneCheck *>(opaque);
		if (obs_source_muted(source) || obs_source_get_volume(source) <= 0.0001f || !obs_source_get_audio_mixers(source))
			++result.muted;
		else
			++result.audible;
	}
	return true;
}
struct AudioUndo {
	OBSWeakSource source;
	float volume = 1.0f;
	float appliedVolume = 1.0f;
	std::vector<OBSSource> filters;
};
class CreatorTools final : public QDialog {
	QTreeWidget *checks;
	QLabel *health;
	QLabel *session;
	QString diagnostics;
	QLabel *audioStatus;
	QComboBox *microphone;
	QPushButton *measure;
	QPushButton *apply;
	QPushButton *undo;
	QPushButton *shield;
	QCheckBox *noise;
	QTimer timer;
	QTimer sampleTimer;
	obs_volmeter_t *meter = nullptr;
	OBSWeakSource measuredSource;
	std::mutex sampleMutex;
	std::vector<float> peaks;
	float suggestedGain = 0.0f;
	AudioUndo change;
	uint32_t previousFrames = 0, previousLag = 0, previousSkipped = 0;
	int previousDropped = 0;

	static void Meter(void *data, const float *, const float *, const float input[])
	{
		auto *self = static_cast<CreatorTools *>(data);
		float peak = -INFINITY;
		for (int i = 0; i < obs_volmeter_get_nr_channels(self->meter); ++i)
			peak = std::max(peak, input[i]);
		std::lock_guard<std::mutex> lock(self->sampleMutex);
		if (std::isfinite(peak) && peak > -55.0f && self->peaks.size() < 4096)
			self->peaks.push_back(peak);
	}
	void StopMeter()
	{
		sampleTimer.stop();
		if (meter) {
			obs_volmeter_remove_callback(meter, Meter, this);
			obs_volmeter_destroy(meter);
			meter = nullptr;
		}
		measure->setEnabled(true);
		microphone->setEnabled(true);
	}
	void AddCheck(const QString &state, const QString &title, const QString &detail)
	{
		auto *item = new QTreeWidgetItem(checks);
		item->setData(0, Qt::AccessibleTextRole, title + ": " + state + ". " + detail);
		auto *card = new QFrame(checks);
		card->setProperty("penguinCard", true);
		auto *column = new QVBoxLayout(card);
		column->setContentsMargins(12, 8, 12, 8);
		column->setSpacing(4);
		auto *row = new QHBoxLayout;
		auto *heading = new QLabel(title, card);
		heading->setProperty("penguinSectionTitle", true);
		row->addWidget(heading, 1);
		auto *badge = new QLabel(state, card);
		badge->setProperty("penguinBadge", true);
		badge->setProperty("ready", state == "Ready");
		badge->setAlignment(Qt::AlignCenter);
		row->addWidget(badge);
		column->addLayout(row);
		auto *label = new QLabel(detail, card);
		label->setTextFormat(Qt::PlainText);
		label->setWordWrap(true);
		label->setProperty("penguinHint", true);
		column->addWidget(label);
		item->setSizeHint(0, QSize(500, 78));
		checks->setItemWidget(item, 0, card);
	}
	static bool AudioSource(void *data, obs_source_t *source)
	{
		if (obs_source_get_type(source) == OBS_SOURCE_TYPE_INPUT &&
		    (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO)) {
			auto *combo = static_cast<QComboBox *>(data);
			combo->addItem(QString::fromUtf8(obs_source_get_name(source)),
				       QString::fromUtf8(obs_source_get_uuid(source)));
		}
		return true;
	}
	OBSSource SelectedAudio()
	{
		OBSSourceAutoRelease source = obs_get_source_by_uuid(microphone->currentData().toString().toUtf8().constData());
		return OBSSource(source);
	}
	void MeasureAudio()
	{
		if (privacy.load()) {
			audioStatus->setText("Resume picture and sound before measuring your microphone.");
			return;
		}
		auto source = SelectedAudio();
		if (!source)
			return;
		StopMeter();
		measuredSource = OBSGetWeakRef(source);
		{
			std::lock_guard<std::mutex> lock(sampleMutex);
			peaks.clear();
		}
		meter = obs_volmeter_create(OBS_FADER_LOG);
		if (!meter || !obs_volmeter_attach_source(meter, source)) {
			StopMeter();
			audioStatus->setText("This source could not be measured. Choose another audio source.");
			return;
		}
		obs_volmeter_add_callback(meter, Meter, this);
		apply->setEnabled(false);
		measure->setEnabled(false);
		microphone->setEnabled(false);
		audioStatus->setText("Speak normally for 10 seconds. Only level measurements are retained; no audio is recorded.");
		sampleTimer.start(10000);
	}
	void FinishMeasurement()
	{
		StopMeter();
		std::lock_guard<std::mutex> lock(sampleMutex);
		if (peaks.size() < 10) {
			audioStatus->setText("Not enough audible speech. Check your microphone, then measure again.");
			return;
		}
		std::sort(peaks.begin(), peaks.end());
		const float peak = peaks[(peaks.size() - 1) * 9 / 10];
		suggestedGain = std::clamp(-12.0f - peak, -12.0f, 0.0f);
		audioStatus->setText(QString("Typical speech peak: %1 dBFS. Suggested fader level: %2 dB (unity maximum). "
			"Apply adds gentle compression and a -1 dB limiter only when those filters are absent. "
			"It does not unmute your microphone or replace existing filters.")
			.arg(peak, 0, 'f', 1).arg(suggestedGain, 0, 'f', 1));
		apply->setEnabled(true);
	}
	static void FindFilter(obs_source_t *, obs_source_t *filter, void *data)
	{
		auto *ids = static_cast<std::vector<std::string> *>(data);
		ids->emplace_back(obs_source_get_unversioned_id(filter));
	}
	void ApplyAudio()
	{
		auto source = OBSGetStrongRef(measuredSource);
		if (!source)
			return;
		// One undo transaction at a time; never remove a user's existing filters.
		if (!change.filters.empty() || change.source) {
			audioStatus->setText("Undo the previous assistant adjustment before applying another one.");
			return;
		}
		change.source = measuredSource;
		change.volume = obs_source_get_volume(source);
		change.appliedVolume = std::clamp(std::pow(10.0f, suggestedGain / 20.0f), 0.0f, 1.0f);
		obs_source_set_volume(source, change.appliedVolume);
		std::vector<std::string> ids;
		obs_source_enum_filters(source, FindFilter, &ids);
		std::vector<const char *> requested;
		if (noise->isChecked())
			requested.push_back("noise_suppress_filter");
		requested.push_back("compressor_filter");
		requested.push_back("limiter_filter");
		for (const char *id : requested) {
			if (std::find(ids.begin(), ids.end(), id) != ids.end())
				continue;
			OBSDataAutoRelease settings = obs_data_create();
			obs_data_set_double(settings, "threshold", std::strcmp(id, "limiter_filter") == 0 ? -1.0 : -18.0);
			if (std::strcmp(id, "compressor_filter") == 0) {
				obs_data_set_double(settings, "ratio", 3.0);
				obs_data_set_int(settings, "attack_time", 6);
				obs_data_set_int(settings, "release_time", 100);
			}
			OBSSourceAutoRelease filter = obs_source_create(std::strcmp(id, "noise_suppress_filter") == 0 ? "noise_suppress_filter_v2" : id,
				std::strcmp(id, "limiter_filter") == 0 ? "Creator voice limiter" : std::strcmp(id, "compressor_filter") == 0 ? "Creator voice compressor" : "Creator voice noise suppression", settings, nullptr);
			if (filter) {
				obs_source_filter_add(source, filter);
				change.filters.emplace_back(filter);
			}
		}
		apply->setEnabled(false);
		undo->setEnabled(true);
		audioStatus->setText("Voice adjustments applied. Listen to a short local recording before going live. Undo is available in this session.");
		obs_frontend_save();
	}
	void UndoAudio()
	{
		auto source = OBSGetStrongRef(change.source);
		if (source) {
			if (std::abs(obs_source_get_volume(source) - change.appliedVolume) < 0.0001f)
				obs_source_set_volume(source, change.volume);
			for (auto &filter : change.filters)
				obs_source_filter_remove(source, filter);
		}
		change = {};
		undo->setEnabled(false);
		audioStatus->setText("Assistant changes undone. Any later manual volume adjustment was preserved.");
		obs_frontend_save();
	}
	void UpdateHealth()
	{
		const uint32_t frames = obs_get_total_frames(), lag = obs_get_lagged_frames();
		auto *video = obs_get_video();
		const uint32_t skipped = video ? video_output_get_skipped_frames(video) : 0;
		OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
		const int dropped = output ? obs_output_get_frames_dropped(output) : 0;
		QStringList problems;
		if (frames >= previousFrames && lag > previousLag)
			problems << "Rendering delay: reduce expensive camera effects or scene complexity; check GPU load.";
		if (skipped > previousSkipped)
			problems << "Encoding delay: try a hardware encoder or lower output resolution/FPS after stopping your output.";
		if (dropped > previousDropped)
			problems << "Network drops: check your connection and streaming bitrate. This does not indicate a camera problem.";
		if (!lastOutputHelp.isEmpty())
			problems << "Last output failure: " + lastOutputHelp;
		OBSOutputAutoRelease recording = obs_frontend_get_recording_output();
		const bool active = obs_frontend_recording_active();
		obs_video_info info{};
		obs_get_video_info(&info);
		QString state = active ? (obs_frontend_recording_paused() ? "Paused" : "Recording") : "Stopped";
		QString summary = "Recording: " + state;
		if (recording) {
			summary += QString("\nVideo duration (approx.): %1 · Written data: %2 MiB")
				.arg(RecordingDuration(obs_output_get_total_frames(recording), info.fps_num, info.fps_den))
				.arg(obs_output_get_total_bytes(recording) / (1024.0 * 1024.0), 0, 'f', 1);
		}
		summary += QString("\nPrimary stream: %1 · Privacy: %2")
			.arg(obs_frontend_streaming_active() ? "Active" : "Stopped")
			.arg(privacy.load() ? "On" : "Off");
		session->setText(summary);
		diagnostics = QString("Penguin Studio — performance snapshot\n%1\nEngine render counters: %2 frames, %3 delayed\nEncoder skipped frames: %4\nPrimary stream dropped frames: %5\n\n%6\n\nCounters can reset between outputs. No account, source names, paths or credentials are included.")
			.arg(summary).arg(frames).arg(lag).arg(skipped).arg(dropped)
			.arg(problems.isEmpty() ? "No new loss events in the latest sample." : problems.join("\n"));
		health->setText(problems.isEmpty() ? "No new render, encoder or network losses in the last sample. This is not a bandwidth or account authorization test." : problems.join("\n\n"));
		previousFrames = frames;
		previousLag = lag;
		previousSkipped = skipped;
		previousDropped = dropped;
	}
	void FitCheckRows()
	{
		const int width = std::max(240, checks->viewport()->width() - 20);
		for (int i = 0; i < checks->topLevelItemCount(); ++i) {
			auto *item = checks->topLevelItem(i);
			auto *card = checks->itemWidget(item, 0);
			if (card && card->layout())
				item->setSizeHint(0, QSize(0, std::max(78, card->layout()->totalHeightForWidth(width) + 6)));
		}
	}
protected:
	void resizeEvent(QResizeEvent *event) override
	{
		QDialog::resizeEvent(event);
		QTimer::singleShot(0, this, [this] { FitCheckRows(); });
	}
	void hideEvent(QHideEvent *event) override
	{
		if (sampleTimer.isActive()) {
			StopMeter();
			apply->setEnabled(false);
			audioStatus->setText("Measurement cancelled. Measure again when you are ready.");
		}
		timer.stop();
		QDialog::hideEvent(event);
	}
	void showEvent(QShowEvent *event) override
	{
		RunChecks();
		UpdateHealth();
		timer.start(2000);
		QDialog::showEvent(event);
	}
public:
	explicit CreatorTools(QWidget *parent) : QDialog(parent)
	{
		setObjectName("penguinCreatorTools");
		setWindowTitle("Creator tools — Penguin Studio");
		resize(900, 760);
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(24, 20, 24, 16);
		layout->setSpacing(16);
		auto *title = new QLabel("Creator tools", this);
		title->setProperty("penguinPageTitle", true);
		layout->addWidget(title);
		auto *intro = new QLabel("Check your stream, tune your microphone and control privacy.", this);
		intro->setWordWrap(true);
		intro->setProperty("penguinHint", true);
		layout->addWidget(intro);
		auto *tabs = new QTabWidget(this);
		tabs->setObjectName("creatorTabs");
		layout->addWidget(tabs, 1);
		auto *checkPage = new QWidget(tabs);
		auto *checkLayout = new QVBoxLayout(checkPage);
		checkLayout->setContentsMargins(0, 16, 0, 0);
		checkLayout->setSpacing(12);
		checks = new QTreeWidget(checkPage);
		checks->setObjectName("creatorChecks");
		checks->setColumnCount(1);
		checks->setHeaderHidden(true);
		checks->setRootIsDecorated(false);
		checks->setSelectionMode(QAbstractItemView::NoSelection);
		checks->setFrameShape(QFrame::NoFrame);
		checks->setIndentation(0);
		checks->setWordWrap(true);
		checkLayout->addWidget(checks, 1);
		auto *checkActions = new QHBoxLayout;
		auto *localHint = new QLabel("Local checks only. No stream will be started.", checkPage);
		localHint->setProperty("penguinHint", true);
		localHint->setWordWrap(true);
		checkActions->addWidget(localHint, 1);
		auto *refresh = new QPushButton("Run stream check", checkPage);
		refresh->setAutoDefault(false);
		refresh->setProperty("penguinPrimary", true);
		checkActions->addWidget(refresh);
		checkLayout->addLayout(checkActions);
		connect(refresh, &QPushButton::clicked, this, [this] { RunChecks(); });
		tabs->addTab(checkPage, "Stream check");
		auto *healthPage = new QWidget(tabs);
		auto *healthLayout = new QVBoxLayout(healthPage);
		healthLayout->setContentsMargins(0, 16, 0, 0);
		healthLayout->setSpacing(12);
		auto *healthTitle = new QLabel("Understand stream health", healthPage);
		healthTitle->setProperty("penguinSectionTitle", true);
		healthLayout->addWidget(healthTitle);
		session = new QLabel(healthPage);
		session->setObjectName("creatorSessionSummary");
		session->setProperty("penguinStatusPanel", true);
		session->setWordWrap(true);
		healthLayout->addWidget(session);
		health = new QLabel(healthPage);
		health->setProperty("penguinStatusPanel", true);
		health->setWordWrap(true);
		health->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		healthLayout->addWidget(health);
		auto *snapshotRow = new QHBoxLayout;
		auto *snapshotHint = new QLabel("Copy a snapshot for support. No account details or file paths are included.", healthPage);
		snapshotHint->setWordWrap(true);
		snapshotHint->setProperty("penguinHint", true);
		snapshotRow->addWidget(snapshotHint, 1);
		auto *copy = new QPushButton("Copy diagnostics", healthPage);
		copy->setObjectName("creatorCopyDiagnostics");
		copy->setAutoDefault(false);
		snapshotRow->addWidget(copy);
		healthLayout->addLayout(snapshotRow);
		connect(copy, &QPushButton::clicked, this, [this] {
			// Use the displayed sample; do not consume/reset loss deltas on copy.
			QApplication::clipboard()->setText(diagnostics);
		});
		healthLayout->addStretch();
		tabs->addTab(healthPage, "Performance help");
		connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
			if (index == 1)
				UpdateHealth();
		});
		auto *audioPage = new QWidget(tabs);
		auto *audioLayout = new QVBoxLayout(audioPage);
		audioLayout->setContentsMargins(0, 16, 0, 0);
		audioLayout->setSpacing(16);
		auto *microphoneCard = new QFrame(audioPage);
		microphoneCard->setProperty("penguinCard", true);
		auto *microphoneLayout = new QVBoxLayout(microphoneCard);
		microphoneLayout->setContentsMargins(16, 16, 16, 16);
		microphoneLayout->setSpacing(12);
		auto *microphoneTitle = new QLabel("Microphone", microphoneCard);
		microphoneTitle->setProperty("penguinSectionTitle", true);
		microphoneLayout->addWidget(microphoneTitle);
		auto *voiceHint = new QLabel("Speak at your normal streaming volume for 10 seconds.", microphoneCard);
		voiceHint->setProperty("penguinHint", true);
		voiceHint->setWordWrap(true);
		microphoneLayout->addWidget(voiceHint);
		auto *measurementRow = new QHBoxLayout;
		measurementRow->setSpacing(8);
		microphone = new QComboBox(microphoneCard);
		microphone->setObjectName("creatorMicrophone");
		microphone->setAccessibleName("Microphone");
		microphoneTitle->setBuddy(microphone);
		measurementRow->addWidget(microphone, 1);
		measure = new QPushButton("Measure voice (10 s)", microphoneCard);
		measure->setObjectName("creatorMeasure");
		measure->setAutoDefault(false);
		measurementRow->addWidget(measure);
		microphoneLayout->addLayout(measurementRow);
		audioLayout->addWidget(microphoneCard);
		audioStatus = new QLabel("No audio will be recorded or uploaded.", audioPage);
		audioStatus->setProperty("penguinStatusPanel", true);
		audioStatus->setWordWrap(true);
		audioLayout->addWidget(audioStatus);
		noise = new QCheckBox("Add noise suppression if none is present", audioPage);
		noise->setToolTip("Processes locally using additional CPU. Existing noise suppression is preserved.");
		audioLayout->addWidget(noise);
		audioLayout->addStretch();
		apply = new QPushButton("Apply suggested voice settings", audioPage);
		apply->setAutoDefault(false);
		apply->setProperty("penguinPrimary", true);
		apply->setEnabled(false);
		connect(microphone, &QComboBox::currentIndexChanged, this, [this] { apply->setEnabled(false); });
		undo = new QPushButton("Undo assistant changes", audioPage);
		undo->setAutoDefault(false);
		undo->setEnabled(false);
		auto *audioActions = new QHBoxLayout;
		audioActions->setSpacing(8);
		audioActions->addWidget(undo);
		audioActions->addStretch();
		audioActions->addWidget(apply);
		audioLayout->addLayout(audioActions);
		tabs->addTab(audioPage, "Audio assistant");
		connect(measure, &QPushButton::clicked, this, [this] { MeasureAudio(); });
		connect(apply, &QPushButton::clicked, this, [this] { ApplyAudio(); });
		connect(undo, &QPushButton::clicked, this, [this] { UndoAudio(); });
		sampleTimer.setSingleShot(true);
		connect(&sampleTimer, &QTimer::timeout, this, [this] { FinishMeasurement(); });
		auto *privacyCard = new QFrame(this);
		privacyCard->setProperty("penguinCard", true);
		auto *privacyRow = new QHBoxLayout(privacyCard);
		privacyRow->setContentsMargins(16, 12, 16, 12);
		privacyRow->setSpacing(16);
		auto *privacyText = new QVBoxLayout;
		privacyText->setSpacing(4);
		auto *privacyTitle = new QLabel("Privacy shield", privacyCard);
		privacyTitle->setProperty("penguinSectionTitle", true);
		privacyText->addWidget(privacyTitle);
		auto *description = new QLabel("Hide program video and silence audio. Outputs keep running; the local preview may remain visible.", privacyCard);
		description->setWordWrap(true);
		description->setProperty("penguinHint", true);
		privacyText->addWidget(description);
		privacyRow->addLayout(privacyText, 1);
		shield = new QPushButton("Enable privacy", privacyCard);
		shield->setObjectName("creatorPrivacy");
		shield->setAutoDefault(false);
		shield->setCheckable(true);
		shield->setMinimumWidth(112);
		privacyControl = shield;
		shield->setChecked(privacy.load());
		if (privacy.load())
			shield->setText("Resume output");
		privacyRow->addWidget(shield);
		layout->addWidget(privacyCard);
		connect(shield, &QPushButton::toggled, this, [](bool enabled) { SetPrivacy(enabled); });
		auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
		buttons->button(QDialogButtonBox::Close)->setAutoDefault(false);
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::hide);
		layout->addWidget(buttons);
		connect(&timer, &QTimer::timeout, this, [this] { UpdateHealth(); });
		previousFrames = obs_get_total_frames();
		previousLag = obs_get_lagged_frames();
		previousSkipped = obs_get_video() ? video_output_get_skipped_frames(obs_get_video()) : 0;
		{
			OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
			previousDropped = output ? obs_output_get_frames_dropped(output) : 0;
		}
		timer.start(2000);
		RunChecks();
		UpdateHealth();
	}
	~CreatorTools() override { StopMeter(); }
	void RunChecks()
	{
		checks->clear();
		OBSSourceAutoRelease scene = obs_frontend_get_current_scene();
		SceneCheck result;
		if (scene) {
			auto *selectedScene = obs_scene_from_source(scene);
			if (selectedScene)
				obs_scene_enum_items(selectedScene, CheckItem, &result);
		}
		obs_enum_sources(CheckAudio, &result);
		AddCheck(scene && result.visible && !result.unavailable ? "Ready" : "Review", "Program picture",
			!scene || !result.visible ? "Add at least one visible video source to your program scene." :
			result.unavailable ? QString("%1 visible source(s) have no picture dimensions. Check camera connections and media files.").arg(result.unavailable) :
			QString("%1 visible source(s) have valid dimensions. Preview the picture to confirm the intended content.").arg(result.visible));
		obs_service_t *service = obs_frontend_get_streaming_service();
		OBSDataAutoRelease settings = service ? obs_service_get_settings(service) : nullptr;
		const bool configured = settings && std::strlen(obs_data_get_string(settings, "key")) > 0;
		AddCheck(configured ? "Configured" : "Review", "Streaming destination", configured ? "A stream key is configured. Account permission and network reachability have not been tested." : "Configure a destination in Settings → Stream, or in Streaming channels for multistream.");
		auto *config = obs_frontend_get_profile_config();
		const char *mode = config_get_string(config, "Output", "Mode");
		const char *path = config_get_string(config, mode && std::strcmp(mode, "Advanced") == 0 ? "AdvOut" : "SimpleOutput", mode && std::strcmp(mode, "Advanced") == 0 ? "RecFilePath" : "FilePath");
		QStorageInfo storage(QString::fromUtf8(path ? path : ""));
		const bool disk = storage.isValid() && storage.isReady() && !storage.isReadOnly() && storage.bytesAvailable() > (qint64(2) << 30);
		AddCheck(disk ? "Ready" : "Review", "Recording storage", disk ? QString("%1 GiB available. Long recordings may need more.").arg(storage.bytesAvailable() / double(qint64(1) << 30), 0, 'f', 1) : "Check the recording directory and leave at least 2 GiB free.");
		if (!sampleTimer.isActive()) {
			const auto selected = microphone->currentData();
			microphone->clear();
			obs_enum_sources(AudioSource, microphone);
			const int index = microphone->findData(selected);
			if (index >= 0)
				microphone->setCurrentIndex(index);
		}
		const bool advanced = mode && std::strcmp(mode, "Advanced") == 0;
		const QString type = QString::fromUtf8(config_get_string(config, "AdvOut", "RecType"));
		const QString quality = QString::fromUtf8(config_get_string(config, "SimpleOutput", "RecQuality"));
		const bool custom = advanced ? type == "FFmpeg" : quality == "Lossless";
		const QString format = QString::fromUtf8(config_get_string(config, advanced ? "AdvOut" : "SimpleOutput", "RecFormat2"));
		AddCheck("Review", "Recording resilience", RecordingFormatAdvice(format, custom));
		AddCheck(result.audible ? "Review" : "Silent", "Audio",
			QString("%1 active unmuted input(s), %2 muted or unrouted input(s). %3").arg(result.audible).arg(result.muted)
			.arg(result.audible ? "Measure your microphone and listen to a short recording." : "Check the mixer, mute buttons and audio routing before going live."));
		AddCheck("Review", "Privacy", "Check desktop notifications and sensitive windows. Preview your scene before going live.");
		QTimer::singleShot(0, this, [this] { FitCheckRows(); });
	}
	void Shutdown() { timer.stop(); StopMeter(); hide(); }
};

class PrivacyButton final : public QToolButton {
	QToolBar *toolbar;
	void UpdateSize()
	{
		const auto desired = toolbar->width() < 1200 ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon;
		if (toolButtonStyle() != desired) {
			setToolButtonStyle(desired);
			style()->unpolish(this);
			style()->polish(this);
			updateGeometry();
		}
	}
protected:
	bool eventFilter(QObject *watched, QEvent *event) override
	{
		if (watched == toolbar && event->type() == QEvent::Resize)
			UpdateSize();
		return QToolButton::eventFilter(watched, event);
	}
public:
	PrivacyButton(QWidget *parent, QToolBar *bar) : QToolButton(parent), toolbar(bar)
	{
		setObjectName("penguinPrivacyButton");
		setDefaultAction(privacyAction);
		setIcon(PenguinStudio::StudioIcon(PenguinStudio::Icon::Privacy));
		setIconSize(QSize(18, 18));
		setCursor(Qt::PointingHandCursor);
		setAccessibleName("Privacy shield");
		bar->installEventFilter(this);
		UpdateSize();
	}
};
QPointer<CreatorTools> tools;
QPointer<QWidget> mainWindow;
bool stopped = false;
void Event(obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED || event == OBS_FRONTEND_EVENT_RECORDING_STARTED)
		lastOutputHelp.clear();
	if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED || event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
		const bool recording = event == OBS_FRONTEND_EVENT_RECORDING_STOPPED;
		OBSOutputAutoRelease output = recording ? obs_frontend_get_recording_output() : obs_frontend_get_streaming_output();
		if (output) {
			const auto help = ExplainOutputFailure(obs_output_get_last_error(output), recording);
			if (!help.isEmpty())
				lastOutputHelp = help;
		}
	}
	if ((event == OBS_FRONTEND_EVENT_FINISHED_LOADING || event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED) && !privacy.load()) {
		obs_enum_sources(CleanSavedGuards, nullptr);
		for (uint32_t i = 0; i < MAX_CHANNELS; ++i) {
			OBSSourceAutoRelease source = obs_get_output_source(i);
			if (source)
				CleanSavedGuards(nullptr, source);
		}
	}
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING && mainWindow && !privacyToolbar) {
		auto *bar = mainWindow->findChild<QToolBar *>("penguinTopBar");
		if (bar) {
			auto *anchor = mainWindow->findChild<QToolButton *>("penguinToolbarButton");
			auto *row = anchor ? qobject_cast<QHBoxLayout *>(anchor->parentWidget()->layout()) : nullptr;
			privacyToolbar = new PrivacyButton(row ? anchor->parentWidget() : bar, bar);
			if (row)
				row->insertWidget(row->indexOf(anchor), privacyToolbar);
			else
				bar->addWidget(privacyToolbar);
		}
	}
	if (event == OBS_FRONTEND_EVENT_EXIT) {
		CloseCreatorTools();
		return;
	}
	if (event == OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN || event == OBS_FRONTEND_EVENT_EXIT) {
		stopped = true;
		if (tools) {
			tools->Shutdown();
			delete tools.data();
		}
	}
}
} // namespace

void RegisterCreatorTools()
{
	obs_source_info video{};
	video.id = VideoGuard;
	video.type = OBS_SOURCE_TYPE_FILTER;
	video.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CAP_OBSOLETE;
	video.get_name = GuardName;
	video.create = CreateGuard;
	video.destroy = DestroyGuard;
	video.video_render = RenderGuard;
	obs_register_source(&video);
	obs_source_info audio{};
	audio.id = AudioGuard;
	audio.type = OBS_SOURCE_TYPE_FILTER;
	audio.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_CAP_OBSOLETE;
	audio.get_name = GuardName;
	audio.create = CreateGuard;
	audio.destroy = DestroyGuard;
	audio.filter_audio = SilenceGuard;
	obs_register_source(&audio);
}
void OpenCreatorTools()
{
	mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!mainWindow)
		return;
	privacyAction = new QAction("Privacy", mainWindow);
	privacyAction->setCheckable(true);
	privacyAction->setIcon(PenguinStudio::StudioIcon(PenguinStudio::Icon::Privacy));
	privacyAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
	privacyAction->setToolTip("Hide the program picture and silence source audio (Ctrl+Shift+P)");
	mainWindow->addAction(privacyAction);
	QObject::connect(privacyAction, &QAction::toggled, mainWindow, [](bool enabled) { SetPrivacy(enabled); });
	auto *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Creator tools"));
	creatorAction = action;
	QObject::connect(action, &QAction::triggered, mainWindow, [] {
		if (stopped)
			return;
		if (!tools)
			tools = new CreatorTools(mainWindow);
		tools->show();
		tools->raise();
		tools->activateWindow();
	});
	signal_handler_connect(obs_get_signal_handler(), "source_create", SourceCreated, nullptr);
	signal_handler_connect(obs_get_signal_handler(), "channel_change", ChannelChanged, nullptr);
	obs_frontend_add_event_callback(Event, nullptr);
}
void CloseCreatorTools()
{
	static bool closed = false;
	if (closed)
		return;
	closed = true;
	signal_handler_disconnect(obs_get_signal_handler(), "source_create", SourceCreated, nullptr);
	signal_handler_disconnect(obs_get_signal_handler(), "channel_change", ChannelChanged, nullptr);
	if (!stopped)
		obs_frontend_remove_event_callback(Event, nullptr);
	if (tools)
		delete tools.data();
	SetPrivacy(false);
	delete privacyToolbar.data();
	delete privacyAction.data();
	if (creatorAction)
		QObject::disconnect(creatorAction, nullptr, mainWindow, nullptr);
}
