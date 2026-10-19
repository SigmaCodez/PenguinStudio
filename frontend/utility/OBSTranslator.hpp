/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#pragma once

#include <QString>
#include <QTranslator>

#include <cstring>
#include <optional>

namespace OBSQtTranslation {
// Qt platform themes may add keyboard mnemonics to standard button labels.
// Keep the catalogue's existing lookup first and leave application text alone.
template<typename Lookup> QString Translate(const char *context, const char *sourceText, Lookup lookup)
{
	const QString original = QString::fromUtf8(sourceText);
	QString key = original;
	key.remove(QLatin1Char(' '));
	if (const auto translated = lookup(key))
		return *translated;

	const bool standardButton = context && (std::strcmp(context, "QGnomeTheme") == 0 ||
					       std::strcmp(context, "QPlatformTheme") == 0 ||
					       std::strcmp(context, "QDialogButtonBox") == 0);
	if (!standardButton || !original.contains(QLatin1Char('&')))
		return original;

	QString plain;
	plain.reserve(original.size());
	for (qsizetype index = 0; index < original.size(); ++index) {
		if (original[index] != QLatin1Char('&')) {
			plain += original[index];
		} else if (index + 1 < original.size() && original[index + 1] == QLatin1Char('&')) {
			plain += QLatin1Char('&');
			++index;
		}
	}
	plain.remove(QLatin1Char(' '));
	if (!plain.isEmpty() && plain != key)
		if (const auto translated = lookup(plain))
			return *translated;
	return original;
}
} // namespace OBSQtTranslation

class OBSTranslator : public QTranslator {
	Q_OBJECT

public:
	virtual bool isEmpty() const override { return false; }

	virtual QString translate(const char *context, const char *sourceText, const char *disambiguation,
				  int n) const override;
};
