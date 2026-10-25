// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/penguin-backgroundremoval/vendor/src/models/HandRecoveryMask.hpp"
#include <iostream>
#include <stdexcept>

int main()
{
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
