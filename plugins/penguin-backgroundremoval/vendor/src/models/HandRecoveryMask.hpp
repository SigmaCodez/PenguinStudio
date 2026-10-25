// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <opencv2/imgproc.hpp>
#include <vector>

// The skin model can also classify background objects as skin. Recover detail
// only in components supported by the primary person mask; never erode that mask.
inline void MergeSupportedHandRecovery(cv::Mat &alpha, const cv::Mat &skin)
{
	cv::Mat labels;
	const int count = cv::connectedComponents(skin > 0.25f, labels, 8, CV_32S);
	std::vector<int> area(count, 0), support(count, 0);
	for (int y = 0; y < labels.rows; ++y) {
		const int *ids = labels.ptr<int>(y);
		const float *person = alpha.ptr<float>(y);
		for (int x = 0; x < labels.cols; ++x) {
			const int id = ids[x];
			if (id) {
				++area[id];
				support[id] += person[x] > 0.5f;
			}
		}
	}
	cv::Mat allowed = cv::Mat::zeros(skin.size(), CV_32F);
	for (int y = 0; y < labels.rows; ++y) {
		const int *ids = labels.ptr<int>(y);
		float *row = allowed.ptr<float>(y);
		for (int x = 0; x < labels.cols; ++x) {
			const int id = ids[x];
			if (id && support[id] > area[id] * 0.15f)
				row[x] = 1.0f;
		}
	}
	cv::GaussianBlur(allowed, allowed, cv::Size(3, 3), 0);
	cv::max(alpha, skin.mul(allowed), alpha);
}
