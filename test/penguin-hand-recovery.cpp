// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/penguin-backgroundremoval/vendor/src/models/HandRecoveryMask.hpp"
#include <iostream>
#include <stdexcept>

// Preserve the original per-pixel eligibility calculation as an independent
// reference for the component lookup optimization.
static void ReferenceMerge(cv::Mat &alpha, const cv::Mat &skin)
{
	cv::Mat labels;
	const int count = cv::connectedComponents(skin > 0.25f, labels, 8, CV_32S);
	std::vector<int> area(count, 0), support(count, 0);
	for (int y = 0; y < labels.rows; ++y) {
		for (int x = 0; x < labels.cols; ++x) {
			const int id = labels.at<int>(y, x);
			if (id) {
				++area[id];
				support[id] += alpha.at<float>(y, x) > 0.5f;
			}
		}
	}
	cv::Mat allowed = cv::Mat::zeros(skin.size(), CV_32F);
	for (int y = 0; y < labels.rows; ++y) {
		for (int x = 0; x < labels.cols; ++x) {
			const int id = labels.at<int>(y, x);
			if (id && support[id] > area[id] * 0.15f)
				allowed.at<float>(y, x) = 1.0f;
		}
	}
	cv::GaussianBlur(allowed, allowed, cv::Size(3, 3), 0);
	cv::max(alpha, skin.mul(allowed), alpha);
}

static void CheckLookupEquivalence()
{
	cv::RNG rng(90317);
	for (const cv::Size size : {cv::Size(31, 17), cv::Size(64, 48), cv::Size(256, 144), cv::Size(320, 192)}) {
		for (int padding : {0, 7}) {
			for (int sample = 0; sample < 4; ++sample) {
				cv::Mat storage(size.height + 2, size.width + padding, CV_32F);
				cv::Mat skinStorage(storage.size(), CV_32F);
				rng.fill(storage, cv::RNG::UNIFORM, 0.0, 1.0);
				rng.fill(skinStorage, cv::RNG::UNIFORM, 0.0, sample % 2 ? 0.3 : 1.0);
				auto alpha = storage(cv::Rect(0, 1, size.width, size.height));
				auto skin = skinStorage(cv::Rect(0, 1, size.width, size.height));
				const auto original = alpha.clone();
				const auto savedSkin = skinStorage.clone();
				auto expected = alpha.clone();
				ReferenceMerge(expected, skin);
				MergeSupportedHandRecovery(alpha, skin);
				if (cv::norm(alpha, expected, cv::NORM_INF) != 0.0 ||
				    cv::norm(skinStorage, savedSkin, cv::NORM_INF) != 0.0 ||
				    cv::countNonZero(alpha < original))
					throw std::runtime_error("Component lookup changed hand recovery or source data");
			}
		}
	}
}

int main()
{
	CheckLookupEquivalence();
	cv::Mat person = cv::Mat::zeros(48, 64, CV_32F);
	person(cv::Rect(16, 16, 16, 24)).setTo(0.9f);
	const cv::Mat original = person.clone();
	cv::Mat skin = cv::Mat::zeros(person.size(), CV_32F);
	skin(cv::Rect(24, 12, 8, 16)).setTo(1.0f); // Supported hand extending beyond the matte.
	skin(cv::Rect(2, 2, 8, 8)).setTo(1.0f);   // Unrelated background classified as skin.
	MergeSupportedHandRecovery(person, skin);
	if (person.at<float>(15, 27) < 0.9f || person.at<float>(5, 5) != 0.0f ||
	    cv::countNonZero(person < original))
		throw std::runtime_error("Hand detail, background rejection or primary matte preservation failed");
	for (bool emptyPerson : {false, true}) {
		person = emptyPerson ? cv::Mat::zeros(original.size(), CV_32F) : original.clone();
		const cv::Mat before = person.clone();
		skin.setTo(emptyPerson ? 1.0f : 0.0f);
		MergeSupportedHandRecovery(person, skin);
		if (cv::norm(person, before, cv::NORM_INF) != 0.0)
			throw std::runtime_error("Empty recovery or absent-person input changed the matte");
	}
	std::cout << "Supported hand detail retained; isolated background recovery rejected; primary matte unchanged\n";
}
