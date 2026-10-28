#include "StatusBarWidget.hpp"
#include "ui_StatusBarWidget.h"
#include "moc_StatusBarWidget.cpp"
#include <QEvent>

StatusBarWidget::StatusBarWidget(QWidget *parent) : QWidget(parent), ui(new Ui::StatusBarWidget)
{
	ui->setupUi(this);
	setMinimumHeight(fontMetrics().height() + 24);
	ui->horizontalLayout->setContentsMargins(12, 6, 12, 6);
	ui->horizontalLayout->setSpacing(8);
	for (auto *frame : {ui->networkFrame, ui->streamFrame, ui->recordFrame,
			   ui->cpuFrame, ui->gpuFrame, ui->fpsFrame, ui->delayFrame, ui->issuesFrame}) {
		frame->layout()->setContentsMargins(10, 5, 10, 5);
		frame->layout()->setSpacing(6);
	}
	for (auto *label : {ui->streamTime, ui->recordTime, ui->cpuUsage, ui->gpuUsage, ui->fpsCurrent})
		label->setAlignment(Qt::AlignCenter);
	ui->networkFrame->hide();
	ui->streamFrame->hide();
	ui->recordFrame->hide();
}

void StatusBarWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);
	if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)
		setMinimumHeight(fontMetrics().height() + 24);
}

StatusBarWidget::~StatusBarWidget() {}
