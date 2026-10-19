// SPDX-License-Identifier: GPL-2.0-or-later
#include <frontend/utility/OBSTranslator.hpp>

#include <QHash>
#include <QStringList>

#include <cassert>
#include <iostream>

int main()
{
	QHash<QString, QString> catalogue{{"Cancel", "Abbrechen"}, {"Apply", "Übernehmen"},
					{"Save&Close", "Speichern & schließen"}, {"SaveClose", "Speichern und schließen"}};
	QStringList calls;
	auto translate = [&](const char *context, const char *source) {
		calls.clear();
		return OBSQtTranslation::Translate(context, source, [&](const QString &key) -> std::optional<QString> {
			calls.append(key);
			const auto found = catalogue.constFind(key);
			return found == catalogue.cend() ? std::nullopt : std::optional<QString>(*found);
		});
	};

	assert(translate("AnyApplicationDialog", "Cancel") == "Abbrechen");
	assert(calls == QStringList{"Cancel"});
	for (const char *context : {"QGnomeTheme", "QPlatformTheme", "QDialogButtonBox"}) {
		assert(translate(context, "&Cancel") == "Abbrechen");
		assert(calls == (QStringList{"&Cancel", "Cancel"}));
		assert(translate(context, "A&pply") == "Übernehmen");
	}

	// An exact catalogue entry always wins, including an accelerator in its key.
	catalogue.insert("&Cancel", "Exact accelerator entry");
	assert(translate("QGnomeTheme", "&Cancel") == "Exact accelerator entry");
	assert(calls == QStringList{"&Cancel"});
	catalogue.remove("&Cancel");

	// Escaped && means one literal ampersand; spaces must not merge mnemonics.
	assert(translate("QGnomeTheme", "&Save && C&lose") == "Speichern & schließen");
	assert(calls == (QStringList{"&Save&&C&lose", "Save&Close"}));
	assert(translate("QGnomeTheme", "&Save & &Close") == "Speichern und schließen");
	assert(calls == (QStringList{"&Save&&Close", "SaveClose"}));

	for (const char *context : {"OBSBasicSettings", "QLineEdit", "QGnomeThemeExtra", ""}) {
		assert(translate(context, "&Cancel") == "&Cancel");
		assert(calls == QStringList{"&Cancel"});
	}
	assert(translate(nullptr, "&Cancel") == "&Cancel");
	assert(translate("QGnomeTheme", "&Unknown && label") == "&Unknown && label");
	assert(translate("QGnomeTheme", "Unknown label") == "Unknown label");
	assert(calls == QStringList{"Unknownlabel"});
	assert(translate("QGnomeTheme", "&") == "&");
	assert(calls == QStringList{"&"});

	std::cout << "Qt standard-button translation: exact lookup, scoped mnemonics, escaped ampersands, and unknown text passed.\n";
}
