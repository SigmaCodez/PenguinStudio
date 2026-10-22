// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <obs.hpp>

#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <array>
#include <functional>

class OBSBasic;
class QLabel;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QToolButton;

class PenguinInspector final : public QWidget {
	Q_OBJECT

	QPointer<OBSBasic> main;
	QTimer refreshTimer;
	QLabel *sourceName;
	QLabel *sourceType;
	QLabel *sourceIcon;
	QLabel *sourceSize;
	QLabel *emptyTitle;
	QLabel *emptyHint;
	QWidget *emptyState;
	QWidget *content;
	QWidget *transformCard;
	QWidget *cropCard;
	QWidget *cameraCard;
	QComboBox *cameraShape;
	QComboBox *cameraBackground;
	QSpinBox *cameraRadius;
	QSpinBox *cameraSimilarity;
	QLabel *cameraStatus;
	QToolButton *visible;
	QToolButton *locked;
	QToolButton *aspectLock;
	std::array<QDoubleSpinBox *, 5> transformFields{};
	std::array<QSpinBox *, 4> cropFields{};
	QString selectionToken;
	bool syncing = false;
	bool stopped = false;

	OBSSceneItem CurrentItem() const;
	void ApplyTransform(const std::function<void(obs_sceneitem_t *)> &edit);
	void ChangeTransform(size_t field, double value);
	void ChangeCrop(size_t field, int value);
	void ChangeFlag(bool visibility, bool value);
	void ChangeCamera();

public:
	explicit PenguinInspector(OBSBasic *main, QWidget *parent = nullptr);
	~PenguinInspector() override;

public slots:
	void RefreshSelection();
	void Shutdown();
};
