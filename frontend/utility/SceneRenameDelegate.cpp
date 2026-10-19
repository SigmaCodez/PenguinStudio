/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "SceneRenameDelegate.hpp"

#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QApplication>
#include <QStyle>
#include <QListView>
#include <algorithm>

#include "moc_SceneRenameDelegate.cpp"

SceneRenameDelegate::SceneRenameDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void SceneRenameDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QStyledItemDelegate::setEditorData(editor, index);
	QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
	if (lineEdit) {
		lineEdit->selectAll();
	}
}

bool SceneRenameDelegate::eventFilter(QObject *editor, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		switch (keyEvent->key()) {
		case Qt::Key_Escape: {
			QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
			if (lineEdit) {
				lineEdit->undo();
			}
			break;
		}
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			return false;
		}
	}

	return QStyledItemDelegate::eventFilter(editor, event);
}

// Scene rows carry a stable visual index while keeping the native editor and
// drag/drop model. The grid alternative continues to use its original delegate.
void SceneRenameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
			       const QModelIndex &index) const
{
	const auto *view = qobject_cast<const QListView *>(option.widget);
	if (!view || view->viewMode() == QListView::IconMode) {
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}
	QStyleOptionViewItem item(option);
	initStyleOption(&item, index);
	const QString text = item.text;
	item.text.clear();
	const auto *style = item.widget ? item.widget->style() : QApplication::style();
	style->drawControl(QStyle::CE_ItemViewItem, &item, painter, item.widget);
	const bool selected = item.state & QStyle::State_Selected;
	const QColor foreground = item.palette.color(selected ? QPalette::HighlightedText : QPalette::Text);
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing);
	const int badgeSize = std::max(26, item.fontMetrics.height() + 8);
	const QRect logicalBadge(item.rect.left() + 10, item.rect.center().y() - badgeSize / 2, badgeSize, badgeSize);
	const QRect badge = QStyle::visualRect(item.direction, item.rect, logicalBadge);
	QColor badgeColor = foreground;
	badgeColor.setAlpha(selected ? 30 : 15);
	painter->setPen(Qt::NoPen);
	painter->setBrush(badgeColor);
	painter->drawRoundedRect(badge, 6, 6);
	painter->setPen(foreground);
	painter->drawText(badge, Qt::AlignCenter, QString::number(index.row() + 1).rightJustified(2, QLatin1Char('0')));
	const QRect label = QStyle::visualRect(item.direction, item.rect, item.rect.adjusted(badgeSize + 20, 0, -12, 0));
	const auto alignment = item.direction == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft;
	painter->drawText(label, Qt::AlignVCenter | alignment,
			  item.fontMetrics.elidedText(text, Qt::ElideRight, label.width()));
	painter->restore();
}

QSize SceneRenameDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size = QStyledItemDelegate::sizeHint(option, index);
	const auto *view = qobject_cast<const QListView *>(option.widget);
	if (view && view->viewMode() == QListView::ListMode)
		size.setHeight(std::max(size.height(), option.fontMetrics.height() + 28));
	return size;
}
