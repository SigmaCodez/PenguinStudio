// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QDateTime>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>
#include <functional>
#include <algorithm>

namespace PenguinChat {
class MessageDelegate final : public QStyledItemDelegate {
	static qreal Layout(QTextLayout &layout, qreal width)
	{
		QTextOption options;
		options.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
		layout.setTextOption(options);
		qreal height = 0;
		layout.beginLayout();
		for (;;) {
			auto line = layout.createLine();
			if (!line.isValid()) {
				break;
			}
			line.setLineWidth(std::max(40.0, width));
			line.setPosition(QPointF(0, height));
			height += line.height();
		}
		layout.endLayout();
		return height;
	}

public:
	explicit MessageDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
	{
		const auto *list = qobject_cast<const QListWidget *>(parent());
		const int width = list ? list->viewport()->width() - 16 : option.rect.width();
		QTextLayout layout(index.data(Qt::UserRole + 3).toString(), option.font);
		return QSize(width, int(Layout(layout, width - 24)) + option.fontMetrics.height() + 30);
	}
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
	{
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		const QRect rect = option.rect.adjusted(2, 1, -2, -1);
		painter->setPen(Qt::NoPen);
		painter->setBrush(option.palette.color(
			option.state & QStyle::State_Selected ? QPalette::Highlight : QPalette::AlternateBase));
		painter->drawRoundedRect(rect, 8, 8);
		const QColor color = option.palette.color(
			option.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text);
		painter->setPen(color);
		auto bold = option.font;
		bold.setBold(true);
		painter->setFont(bold);
		const QString header = index.data(Qt::UserRole + 2).toString();
		painter->drawText(rect.adjusted(12, 8, -12, 0), Qt::AlignTop | Qt::AlignLeft,
				  QFontMetrics(bold).elidedText(header, Qt::ElideRight, rect.width() - 24));
		painter->setFont(option.font);
		QTextLayout layout(index.data(Qt::UserRole + 3).toString(), option.font);
		Layout(layout, rect.width() - 24);
		layout.draw(painter, QPointF(rect.x() + 12, rect.y() + option.fontMetrics.height() + 13));
		painter->restore();
	}
};
// Store identifiers separately from display text. Never parse a screen name to authorize an action.
inline void Style(QWidget *widget, QListWidget *history)
{
	history->setObjectName("penguinChatHistory");
	history->setWordWrap(true);
	history->setItemDelegate(new MessageDelegate(history));
	history->setSpacing(6);
	history->setAlternatingRowColors(true);
	history->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	history->setContextMenuPolicy(Qt::CustomContextMenu);
	history->setAccessibleName("Chat messages — select a message for moderation");
	widget->setProperty("penguinChatPanel", true);
	if (auto *layout = widget->layout()) {
		layout->setContentsMargins(12, 12, 12, 12);
		layout->setSpacing(8);
	}
}
inline void Participants(QWidget *parent, const QString &title, const QString &explanation, const QStringList &names)
{
	auto *dialog = new QDialog(parent);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(title);
	dialog->resize(360, 480);
	auto *layout = new QVBoxLayout(dialog);
	auto *hint = new QLabel(explanation, dialog);
	hint->setTextFormat(Qt::PlainText);
	hint->setWordWrap(true);
	layout->addWidget(hint);
	auto *search = new QLineEdit(dialog);
	search->setPlaceholderText("Search participants");
	layout->addWidget(search);
	auto *list = new QListWidget(dialog);
	list->addItems(names);
	list->sortItems();
	layout->addWidget(list);
	QObject::connect(search, &QLineEdit::textChanged, list, [list](const QString &query) {
		for (int i = 0; i < list->count(); ++i) {
			list->item(i)->setHidden(!list->item(i)->text().contains(query, Qt::CaseInsensitive));
		}
	});
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
	layout->addWidget(buttons);
	QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	dialog->show();
}
inline void SessionBans(QWidget *parent, const QMap<QString, QString> &users, const std::function<void(QString)> &unban)
{
	auto *dialog = new QDialog(parent);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle("Bans and timeouts from this session");
	dialog->resize(380, 420);
	auto *layout = new QVBoxLayout(dialog);
	auto *hint = new QLabel(
		"Only actions completed in this session are listed. Use the platform dashboard for older bans.",
		dialog);
	hint->setWordWrap(true);
	layout->addWidget(hint);
	auto *list = new QListWidget(dialog);
	layout->addWidget(list);
	for (auto it = users.cbegin(); it != users.cend(); ++it) {
		auto *item = new QListWidgetItem(it.value(), list);
		item->setData(Qt::UserRole, it.key());
	}
	auto *remove = new QPushButton("Remove selected ban / timeout", dialog);
	remove->setEnabled(false);
	layout->addWidget(remove);
	QObject::connect(list, &QListWidget::currentRowChanged, remove,
			 [remove](int row) { remove->setEnabled(row >= 0); });
	QObject::connect(remove, &QPushButton::clicked, dialog, [dialog, list, unban] {
		if (list->currentItem()) {
			const auto target = list->currentItem()->data(Qt::UserRole).toString();
			dialog->accept();
			unban(target);
		}
	});
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
	layout->addWidget(buttons);
	QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	dialog->show();
}
inline void MessageMenu(QWidget *parent, QListWidget *history, const QPoint &point,
			const std::function<void(QString, QString, QString)> &action)
{
	auto *item = point.x() < 0 ? history->currentItem() : history->itemAt(point);
	if (!item || item->data(Qt::UserRole).toString().isEmpty()) {
		return;
	}
	const QString id = item->data(Qt::UserRole).toString();
	const QString user = item->data(Qt::UserRole + 1).toString();
	QMenu menu(parent);
	menu.addAction("Delete message", [=] { action("delete", id, user); });
	if (!user.isEmpty()) {
		menu.addAction("Timeout for 10 minutes…", [=] { action("timeout", id, user); });
		menu.addAction("Ban from chat…", [=] { action("ban", id, user); });
		menu.addAction("Remove ban / timeout…", [=] { action("unban", id, user); });
	}
	menu.exec(history->viewport()->mapToGlobal(point));
}
inline bool Confirm(QWidget *parent, const QString &action)
{
	return QMessageBox::question(parent, "Chat moderation", action + "?", QMessageBox::Yes | QMessageBox::Cancel,
				     QMessageBox::Cancel) == QMessageBox::Yes;
}
} // namespace PenguinChat
