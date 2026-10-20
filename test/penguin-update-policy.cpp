// SPDX-License-Identifier: GPL-2.0-or-later
#include "../frontend/utility/PenguinUpdatePolicy.hpp"
#include <cstdlib>
#include <iostream>

void Require(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}
int main()
{
	using namespace PenguinUpdates;
	const auto name = PackageName("0.3.0", "ubuntu26.04");
	const QJsonObject asset{
		{"name", name},
		{"state", "uploaded"},
		{"size", 150000000},
		{"digest", "sha256:" + QString(64, 'a')},
		{"browser_download_url",
		 "https://github.com/SigmaCodez/PenguinStudio/releases/download/penguin-v0.3.0/" + name}};
	const QJsonObject release{{"tag_name", "penguin-v0.3.0"}, {"prerelease", true}, {"assets", QJsonArray{asset}}};
	Require(Select({release}, "0.2.1", "ubuntu26.04", true).name == name, "compatible preview not selected");
	Require(Select({release}, "0.2.1", "ubuntu24.04", true).name.isEmpty(), "wrong distro accepted");
	Require(Select({release}, "0.2.1", "", true).name.isEmpty(), "unknown installation offered native update");
	Require(Select({release}, "0.2.1", "ubuntu26.04", false).version.isEmpty(),
		"preview ignored channel preference");
	Require(Select({release}, "0.3.0", "ubuntu26.04", true).version.isEmpty(), "same version offered");
	Require(Select({release}, "0.10.0", "ubuntu26.04", true).version.isEmpty(), "downgrade offered");
	for (const auto &field : {"digest", "browser_download_url", "state"}) {
		auto badAsset = asset;
		badAsset[field] = "invalid";
		auto badRelease = release;
		badRelease["assets"] = QJsonArray{badAsset};
		Require(Select({badRelease}, "0.2.1", "ubuntu26.04", true).name.isEmpty(), "unverified asset accepted");
	}
	auto draft = release;
	draft["draft"] = true;
	Require(Select({draft}, "0.2.1", "ubuntu26.04", true).version.isEmpty(), "draft offered");
	auto foreign = release;
	foreign["tag_name"] = "32.2.3";
	Require(Select({foreign}, "0.2.1", "ubuntu26.04", true).version.isEmpty(), "upstream version offered");
	std::cout << "Update selection, channel, digest and compatibility checks passed\n";
}
