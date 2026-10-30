#pragma once

#include <obs.hpp>

#include <QWidget>
#include <QPointer>
#include <QWindow>

#define GREY_COLOR_BACKGROUND 0xFF4C4C4C

class OBSQTDisplay : public QWidget {
	Q_OBJECT
	Q_PROPERTY(QColor displayBackgroundColor MEMBER backgroundColor READ GetDisplayBackgroundColor WRITE
			   SetDisplayBackgroundColor)

	OBSDisplay display;
	bool destroying = false;
	QPointer<QWindow> displayWindow;
	QPointer<QObject> surfaceFilter;
	void BindDisplayWindow();

protected:
	bool event(QEvent *event) override;
	virtual void paintEvent(QPaintEvent *event) override;
	virtual void moveEvent(QMoveEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
	void DisplayCreated(OBSQTDisplay *window);
	void DisplayResized();

public:
	OBSQTDisplay(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
	~OBSQTDisplay() { display = nullptr; }

	virtual QPaintEngine *paintEngine() const override;

	inline obs_display_t *GetDisplay() const { return display; }

	uint32_t backgroundColor = GREY_COLOR_BACKGROUND;

	QColor GetDisplayBackgroundColor() const;
	void SetDisplayBackgroundColor(const QColor &color);
	void UpdateDisplayBackgroundColor();
	void CreateDisplay();
	void DestroyDisplay()
	{
		display = nullptr;
		destroying = true;
	};

	// Qt can recreate native surfaces when the first accelerated child (for
	// example WebEngine chat) switches the top-level window's rendering mode.
	void RestoreDisplay()
	{
		destroying = false;
		CreateDisplay();
	}

	void OnMove();
	void OnDisplayChange();
};
