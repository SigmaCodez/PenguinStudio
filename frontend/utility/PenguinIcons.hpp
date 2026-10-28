// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>
#include <QRectF>

namespace PenguinStudio {

enum class Icon {
	Camera,
	Layers,
	Scene,
	Plus,
	Settings,
	Live,
	Record,
	Clip,
	Inspect,
	More,
	Chevron,
	Monitor,
	Portrait,
	Audio,
	Folder,
	Reset,
	Link,
	Search,
	Fit,
	Center,
	FlipHorizontal,
	FlipVertical,
	Privacy
};

// Original 24-unit line icons. Rendering both resolutions keeps the studio
// toolbar sharp on standard and HiDPI displays without a platform icon theme.
inline QIcon StudioIcon(Icon glyph, QColor color = QColor("#BDC0CB"), int size = 20)
{
	QIcon icon;
	for (int scale : {1, 2}) {
		QPixmap pixmap(size * scale, size * scale);
		pixmap.fill(Qt::transparent);
		QPainter p(&pixmap);
		p.setRenderHint(QPainter::Antialiasing);
		p.scale(qreal(size * scale) / 24.0, qreal(size * scale) / 24.0);
		p.setPen(QPen(color, 1.65, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		p.setBrush(Qt::NoBrush);
		auto line = [&](qreal x1, qreal y1, qreal x2, qreal y2) {
			p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
		};
		switch (glyph) {
		case Icon::Privacy: {
			QPainterPath shield;
			shield.moveTo(12, 3);
			shield.lineTo(20, 6);
			shield.lineTo(20, 12);
			shield.quadTo(20, 18, 12, 22);
			shield.quadTo(4, 18, 4, 12);
			shield.lineTo(4, 6);
			shield.closeSubpath();
			p.drawPath(shield);
			line(8, 12, 11, 15);
			line(11, 15, 16, 9);
			break;
		}
		case Icon::Camera:
			p.drawRoundedRect(QRectF(3, 6, 13, 12), 2, 2);
			p.drawPolygon(QPolygonF({QPointF(16, 10), QPointF(21, 7), QPointF(21, 17), QPointF(16, 14)}));
			break;
		case Icon::Layers:
			p.drawPolygon(QPolygonF({QPointF(12, 3), QPointF(22, 8), QPointF(12, 13), QPointF(2, 8)}));
			p.drawPolyline(QPolygonF({QPointF(3, 12), QPointF(12, 17), QPointF(21, 12)}));
			p.drawPolyline(QPolygonF({QPointF(3, 16), QPointF(12, 21), QPointF(21, 16)}));
			break;
		case Icon::Scene:
			p.drawRoundedRect(QRectF(3, 4, 18, 16), 2, 2);
			line(3, 9, 21, 9);
			line(8, 9, 8, 20);
			break;
		case Icon::Plus:
			line(12, 5, 12, 19);
			line(5, 12, 19, 12);
			break;
		case Icon::Settings:
			p.drawEllipse(QPointF(12, 12), 6.5, 6.5);
			p.drawEllipse(QPointF(12, 12), 2.5, 2.5);
			for (int i = 0; i < 8; ++i) {
				p.save();
				p.translate(12, 12);
				p.rotate(i * 45.0);
				line(0, -7, 0, -10);
				p.restore();
			}
			break;
		case Icon::Live:
			p.drawEllipse(QPointF(12, 12), 2, 2);
			p.drawArc(QRectF(6, 6, 12, 12), -55 * 16, 110 * 16);
			p.drawArc(QRectF(6, 6, 12, 12), 125 * 16, 110 * 16);
			p.drawArc(QRectF(2, 2, 20, 20), -50 * 16, 100 * 16);
			p.drawArc(QRectF(2, 2, 20, 20), 130 * 16, 100 * 16);
			break;
		case Icon::Record:
			p.drawEllipse(QPointF(12, 12), 8, 8);
			p.setBrush(color);
			p.drawEllipse(QPointF(12, 12), 3.5, 3.5);
			break;
		case Icon::Clip:
			p.drawRoundedRect(QRectF(4, 4, 16, 16), 2, 2);
			line(4, 9, 20, 9);
			line(9, 4, 6, 9);
			line(15, 4, 12, 9);
			p.drawPolygon(QPolygonF({QPointF(10, 12), QPointF(15, 15), QPointF(10, 18)}));
			break;
		case Icon::Inspect:
			line(5, 4, 5, 20);
			line(12, 4, 12, 20);
			line(19, 4, 19, 20);
			p.setBrush(QColor("#1D1F25"));
			p.drawRoundedRect(QRectF(3, 7, 4, 4), 1, 1);
			p.drawRoundedRect(QRectF(10, 13, 4, 4), 1, 1);
			p.drawRoundedRect(QRectF(17, 6, 4, 4), 1, 1);
			break;
		case Icon::More:
			p.setPen(Qt::NoPen);
			p.setBrush(color);
			for (int x : {5, 12, 19})
				p.drawEllipse(QPointF(x, 12), 1.5, 1.5);
			break;
		case Icon::Chevron:
			p.drawPolyline(QPolygonF({QPointF(6, 9), QPointF(12, 15), QPointF(18, 9)}));
			break;
		case Icon::Monitor:
			p.drawRoundedRect(QRectF(2.5, 3.5, 19, 13), 2, 2);
			line(12, 17, 12, 21);
			line(8, 21, 16, 21);
			break;
		case Icon::Portrait:
			p.drawRoundedRect(QRectF(6, 2, 12, 20), 2, 2);
			line(10, 18.5, 14, 18.5);
			break;
		case Icon::Audio:
			line(4, 9, 4, 15);
			line(8, 5, 8, 19);
			line(12, 2, 12, 22);
			line(16, 7, 16, 17);
			line(20, 10, 20, 14);
			break;
		case Icon::Folder: {
			QPainterPath path;
			path.moveTo(3, 6);
			path.lineTo(9, 6);
			path.lineTo(11, 9);
			path.lineTo(21, 9);
			path.lineTo(21, 19);
			path.lineTo(3, 19);
			path.closeSubpath();
			p.drawPath(path);
			break;
		}
		case Icon::Reset:
			p.drawArc(QRectF(5, 5, 15, 15), -50 * 16, 285 * 16);
			p.drawPolyline(QPolygonF({QPointF(3, 4), QPointF(3, 10), QPointF(9, 10)}));
			break;
		case Icon::Search:
			p.drawEllipse(QPointF(10, 10), 6, 6);
			line(14.5, 14.5, 21, 21);
			break;
		case Icon::Fit:
			p.drawRect(QRectF(7, 8, 10, 8));
			p.drawPolyline(QPolygonF({QPointF(3, 8), QPointF(3, 3), QPointF(8, 3)}));
			p.drawPolyline(QPolygonF({QPointF(16, 3), QPointF(21, 3), QPointF(21, 8)}));
			p.drawPolyline(QPolygonF({QPointF(3, 16), QPointF(3, 21), QPointF(8, 21)}));
			p.drawPolyline(QPolygonF({QPointF(16, 21), QPointF(21, 21), QPointF(21, 16)}));
			break;
		case Icon::Center:
			p.drawRect(QRectF(7, 7, 10, 10));
			line(12, 2, 12, 6);
			line(12, 18, 12, 22);
			line(2, 12, 6, 12);
			line(18, 12, 22, 12);
			break;
		case Icon::FlipHorizontal:
			line(12, 3, 12, 21);
			p.drawPolygon(QPolygonF({QPointF(3, 17), QPointF(8, 7), QPointF(8, 17)}));
			p.drawPolygon(QPolygonF({QPointF(21, 17), QPointF(16, 7), QPointF(16, 17)}));
			break;
		case Icon::FlipVertical:
			line(3, 12, 21, 12);
			p.drawPolygon(QPolygonF({QPointF(7, 3), QPointF(17, 8), QPointF(7, 8)}));
			p.drawPolygon(QPolygonF({QPointF(7, 21), QPointF(17, 16), QPointF(7, 16)}));
			break;
		case Icon::Link: {
			QPainterPath path;
			path.moveTo(10, 8);
			path.lineTo(13, 5);
			path.cubicTo(18, 0, 24, 6, 19, 11);
			path.lineTo(16, 14);
			path.moveTo(8, 10);
			path.lineTo(5, 13);
			path.cubicTo(0, 18, 6, 24, 11, 19);
			path.lineTo(14, 16);
			p.drawPath(path);
			line(8, 16, 16, 8);
			break;
		}
		}
		p.end();
		pixmap.setDevicePixelRatio(scale);
		icon.addPixmap(pixmap);
	}
	return icon;
}

inline QIcon TrayStatusIcon(bool paused)
{
	QPixmap pixmap(64, 64);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.drawPixmap(0, 0, 64, 64, QPixmap(":/res/images/penguin-studio.png"));
	painter.setPen(QPen(QColor("#101A15"), 3));
	painter.setBrush(paused ? QColor("#F6BE4F") : QColor("#59E6B3"));
	painter.drawEllipse(QRectF(39, 39, 23, 23));
	if (paused) {
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor("#10241A"));
		painter.drawRect(QRectF(45, 44, 4, 12));
		painter.drawRect(QRectF(52, 44, 4, 12));
	}
	painter.end();
	return QIcon(pixmap);
}

} // namespace PenguinStudio
