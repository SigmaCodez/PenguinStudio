// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QAbstractItemView>

class CategoryPicker final : public QComboBox {
public:
	explicit CategoryPicker(QWidget *parent = nullptr) : QComboBox(parent)
	{
		setEditable(true);
		setInsertPolicy(QComboBox::NoInsert);
		setMaxVisibleItems(12);
		lineEdit()->setClearButtonEnabled(true);
		completer()->setCaseSensitivity(Qt::CaseInsensitive);
		completer()->setFilterMode(Qt::MatchContains);
		completer()->setCompletionMode(QCompleter::PopupCompletion);
	}
	QString Id() const
	{
		const int i = findText(currentText(), Qt::MatchFixedString);
		return i < 0 ? QString() : itemData(i).toString();
	}
	bool Valid() const { return currentText().trimmed().isEmpty() || !Id().isEmpty(); }
	void Select(const QString &id, const QString &name)
	{
		QSignalBlocker block(this);
		int i = findData(id);
		if (i < 0) {
			addItem(name.isEmpty() ? id : name, id);
			i = count() - 1;
		}
		setCurrentIndex(i);
	}
	void Choices(const QList<QPair<QString, QString>> &items, bool showSuggestions)
	{
		const auto edited = currentText();
		QSignalBlocker block(this);
		clear();
		for (const auto &item : items) {
			addItem(item.first, item.second);
		}
		setCurrentIndex(-1);
		setEditText(edited);
		completer()->setCompletionPrefix(edited);
		if (showSuggestions && lineEdit()->hasFocus() && !items.isEmpty()) {
			completer()->complete();
		}
	}
};
