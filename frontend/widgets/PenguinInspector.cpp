// SPDX-License-Identifier: GPL-2.0-or-later
#include "PenguinInspector.hpp"

#include "OBSBasic.hpp"

#include <utility/PenguinIcons.hpp>
#include <utility/PenguinCameraFilters.hpp>

#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "moc_PenguinInspector.cpp"

namespace {
QLabel *Label(const QString &text, QWidget *parent, const char *name = nullptr)
{
	auto *label = new QLabel(text, parent);
	label->setTextFormat(Qt::PlainText);
	if (name) {
		label->setObjectName(QString::fromUtf8(name));
	}
	return label;
}

QFrame *Card(QWidget *parent, QVBoxLayout *layout, const QString &title)
{
	auto *card = new QFrame(parent);
	card->setObjectName(QStringLiteral("inspectorCard"));
	auto *box = new QVBoxLayout(card);
	box->setContentsMargins(0, 12, 0, 0);
	box->setSpacing(10);
	if (!title.isEmpty()) {
		box->addWidget(Label(title, card, "inspectorSectionTitle"));
	}
	layout->addWidget(card);
	return card;
}

QDoubleSpinBox *Number(QWidget *parent, const char *name, const QString &accessibleName)
{
	auto *number = new QDoubleSpinBox(parent);
	number->setObjectName(QString::fromUtf8(name));
	number->setProperty("class", "inspector-field");
	number->setAccessibleName(accessibleName);
	number->setDecimals(2);
	number->setRange(-1000000.0, 1000000.0);
	number->setKeyboardTracking(false);
	number->setAccelerated(true);
	number->setMinimumWidth(0);
	number->setMinimumHeight(32);
	number->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
	return number;
}

bool Editing(const QWidget *widget)
{
	const auto *focus = QApplication::focusWidget();
	return widget->hasFocus() || (focus && widget->isAncestorOf(focus));
}

using PenguinCamera::CameraFilterId;
using PenguinCamera::CameraFilter;
using PenguinCamera::CameraMatting;
using PenguinCamera::SaveCamera;

void RestoreCamera(const std::string &json)
{
	PenguinCamera::RestoreCamera(json);
	OBSBasic::Get()->SaveProject();
}

void RestoreTransform(const std::string &json)
{
	OBSDataAutoRelease data = obs_data_create_from_json(json.c_str());
	OBSSourceAutoRelease scene = obs_get_source_by_uuid(obs_data_get_string(data, "scene_uuid"));
	if (!scene) {
		return;
	}
	OBSBasic::Get()->SetCurrentScene(scene.Get(), true);
	obs_scene_load_transform_states(json.c_str());
}
} // namespace

PenguinInspector::PenguinInspector(OBSBasic *mainWindow, QWidget *parent)
	: QWidget(parent),
	  main(mainWindow),
	  refreshTimer(this)
{
	setObjectName(QStringLiteral("penguinInspector"));
	setMinimumWidth(340);
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->setSpacing(0);
	auto *scroll = new QScrollArea(this);
	scroll->setObjectName(QStringLiteral("inspectorScroll"));
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	auto *body = new QWidget(scroll);
	body->setObjectName(QStringLiteral("inspectorBody"));
	auto *layout = new QVBoxLayout(body);
	layout->setContentsMargins(12, 14, 12, 14);
	layout->setSpacing(12);
	scroll->setWidget(body);
	outer->addWidget(scroll);

	emptyState = new QWidget(body);
	emptyState->setObjectName(QStringLiteral("inspectorEmptyState"));
	auto *emptyLayout = new QVBoxLayout(emptyState);
	emptyLayout->setContentsMargins(12, 36, 12, 12);
	emptyLayout->setSpacing(12);
	auto *emptyIcon = new QLabel(emptyState);
	emptyIcon->setObjectName(QStringLiteral("inspectorEmptyIcon"));
	emptyIcon->setPixmap(PenguinStudio::StudioIcon(PenguinStudio::Icon::Inspect).pixmap(32, 32));
	emptyIcon->setAlignment(Qt::AlignCenter);
	emptyIcon->setFixedSize(64, 64);
	emptyLayout->addWidget(emptyIcon, 0, Qt::AlignHCenter);
	emptyTitle = Label(QTStr("Penguin.Inspector.EmptyTitle"), emptyState, "inspectorEmptyTitle");
	emptyTitle->setAlignment(Qt::AlignCenter);
	emptyTitle->setWordWrap(true);
	emptyHint = Label(QTStr("Penguin.Inspector.EmptyHint"), emptyState, "inspectorEmptyHint");
	emptyHint->setAlignment(Qt::AlignCenter);
	emptyHint->setWordWrap(true);
	emptyLayout->addWidget(emptyTitle);
	emptyLayout->addWidget(emptyHint);
	layout->addWidget(emptyState);
	content = new QWidget(body);
	content->setObjectName(QStringLiteral("inspectorContent"));
	auto *contents = new QVBoxLayout(content);
	contents->setContentsMargins(0, 0, 0, 0);
	contents->setSpacing(14);
	layout->addWidget(content);
	layout->addStretch();

	auto *identityCard = new QFrame(content);
	identityCard->setObjectName(QStringLiteral("inspectorIdentity"));
	auto *identityLayout = new QVBoxLayout(identityCard);
	identityLayout->setContentsMargins(10, 10, 10, 10);
	identityLayout->setSpacing(10);
	auto *identity = new QHBoxLayout;
	identity->setSpacing(10);
	sourceIcon = new QLabel(identityCard);
	sourceIcon->setObjectName(QStringLiteral("inspectorSourceIcon"));
	sourceIcon->setAlignment(Qt::AlignCenter);
	sourceIcon->setFixedSize(40, 40);
	identity->addWidget(sourceIcon);
	auto *names = new QVBoxLayout;
	names->setSpacing(3);
	sourceName = Label({}, identityCard, "inspectorSourceName");
	sourceName->setWordWrap(true);
	sourceType = Label({}, identityCard, "inspectorSourceType");
	sourceType->setWordWrap(true);
	names->addWidget(sourceName);
	names->addWidget(sourceType);
	identity->addLayout(names, 1);
	identityLayout->addLayout(identity);

	auto *flags = new QHBoxLayout;
	flags->setSpacing(6);
	visible = new QToolButton(identityCard);
	visible->setObjectName(QStringLiteral("inspectorVisible"));
	visible->setText(QTStr("Penguin.Inspector.Visible"));
	visible->setCheckable(true);
	locked = new QToolButton(identityCard);
	locked->setObjectName(QStringLiteral("inspectorLocked"));
	locked->setText(QTStr("Penguin.Inspector.Locked"));
	locked->setCheckable(true);
	for (auto *button : {visible, locked}) {
		button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		button->setMinimumHeight(32);
		flags->addWidget(button);
	}
	identityLayout->addLayout(flags);
	contents->addWidget(identityCard);
	connect(visible, &QToolButton::clicked, this, [this](bool value) { ChangeFlag(true, value); });
	connect(locked, &QToolButton::clicked, this, [this](bool value) { ChangeFlag(false, value); });

	auto addAction = [this](QGridLayout *grid, int row, int column, const char *key, const char *name,
				const std::function<void()> &callback) {
		const QString label = QTStr(key);
		// Action labels are plain language, not keyboard mnemonics.
		auto *button = new QPushButton(QString(label).replace(QLatin1Char('&'), QStringLiteral("&&")), content);
		button->setObjectName(QString::fromUtf8(name));
		button->setProperty("class", "inspector-action");
		button->setAccessibleName(label);
		button->setToolTip(label);
		button->setMinimumHeight(32);
		grid->addWidget(button, row, column);
		connect(button, &QPushButton::clicked, this, [this, callback]() {
			if (!CurrentItem()) {
				return;
			}
			callback();
			RefreshSelection();
		});
		return button;
	};
	auto *sourceActions = new QGridLayout;
	sourceActions->setSpacing(6);
	auto *properties =
		addAction(sourceActions, 0, 0, "Penguin.Inspector.Properties", "inspectorProperties", [this]() {
			if (auto item = CurrentItem()) {
				main->CreatePropertiesWindow(obs_sceneitem_get_source(item));
			}
		});
	properties->setIcon(PenguinStudio::StudioIcon(PenguinStudio::Icon::Settings));
	auto *filters = addAction(sourceActions, 1, 0, "Penguin.Inspector.Filters", "inspectorFilters", [this]() {
		if (auto item = CurrentItem()) {
			main->CreateFiltersWindow(obs_sceneitem_get_source(item));
		}
	});
	filters->setIcon(PenguinStudio::StudioIcon(PenguinStudio::Icon::Inspect));
	contents->addLayout(sourceActions);

	cameraCard = Card(content, contents, QTStr("Penguin.Camera.Title"));
	auto *cameraLayout = static_cast<QVBoxLayout *>(cameraCard->layout());
	auto addChoice = [&](const char *key, const char *name, const std::initializer_list<const char *> &choices) {
		cameraLayout->addWidget(Label(QTStr(key), cameraCard, "inspectorFieldLabel"));
		auto *box = new QComboBox(cameraCard);
		box->setObjectName(QString::fromUtf8(name));
		box->setAccessibleName(QTStr(key));
		box->setMinimumHeight(32);
		for (const auto *choice : choices) {
			box->addItem(QTStr(choice));
		}
		cameraLayout->addWidget(box);
		connect(box, &QComboBox::currentIndexChanged, this, &PenguinInspector::ChangeCamera);
		return box;
	};
	cameraShape = addChoice("Penguin.Camera.Shape", "cameraShape",
				{"Penguin.Camera.Rectangle", "Penguin.Camera.Circle", "Penguin.Camera.Oval",
				 "Penguin.Camera.Rounded"});
	auto addSlider = [&](const char *key, const char *name, int maximum) {
		auto *field = new QSpinBox(cameraCard);
		field->setObjectName(QString::fromUtf8(name));
		field->setAccessibleName(QTStr(key));
		field->setPrefix(QTStr(key) + QStringLiteral(": "));
		field->setSuffix(QStringLiteral(" %"));
		field->setRange(0, maximum);
		field->setKeyboardTracking(false);
		field->setMinimumHeight(32);
		cameraLayout->addWidget(field);
		connect(field, &QSpinBox::valueChanged, this, &PenguinInspector::ChangeCamera);
		return field;
	};
	cameraRadius = addSlider("Penguin.Camera.Radius", "cameraRadius", 100);
	cameraBackground = addChoice("Penguin.Camera.Background", "cameraBackground",
				     {"Penguin.Camera.Off", "Penguin.Camera.Automatic", "Penguin.Camera.Green",
				      "Penguin.Camera.Blue", "Penguin.Camera.Blur", "Penguin.Camera.Image"});
	addChoice("Penguin.Camera.Processing", "cameraProcessing",
		  {"Penguin.Camera.DeviceAutomatic", "Penguin.Camera.DeviceGPU", "Penguin.Camera.DeviceCPU"});
	auto *cameraHands = new QCheckBox(QTStr("Penguin.Camera.PreserveHands"), cameraCard);
	cameraHands->setObjectName("cameraPreserveHands");
	cameraHands->setToolTip(QTStr("Penguin.Camera.PreserveHandsHint"));
	cameraLayout->addWidget(cameraHands);
	connect(cameraHands, &QCheckBox::toggled, this, &PenguinInspector::ChangeCamera);
	cameraSimilarity = addSlider("Penguin.Camera.Similarity", "cameraSimilarity", 90);
	auto *cameraBlur = addSlider("Penguin.Camera.BlurStrength", "cameraBlur", 20);
	cameraBlur->setRange(1, 20);
	cameraBlur->setSuffix(QString());
	auto *cameraImage = new QPushButton(QTStr("Penguin.Camera.ChooseImage"), cameraCard);
	cameraImage->setObjectName("cameraImage");
	cameraLayout->addWidget(cameraImage);
	connect(cameraImage, &QPushButton::clicked, this, [this, cameraImage]() {
		const auto path = QFileDialog::getOpenFileName(this, QTStr("Penguin.Camera.ChooseImage"), {},
							       "Images (*.png *.jpg *.jpeg *.bmp)");
		if (path.isEmpty()) {
			return;
		}
		QImageReader reader(path);
		const auto size = reader.size();
		if (!size.isValid() || size.width() > 8192 || size.height() > 8192 || reader.read().isNull()) {
			QMessageBox::warning(this, QTStr("Penguin.Camera.Title"), QTStr("Penguin.Camera.InvalidImage"));
			return;
		}
		cameraImage->setProperty("backgroundPath", path);
		ChangeCamera();
	});
	cameraStatus = Label(QString(), cameraCard, "inspectorFieldLabel");
	cameraStatus->setWordWrap(true);
	cameraLayout->addWidget(cameraStatus);

	transformCard = Card(content, contents, QTStr("Penguin.Inspector.Transform"));
	auto *transformLayout = static_cast<QVBoxLayout *>(transformCard->layout());
	transformLayout->addWidget(Label(QTStr("Penguin.Inspector.Position"), transformCard, "inspectorFieldLabel"));
	auto *position = new QHBoxLayout;
	position->setSpacing(6);
	transformFields[0] = Number(transformCard, "inspectorX", QTStr("Basic.TransformWindow.PositionX"));
	transformFields[1] = Number(transformCard, "inspectorY", QTStr("Basic.TransformWindow.PositionY"));
	transformFields[0]->setPrefix(QStringLiteral("X  "));
	transformFields[1]->setPrefix(QStringLiteral("Y  "));
	position->addWidget(transformFields[0]);
	position->addWidget(transformFields[1]);
	transformLayout->addLayout(position);

	auto *dimensionsTitle = new QHBoxLayout;
	dimensionsTitle->addWidget(Label(QTStr("Penguin.Inspector.Dimensions"), transformCard, "inspectorFieldLabel"),
				   1);
	aspectLock = new QToolButton(transformCard);
	aspectLock->setObjectName(QStringLiteral("inspectorAspectLock"));
	aspectLock->setIcon(PenguinStudio::StudioIcon(PenguinStudio::Icon::Link));
	aspectLock->setToolTip(QTStr("Penguin.Inspector.AspectRatio"));
	aspectLock->setAccessibleName(QTStr("Penguin.Inspector.AspectRatio"));
	aspectLock->setCheckable(true);
	aspectLock->setChecked(true);
	dimensionsTitle->addWidget(aspectLock);
	transformLayout->addLayout(dimensionsTitle);
	auto *dimensions = new QHBoxLayout;
	dimensions->setSpacing(6);
	transformFields[2] = Number(transformCard, "inspectorWidth", QTStr("Penguin.Inspector.Width"));
	transformFields[3] = Number(transformCard, "inspectorHeight", QTStr("Penguin.Inspector.Height"));
	transformFields[2]->setPrefix(QStringLiteral("W  "));
	transformFields[3]->setPrefix(QStringLiteral("H  "));
	for (size_t index : {size_t(2), size_t(3)}) {
		transformFields[index]->setMinimum(1.0);
		dimensions->addWidget(transformFields[index]);
	}
	transformLayout->addLayout(dimensions);
	sourceSize = Label({}, transformCard, "inspectorSourceSize");
	transformLayout->addWidget(sourceSize);
	auto *rotation = new QHBoxLayout;
	rotation->addWidget(Label(QTStr("Penguin.Inspector.Rotation"), transformCard, "inspectorFieldLabel"), 1);
	transformFields[4] = Number(transformCard, "inspectorRotation", QTStr("Penguin.Inspector.Rotation"));
	transformFields[4]->setRange(-36000.0, 36000.0);
	transformFields[4]->setSuffix(QStringLiteral("°"));
	rotation->addWidget(transformFields[4], 1);
	transformLayout->addLayout(rotation);
	for (size_t index = 0; index < transformFields.size(); ++index) {
		connect(transformFields[index], &QDoubleSpinBox::valueChanged, this,
			[this, index](double value) { ChangeTransform(index, value); });
	}

	auto *arrange = new QGridLayout;
	arrange->setSpacing(6);
	const std::array<QPushButton *, 4> arrangeButtons = {
		addAction(arrange, 0, 0, "Penguin.Inspector.Fit", "inspectorFit",
			  [this]() { main->on_actionFitToScreen_triggered(); }),
		addAction(arrange, 0, 1, "Penguin.Inspector.Center", "inspectorCenter",
			  [this]() { main->on_actionCenterToScreen_triggered(); }),
		addAction(arrange, 0, 2, "Penguin.Inspector.FlipHorizontal", "inspectorFlipHorizontal",
			  [this]() { main->on_actionFlipHorizontal_triggered(); }),
		addAction(arrange, 0, 3, "Penguin.Inspector.FlipVertical", "inspectorFlipVertical",
			  [this]() { main->on_actionFlipVertical_triggered(); })};
	const std::array<PenguinStudio::Icon, 4> arrangeIcons = {PenguinStudio::Icon::Fit, PenguinStudio::Icon::Center,
								 PenguinStudio::Icon::FlipHorizontal,
								 PenguinStudio::Icon::FlipVertical};
	for (size_t index = 0; index < arrangeButtons.size(); ++index) {
		auto *button = arrangeButtons[index];
		button->setProperty("class", "inspector-arrange");
		button->setText({});
		button->setIcon(PenguinStudio::StudioIcon(arrangeIcons[index]));
		button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	transformLayout->addLayout(arrange);

	cropCard = Card(content, contents, {});
	auto *cropLayout = static_cast<QVBoxLayout *>(cropCard->layout());
	auto *cropToggle = new QToolButton(cropCard);
	cropToggle->setObjectName(QStringLiteral("inspectorCropToggle"));
	cropToggle->setText(QTStr("Penguin.Inspector.Crop"));
	cropToggle->setAccessibleName(QTStr("Penguin.Inspector.Crop"));
	cropToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	cropToggle->setArrowType(Qt::RightArrow);
	cropToggle->setCheckable(true);
	cropToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	cropLayout->addWidget(cropToggle);
	auto *cropBody = new QWidget(cropCard);
	cropBody->setObjectName(QStringLiteral("inspectorCropFields"));
	auto *cropGrid = new QGridLayout(cropBody);
	cropGrid->setContentsMargins(0, 0, 0, 0);
	cropGrid->setHorizontalSpacing(6);
	cropGrid->setVerticalSpacing(6);
	const std::array<const char *, 4> cropNames = {"inspectorCropLeft", "inspectorCropRight", "inspectorCropTop",
						       "inspectorCropBottom"};
	const std::array<const char *, 4> cropLabels = {"Penguin.Inspector.CropLeft", "Penguin.Inspector.CropRight",
							"Penguin.Inspector.CropTop", "Penguin.Inspector.CropBottom"};
	for (size_t index = 0; index < cropFields.size(); ++index) {
		auto *field = new QSpinBox(cropBody);
		cropFields[index] = field;
		field->setObjectName(QString::fromUtf8(cropNames[index]));
		field->setProperty("class", "inspector-field");
		field->setAccessibleName(QTStr(cropLabels[index]));
		field->setKeyboardTracking(false);
		field->setRange(0, 1000000);
		field->setSuffix(QStringLiteral(" px"));
		field->setMinimumHeight(30);
		field->setMinimumWidth(0);
		field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
		cropGrid->addWidget(Label(QTStr(cropLabels[index]), cropBody, "inspectorFieldLabel"),
				    int(index / 2) * 2, int(index % 2));
		cropGrid->addWidget(field, int(index / 2) * 2 + 1, int(index % 2));
		connect(field, &QSpinBox::valueChanged, this, [this, index](int value) { ChangeCrop(index, value); });
	}
	cropLayout->addWidget(cropBody);
	cropBody->hide();
	connect(cropToggle, &QToolButton::toggled, cropBody, [cropBody, cropToggle](bool expanded) {
		cropBody->setVisible(expanded);
		cropToggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
	});

	auto *advancedActions = new QGridLayout;
	advancedActions->setSpacing(6);
	addAction(advancedActions, 0, 0, "Penguin.Inspector.Advanced", "inspectorAdvanced",
		  [this]() { main->CreateEditTransformWindow(CurrentItem()); });
	auto *reset = addAction(advancedActions, 1, 0, "Penguin.Inspector.Reset", "inspectorReset",
				[this]() { main->on_actionResetTransform_triggered(); });
	reset->setIcon(PenguinStudio::StudioIcon(PenguinStudio::Icon::Reset));
	contents->addLayout(advancedActions);
	connect(&refreshTimer, &QTimer::timeout, this, &PenguinInspector::RefreshSelection);
	refreshTimer.start(200);
	RefreshSelection();
}

PenguinInspector::~PenguinInspector() = default;

OBSSceneItem PenguinInspector::CurrentItem() const
{
	if (stopped || !main || !obs_initialized() || main->isClosing() ||
	    main->GetAllSelectedSourceItems().size() != 1) {
		return nullptr;
	}
	return main->GetCurrentSceneItem();
}

void PenguinInspector::Shutdown()
{
	stopped = true;
	refreshTimer.stop();
	setEnabled(false);
}

void PenguinInspector::RefreshSelection()
{
	if (stopped) {
		return;
	}
	if (main && main->isClosing()) {
		Shutdown();
		return;
	}
	const auto item = CurrentItem();
	const int selected = main && obs_initialized() ? main->GetAllSelectedSourceItems().size() : 0;
	content->setVisible(!!item);
	emptyState->setVisible(!item);
	if (!item) {
		selectionToken.clear();
		emptyTitle->setText(selected > 1 ? QTStr("Penguin.Inspector.Multiple").arg(selected)
						 : QTStr("Penguin.Inspector.EmptyTitle"));
		emptyHint->setText(
			QTStr(selected > 1 ? "Penguin.Inspector.MultipleHint" : "Penguin.Inspector.EmptyHint"));
		return;
	}
	auto *source = obs_sceneitem_get_source(item);
	auto *scene = obs_sceneitem_get_scene(item);
	if (!source || !scene) {
		return;
	}
	const QString token = QString::fromUtf8(obs_source_get_uuid(obs_scene_get_source(scene))) + ':' +
			      QString::number(obs_sceneitem_get_id(item));
	const bool changed = token != selectionToken;
	selectionToken = token;
	syncing = true;
	sourceName->setText(QString::fromUtf8(obs_source_get_name(source)));
	sourceType->setText(QString::fromUtf8(obs_source_get_display_name(obs_source_get_id(source))));
	sourceIcon->setPixmap(main->GetSourceIcon(obs_source_get_unversioned_id(source)).pixmap(28, 28));
	visible->setChecked(obs_sceneitem_visible(item));
	locked->setChecked(obs_sceneitem_locked(item));
	const uint32_t sourceWidth = obs_source_get_width(source);
	const uint32_t sourceHeight = obs_source_get_height(source);
	sourceSize->setText(QTStr("Penguin.Inspector.SourceSize").arg(sourceWidth).arg(sourceHeight));
	const bool editable = !obs_sceneitem_locked(item) && (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) &&
			      sourceWidth && sourceHeight;
	const bool cameraAvailable = obs_source_get_display_name(CameraFilterId) != nullptr;
	cameraCard->setVisible(cameraAvailable && (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO));
	cameraCard->setEnabled(editable);
	auto camera = CameraFilter(source);
	OBSDataAutoRelease cameraSettings = camera ? obs_source_get_settings(camera) : obs_data_create();
	if (!camera) {
		obs_data_set_double(cameraSettings, "radius", 25);
		obs_data_set_double(cameraSettings, "similarity", 15);
	}
	if (changed || !Editing(cameraShape)) {
		cameraShape->setCurrentIndex(std::clamp(int(obs_data_get_int(cameraSettings, "shape")), 0, 3));
	}
	if (changed || !Editing(cameraBackground)) {
		cameraBackground->setCurrentIndex(
			std::clamp(int(obs_data_get_int(cameraSettings, "background")), 0, 5));
	}
	if (changed || !Editing(cameraRadius)) {
		cameraRadius->setValue(int(obs_data_get_double(cameraSettings, "radius")));
	}
	if (changed || !Editing(cameraSimilarity)) {
		cameraSimilarity->setValue(int(obs_data_get_double(cameraSettings, "similarity")));
	}
	auto *cameraBlur = findChild<QSpinBox *>("cameraBlur");
	auto *cameraImage = findChild<QPushButton *>("cameraImage");
	if (changed || !Editing(cameraBlur)) {
		cameraBlur->setValue(obs_data_has_user_value(cameraSettings, "blur")
					     ? int(obs_data_get_int(cameraSettings, "blur"))
					     : 8);
	}
	const auto cameraImagePath = QString::fromUtf8(obs_data_get_string(cameraSettings, "background_image"));
	cameraImage->setText(cameraImagePath.isEmpty() ? QTStr("Penguin.Camera.ChooseImage")
						       : QFileInfo(cameraImagePath).fileName());
	cameraImage->setToolTip(cameraImagePath);
	cameraImage->setProperty("backgroundPath", cameraImagePath);
	auto *cameraProcessing = findChild<QComboBox *>("cameraProcessing");
	const auto device = std::string(obs_data_get_string(cameraSettings, "inference_device"));
	if (changed || !Editing(cameraProcessing)) {
		cameraProcessing->setCurrentIndex(device == "cpu" ? 2 : device == "gpu" ? 1 : 0);
	}
	cameraProcessing->setVisible(cameraBackground->currentIndex() == 1 || cameraBackground->currentIndex() >= 4);
	auto *cameraHands = findChild<QCheckBox *>("cameraPreserveHands");
	cameraHands->setChecked(obs_data_get_bool(cameraSettings, "preserve_hands"));
	cameraHands->setVisible(cameraBackground->currentIndex() == 1 || cameraBackground->currentIndex() >= 4);
	cameraImage->setVisible(cameraBackground->currentIndex() == 5);
	cameraBlur->setVisible(cameraBackground->currentIndex() == 4);
	cameraRadius->setVisible(cameraShape->currentIndex() == 3);
	cameraSimilarity->setVisible((cameraBackground->currentIndex() == 2 || cameraBackground->currentIndex() == 3));
	calldata_t status;
	calldata_init(&status);
	auto matting = CameraMatting(source);
	const bool usesRvm = std::strcmp(obs_data_get_string(cameraSettings, "automatic_backend"), "rvm") == 0;
	auto *statusSource = usesRvm ? matting.Get() : camera.Get();
	int aiStatus = 3;
	if (statusSource && obs_source_enabled(statusSource) &&
	    proc_handler_call(obs_source_get_proc_handler(statusSource), "camera_status", &status)) {
		aiStatus = int(calldata_int(&status, "status"));
	}
	const bool activeGPU = calldata_bool(&status, "gpu");
	const bool cpuFallback = calldata_bool(&status, "fallback");
	const bool gpuPreparing = calldata_bool(&status, "preparing");
	calldata_free(&status);
	const char *statusKey = "Penguin.Camera.Privacy";
	if (camera && !obs_source_enabled(camera)) {
		statusKey = "Penguin.Camera.Disabled";
	} else if ((cameraBackground->currentIndex() == 1 || cameraBackground->currentIndex() >= 4)) {
		statusKey = aiStatus == 3   ? "Penguin.Camera.Error"
			    : aiStatus == 2 ? (gpuPreparing  ? "Penguin.Camera.PreparingGPU"
					       : activeGPU   ? "Penguin.Camera.ReadyGPU"
					       : cpuFallback ? "Penguin.Camera.ReadyFallback"
							     : "Penguin.Camera.ReadyCPU")
					    : "Penguin.Camera.Loading";
	}
	if (cameraBackground->currentIndex() == 5 &&
	    (cameraImagePath.isEmpty() || !QFileInfo::exists(cameraImagePath))) {
		statusKey = "Penguin.Camera.MissingImage";
	}
	cameraStatus->setText(QTStr(statusKey));
	transformCard->setEnabled(editable);
	cropCard->setEnabled(editable);
	findChild<QPushButton *>(QStringLiteral("inspectorAdvanced"))->setEnabled(editable);
	findChild<QPushButton *>(QStringLiteral("inspectorReset"))->setEnabled(editable);
	obs_transform_info info;
	obs_sceneitem_crop crop;
	obs_sceneitem_get_info2(item, &info);
	obs_sceneitem_get_crop(item, &crop);
	const double width =
		info.bounds_type == OBS_BOUNDS_NONE
			? std::max(0.0, double(sourceWidth) - crop.left - crop.right) * std::abs(info.scale.x)
			: info.bounds.x;
	const double height =
		info.bounds_type == OBS_BOUNDS_NONE
			? std::max(0.0, double(sourceHeight) - crop.top - crop.bottom) * std::abs(info.scale.y)
			: info.bounds.y;
	const std::array<double, 5> values = {info.pos.x, info.pos.y, width, height, info.rot};
	for (size_t index = 0; index < transformFields.size(); ++index) {
		if (changed || !Editing(transformFields[index])) {
			transformFields[index]->setValue(values[index]);
		}
	}
	const std::array<int, 4> crops = {crop.left, crop.right, crop.top, crop.bottom};
	const std::array<int, 4> maxima = {std::max(0, int(sourceWidth) - crop.right - 1),
					   std::max(0, int(sourceWidth) - crop.left - 1),
					   std::max(0, int(sourceHeight) - crop.bottom - 1),
					   std::max(0, int(sourceHeight) - crop.top - 1)};
	for (size_t index = 0; index < cropFields.size(); ++index) {
		cropFields[index]->setMaximum(maxima[index]);
		if (changed || !Editing(cropFields[index])) {
			cropFields[index]->setValue(crops[index]);
		}
	}
	syncing = false;
}

void PenguinInspector::ApplyTransform(const std::function<void(obs_sceneitem_t *)> &edit)
{
	if (syncing) {
		return;
	}
	const auto item = CurrentItem();
	if (!item || obs_sceneitem_locked(item)) {
		return;
	}
	const OBSScene scene = main->GetCurrentScene();
	if (!scene) {
		return;
	}
	const char *sceneUuid = obs_source_get_uuid(obs_scene_get_source(scene));
	// Resizing a group rebases its children and moves the parent on the next
	// video frame. Keep those related transforms when editing a group child.
	const bool includeGroupTransforms = obs_scene_is_group(obs_sceneitem_get_scene(item));
	OBSDataAutoRelease before = obs_scene_save_transform_states(scene, includeGroupTransforms);
	// libobs stores per-scene UUIDs inside scenes_and_groups, not at the root.
	// Retain the edited scene separately so undo also restores its selection.
	obs_data_set_string(before, "scene_uuid", sceneUuid);
	const std::string undo = obs_data_get_json(before);
	edit(item);
	OBSDataAutoRelease after = obs_scene_save_transform_states(scene, includeGroupTransforms);
	obs_data_set_string(after, "scene_uuid", sceneUuid);
	const std::string redo = obs_data_get_json(after);
	if (undo != redo) {
		main->undo_s.add_action(
			QTStr("Penguin.Inspector.Undo").arg(obs_source_get_name(obs_sceneitem_get_source(item))),
			RestoreTransform, RestoreTransform, undo, redo);
	}
	RefreshSelection();
}

void PenguinInspector::ChangeTransform(size_t field, double value)
{
	ApplyTransform([this, field, value](obs_sceneitem_t *item) {
		obs_transform_info info;
		obs_sceneitem_get_info2(item, &info);
		if (field == 0) {
			info.pos.x = float(value);
		} else if (field == 1) {
			info.pos.y = float(value);
		} else if (field == 4) {
			info.rot = float(value);
		} else {
			const bool horizontal = field == 2;
			if (info.bounds_type != OBS_BOUNDS_NONE) {
				float &dimension = horizontal ? info.bounds.x : info.bounds.y;
				float &other = horizontal ? info.bounds.y : info.bounds.x;
				if (aspectLock->isChecked() && dimension > 0.0f) {
					other *= float(value / dimension);
				}
				dimension = float(value);
			} else {
				obs_sceneitem_crop crop;
				obs_sceneitem_get_crop(item, &crop);
				auto *source = obs_sceneitem_get_source(item);
				const double extent =
					horizontal ? double(obs_source_get_width(source)) - crop.left - crop.right
						   : double(obs_source_get_height(source)) - crop.top - crop.bottom;
				if (extent <= 0) {
					return;
				}
				float &scale = horizontal ? info.scale.x : info.scale.y;
				float &other = horizontal ? info.scale.y : info.scale.x;
				const float next = std::copysign(float(value / extent), scale);
				if (aspectLock->isChecked() && scale != 0.0f) {
					other *= std::abs(next / scale);
				}
				scale = next;
			}
		}
		obs_sceneitem_set_info2(item, &info);
	});
}

void PenguinInspector::ChangeCrop(size_t field, int value)
{
	ApplyTransform([field, value](obs_sceneitem_t *item) {
		obs_sceneitem_crop crop;
		obs_sceneitem_get_crop(item, &crop);
		auto *source = obs_sceneitem_get_source(item);
		if (field == 0) {
			crop.left =
				std::clamp(value, 0, std::max(0, int(obs_source_get_width(source)) - crop.right - 1));
		} else if (field == 1) {
			crop.right =
				std::clamp(value, 0, std::max(0, int(obs_source_get_width(source)) - crop.left - 1));
		} else if (field == 2) {
			crop.top =
				std::clamp(value, 0, std::max(0, int(obs_source_get_height(source)) - crop.bottom - 1));
		} else {
			crop.bottom =
				std::clamp(value, 0, std::max(0, int(obs_source_get_height(source)) - crop.top - 1));
		}
		obs_sceneitem_set_crop(item, &crop);
	});
}

void PenguinInspector::ChangeFlag(bool visibility, bool value)
{
	if (syncing) {
		return;
	}
	const auto item = CurrentItem();
	if (!item) {
		return;
	}
	const bool previous = visibility ? obs_sceneitem_visible(item) : obs_sceneitem_locked(item);
	if (previous == value) {
		return;
	}
	const std::string uuid = obs_source_get_uuid(obs_scene_get_source(obs_sceneitem_get_scene(item)));
	const int64_t id = obs_sceneitem_get_id(item);
	auto restore = [id, visibility](const std::string &sceneUuid, bool enabled) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(sceneUuid.c_str());
		if (!source) {
			return;
		}
		auto *scene = obs_group_or_scene_from_source(source);
		auto *item = scene ? obs_scene_find_sceneitem_by_id(scene, id) : nullptr;
		if (!item) {
			return;
		}
		if (visibility) {
			obs_sceneitem_set_visible(item, enabled);
		} else {
			obs_sceneitem_set_locked(item, enabled);
		}
	};
	main->undo_s.add_action(
		QTStr(visibility ? "Penguin.Inspector.UndoVisibility" : "Penguin.Inspector.UndoLock")
			.arg(obs_source_get_name(obs_sceneitem_get_source(item))),
		[restore, previous](const std::string &data) { restore(data, previous); },
		[restore, value](const std::string &data) { restore(data, value); }, uuid, uuid);
	restore(uuid, value);
	RefreshSelection();
}

void PenguinInspector::ChangeCamera()
{
	if (syncing) {
		return;
	}
	const auto item = CurrentItem();
	if (!item || obs_sceneitem_locked(item)) {
		return;
	}
	auto *source = obs_sceneitem_get_source(item);
	const auto undo = SaveCamera(source);
	auto *cameraBlur = findChild<QSpinBox *>("cameraBlur");
	const auto cameraImagePath = findChild<QPushButton *>("cameraImage")->property("backgroundPath").toString();
	if (!PenguinCamera::Apply(source, QT_TO_UTF8(QTStr("Penguin.Camera.Title")), cameraShape->currentIndex(),
				  cameraBackground->currentIndex(), cameraRadius->value(), cameraSimilarity->value(),
				  cameraBlur->value(), QT_TO_UTF8(cameraImagePath),
				  findChild<QCheckBox *>("cameraPreserveHands")->isChecked(),
				  findChild<QComboBox *>("cameraProcessing")->currentIndex() == 2   ? "cpu"
				  : findChild<QComboBox *>("cameraProcessing")->currentIndex() == 1 ? "gpu"
												    : "auto")) {
		cameraStatus->setText(QTStr("Penguin.Camera.Error"));
		return;
	}
	const auto redo = SaveCamera(source);
	if (undo != redo) {
		main->undo_s.add_action(QTStr("Penguin.Camera.Undo").arg(obs_source_get_name(source)), RestoreCamera,
					RestoreCamera, undo, redo);
	}
	main->SaveProject();
	RefreshSelection();
}
