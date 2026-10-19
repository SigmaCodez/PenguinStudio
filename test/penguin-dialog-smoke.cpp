// SPDX-License-Identifier: GPL-2.0-or-later
// Optional LD_PRELOAD GUI regression helper. Use only an isolated test profile.
// Captures are Qt widget renders; they do not validate native video surfaces.
#include <obs.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionFrame>
#include <QStyleOptionSpinBox>
#include <QTimer>

namespace {
QPointer<QMainWindow> mainWindow;
obs_source_t *source = nullptr;
QJsonArray reports;
bool scrolled = false;
int ticks = 0;

QDir Results()
{
	return QDir(QString::fromLocal8Bit(qgetenv("PENGUIN_DIALOG_DIR")));
}

QDialog *FindDialog(const char *type)
{
	for (auto *widget : QApplication::topLevelWidgets()) {
		auto *dialog = qobject_cast<QDialog *>(widget);
		if (dialog && dialog->isVisible() && QByteArray(dialog->metaObject()->className()) == type)
			return dialog;
	}
	return nullptr;
}

void SaveReport()
{
	QFile file(Results().filePath(QStringLiteral("dialog-report.json")));
	if (file.open(QIODevice::WriteOnly))
		file.write(QJsonDocument(reports).toJson());
}

void CaptureInput(QWidget *widget, QJsonArray &inputs)
{
	QRect content;
	if (auto *combo = qobject_cast<QComboBox *>(widget)) {
		QStyleOptionComboBox option;
		option.initFrom(combo);
		option.currentText = combo->currentText();
		option.editable = combo->isEditable();
		content = combo->style()->subControlRect(QStyle::CC_ComboBox, &option,
						       QStyle::SC_ComboBoxEditField, combo);
	} else if (auto *spin = qobject_cast<QAbstractSpinBox *>(widget)) {
		QStyleOptionSpinBox option;
		option.initFrom(spin);
		option.frame = spin->hasFrame();
		option.buttonSymbols = spin->buttonSymbols();
		content = spin->style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, spin);
	} else if (auto *edit = qobject_cast<QLineEdit *>(widget);
		   edit && !qobject_cast<QComboBox *>(edit->parentWidget()) &&
		   !qobject_cast<QAbstractSpinBox *>(edit->parentWidget())) {
		QStyleOptionFrame option;
		option.initFrom(edit);
		content = edit->style()->subElementRect(QStyle::SE_LineEditContents, &option, edit);
	} else {
		return;
	}
	inputs.append(QJsonObject{{"name", widget->objectName()},
				 {"class", QString::fromLatin1(widget->metaObject()->className())},
				 {"contentHeight", content.height()}, {"fontHeight", widget->fontMetrics().height()},
				 {"passed", content.height() >= widget->fontMetrics().height()}});
}

void Capture(QDialog *dialog, const QString &name)
{
	QJsonObject report{{"name", name}, {"found", dialog != nullptr}};
	if (!dialog) {
		reports.append(report);
		SaveReport();
		return;
	}
	dialog->raise();
	QJsonArray widgets;
	QJsonArray inputs;
	for (auto *widget : dialog->findChildren<QWidget *>()) {
		if (!widget->isVisible())
			continue;
		const QRect bounds(widget->mapTo(dialog, QPoint()), widget->size());
		QString text;
		if (auto *button = qobject_cast<QAbstractButton *>(widget))
			text = button->text();
		if (auto *edit = qobject_cast<QLineEdit *>(widget))
			text = edit->text();
		CaptureInput(widget, inputs);
		widgets.append(QJsonObject{{"name", widget->objectName()},
					  {"class", QString::fromLatin1(widget->metaObject()->className())},
					  {"text", text}, {"x", bounds.x()}, {"y", bounds.y()},
					  {"width", bounds.width()}, {"height", bounds.height()},
					  {"minimumWidth", widget->minimumWidth()},
					  {"minimumSizeHintWidth", widget->minimumSizeHint().width()},
					  {"layoutMinimumWidth", widget->layout() ?
						  widget->layout()->minimumSize().width() : -1}});
	}
	report.insert("widgets", widgets);
	report.insert("inputs", inputs);
	report.insert("platform", QGuiApplication::platformName());
	report.insert("configuredFontScale",
		      int(config_get_int(obs_frontend_get_user_config(), "Appearance", "FontScale")));
	report.insert("captureMethod", "qt-widget-render");
	report.insert("title", dialog->windowTitle());
	report.insert("width", dialog->width());
	report.insert("height", dialog->height());
	const QRect available = dialog->screen()->availableGeometry();
	report.insert("screenName", dialog->screen()->name());
	report.insert("screenAvailableGeometry", QJsonObject{
		{"x", available.x()}, {"y", available.y()},
		{"width", available.width()}, {"height", available.height()}});
	report.insert("mainWindowScreenName", mainWindow->screen()->name());
	report.insert("mainWindowScreenWidth", mainWindow->screen()->availableGeometry().width());
	QJsonArray horizontalOverflow;
	for (auto *area : dialog->findChildren<QAbstractScrollArea *>()) {
		if (area->isVisible() && area->horizontalScrollBar()->maximum() > 0)
			horizontalOverflow.append(QJsonObject{{"name", area->objectName()},
							     {"maximum", area->horizontalScrollBar()->maximum()},
							     {"viewportWidth", area->viewport()->width()}});
	}
	report.insert("horizontalOverflow", horizontalOverflow);
	if (auto *navigation = dialog->findChild<QListWidget *>("listWidget")) {
		const int labelWidth = navigation->sizeHintForColumn(0);
		const int availableWidth = navigation->viewport()->width();
		report.insert("navigationLabelWidth", labelWidth);
		report.insert("navigationAvailableWidth", availableWidth);
		report.insert("navigationFits", labelWidth <= availableWidth);
	}
	if (name == QStringLiteral("settings-advanced")) {
		auto *preserve = dialog->findChild<QCheckBox *>("streamDelayPreserve");
		auto *enable = dialog->findChild<QCheckBox *>("streamDelayEnable");
		QJsonObject check{{"found", preserve && enable}};
		if (preserve && enable) {
			QStyleOptionButton option;
			option.initFrom(preserve);
			option.text = preserve->text();
			const QRect content = preserve->style()->subElementRect(QStyle::SE_CheckBoxContents,
								      &option, preserve);
			const QSize textSize = preserve->fontMetrics().size(Qt::TextShowMnemonic, preserve->text());
			const bool enabledBefore = enable->isChecked();
			const bool checkedBefore = preserve->isChecked();
			if (!enabledBefore)
				enable->click();
			const bool active = preserve->isEnabled();
			preserve->click();
			const bool toggled = preserve->isChecked() != checkedBefore;
			preserve->click();
			const bool restored = preserve->isChecked() == checkedBefore;
			if (!enabledBefore)
				enable->click();
			const bool captionFits = textSize.width() <= content.width() &&
						 textSize.height() <= content.height();
			const bool accessible = preserve->accessibleName().simplified() == preserve->text().simplified();
			const int lineCount = preserve->text().count(QLatin1Char('\n')) + 1;
			const bool heightFits = preserve->height() >= lineCount * preserve->fontMetrics().height();
			check.insert("text", preserve->text());
			check.insert("accessibleName", preserve->accessibleName());
			check.insert("height", preserve->height());
			check.insert("fontHeight", preserve->fontMetrics().height());
			check.insert("lineCount", lineCount);
			check.insert("captionFits", captionFits);
			check.insert("passed", active && toggled && restored && captionFits && accessible && heightFits);
			check.insert("captured", preserve->grab().save(Results().filePath("stream-delay-checkbox.png")));
		}
		report.insert("streamDelayPreserve", check);
	}
	const bool saved = dialog->grab().save(Results().filePath(name + QStringLiteral(".png")));
	report.insert("captured", saved);
	reports.append(report);
	SaveReport();
	qInfo("Dialog capture %s: %s", qPrintable(name), saved ? "PASS" : "FAIL");
}

bool ScrollToBottom(QDialog *dialog)
{
	if (!dialog)
		return false;
	bool moved = false;
	for (auto *area : dialog->findChildren<QAbstractScrollArea *>()) {
		if (area->isVisible() && area->verticalScrollBar()->maximum() > 0) {
			area->verticalScrollBar()->setValue(area->verticalScrollBar()->maximum());
			moved = true;
		}
	}
	return moved;
}

void Click(const char *name)
{
	if (auto *button = mainWindow->findChild<QAbstractButton *>(QString::fromLatin1(name)))
		button->click();
}

void CreateFixture()
{
	obs_source_t *current = obs_frontend_get_current_scene();
	auto *scene = obs_scene_from_source(current);
	auto *settings = obs_data_create();
	obs_data_set_string(settings, "file", qgetenv("PENGUIN_DIALOG_IMAGE").constData());
	source = obs_source_create("image_source", "Penguin Studio logo", settings, nullptr);
	if (scene && source) {
		if (auto *item = obs_scene_add(scene, source))
			obs_sceneitem_select(item, true);
	}
	obs_data_release(settings);
	settings = obs_data_create();
	auto *filter = obs_source_create("color_filter_v2", "Color correction", settings, nullptr);
	if (source && filter)
		obs_source_filter_add(source, filter);
	obs_source_release(filter);
	obs_data_release(settings);
	obs_source_release(current);
}

void Tick()
{
	++ticks;
	if (!mainWindow) {
		for (auto *widget : QApplication::topLevelWidgets())
			if (QByteArray(widget->metaObject()->className()) == "OBSBasic")
				mainWindow = qobject_cast<QMainWindow *>(widget);
	}
	if (!mainWindow)
		return;
	if (ticks < 24) {
		for (auto *widget : QApplication::topLevelWidgets())
			if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible())
				dialog->reject();
	}
	if (ticks == 24) {
		mainWindow->showNormal();
		mainWindow->resize(1280, 800);
		CreateFixture();
	}
	if (ticks == 28 && source)
		QTimer::singleShot(0, qApp, [] { Click("inspectorProperties"); });
	if (ticks == 36)
		Capture(FindDialog("OBSBasicProperties"), QStringLiteral("source-properties"));
	if (ticks == 40)
		if (auto *dialog = FindDialog("OBSBasicProperties"))
			dialog->reject();
	if (ticks == 44 && source)
		QTimer::singleShot(0, qApp, [] { Click("inspectorFilters"); });
	if (ticks == 52)
		Capture(FindDialog("OBSBasicFilters"), QStringLiteral("source-filters"));
	if (ticks == 53)
		scrolled = ScrollToBottom(FindDialog("OBSBasicFilters"));
	if (ticks == 55 && scrolled)
		Capture(FindDialog("OBSBasicFilters"), QStringLiteral("source-filters-bottom"));
	if (ticks == 56)
		if (auto *dialog = FindDialog("OBSBasicFilters"))
			dialog->reject();
	if (ticks == 60)
		QTimer::singleShot(0, qApp, [] { Click("penguinSettingsButton"); });
	const char *pages[] = {"general", "appearance", "stream", "output", "audio", "video",
			       "hotkeys", "accessibility", "advanced"};
	if (ticks >= 68 && ticks <= 132 && (ticks - 68) % 8 == 0) {
		if (auto *dialog = FindDialog("OBSBasicSettings"))
			if (auto *list = dialog->findChild<QListWidget *>("listWidget"))
				list->setCurrentRow((ticks - 68) / 8);
	}
	if (ticks >= 72 && ticks <= 136 && (ticks - 72) % 8 == 0)
		Capture(FindDialog("OBSBasicSettings"),
			QStringLiteral("settings-") + QString::fromLatin1(pages[(ticks - 72) / 8]));
	if (ticks >= 73 && ticks <= 137 && (ticks - 73) % 8 == 0)
		scrolled = ScrollToBottom(FindDialog("OBSBasicSettings"));
	if (ticks >= 75 && ticks <= 139 && (ticks - 75) % 8 == 0 && scrolled)
		Capture(FindDialog("OBSBasicSettings"), QStringLiteral("settings-") +
			QString::fromLatin1(pages[(ticks - 75) / 8]) + QStringLiteral("-bottom"));
	if (ticks == 144) {
		if (auto *dialog = FindDialog("OBSBasicSettings"))
			if (auto *buttons = dialog->findChild<QDialogButtonBox *>("buttonBox"))
				if (auto *cancel = buttons->button(QDialogButtonBox::Cancel))
					cancel->click();
		obs_source_release(source);
		source = nullptr;
		SaveReport();
		mainWindow->close();
	}
}

void Start()
{
	if (qEnvironmentVariableIsEmpty("PENGUIN_DIALOG_DIR"))
		return;
	Results().mkpath(QStringLiteral("."));
	QTimer::singleShot(1000, qApp, [] {
		auto *timer = new QTimer(qApp);
		QObject::connect(timer, &QTimer::timeout, Tick);
		timer->start(250);
	});
}
Q_COREAPP_STARTUP_FUNCTION(Start)
} // namespace
