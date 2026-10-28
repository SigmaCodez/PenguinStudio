// SPDX-License-Identifier: GPL-3.0-or-later
// Compare persistent hand buffers/names with the original per-frame pipeline.
#include <obs-module.h>
#include "../vendor/src/models/HandRecovery.hpp"
#include <cstring>
#include <iostream>
extern "C" void obs_log(int, const char *, ...) {}
extern "C" obs_module_t *obs_current_module() { return nullptr; }

int main(int argc, char **argv)
{
	if (argc != 2) { std::cerr << "Usage: hand-buffers MODEL.ort\n"; return 2; }
	try {
		Ort::InitApi(OrtGetApiBase()->GetApi(ORT_API_VERSION));
		cv::setNumThreads(1);
		HandRecovery actual(1, "cpu", argv[1]);
		Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Hand buffer reference");
		env.DisableTelemetryEvents();
		PenguinInference::DeviceState state;
		auto session = PenguinInference::CreateSession(env, argv[1], "cpu", 1, state);
		cv::RNG random(49128);
		int cases = 0;
		for (auto size : {cv::Size(320,192), cv::Size(320,192), cv::Size(192,320),
			cv::Size(192,320), cv::Size(100,100), cv::Size(320,192)}) {
			cv::Mat rgb(size, CV_32FC3); random.fill(rgb, cv::RNG::UNIFORM, 0., 255.);
			const float scale = 256.f / std::max(size.width, size.height);
			const int width = std::max(1, int(std::round(size.width * scale)));
			const int height = std::max(1, int(std::round(size.height * scale)));
			std::vector<float> input(256*256*3), output(256*256*6);
			cv::Mat padded(256,256,CV_32FC3,input.data()), resized;
			padded.setTo(0);
			cv::resize(rgb,resized,{width,height},0,0,cv::INTER_AREA);
			resized.convertTo(padded(cv::Rect(0,0,width,height)),CV_32FC3,1./255.);
			const std::array<int64_t,4> inDims{1,256,256,3}, outDims{1,256,256,6};
			auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
			auto in = Ort::Value::CreateTensor<float>(memory,input.data(),input.size(),inDims.data(),4);
			auto out = Ort::Value::CreateTensor<float>(memory,output.data(),output.size(),outDims.data(),4);
			Ort::AllocatorWithDefaultOptions allocator;
			auto inputName=session->GetInputNameAllocated(0,allocator), outputName=session->GetOutputNameAllocated(0,allocator);
			const char *inputs[]={inputName.get()}, *outputs[]={outputName.get()};
			session->Run(Ort::RunOptions{nullptr},inputs,&in,1,outputs,&out,1);
			cv::Mat small(height,width,CV_32F), skin;
			if (penguin_logits6_to_skin_mask(output.data(),output.size(),width,height,256*6,
				small.ptr<float>(),small.total(),small.step1()) != 1) throw std::runtime_error("Reference conversion failed");
			cv::resize(small,skin,size);
			cv::Mat expected(size,CV_32F); random.fill(expected,cv::RNG::UNIFORM,0.,1.);
			auto result=expected.clone();
			MergeSupportedHandRecovery(expected,skin);
			actual.infer(rgb); actual.merge(result);
			if (std::memcmp(expected.data,result.data,expected.total()*sizeof(float))) throw std::runtime_error("Persistent-buffer output differs");
			++cases;
		}
		std::cout << cases << " real-model repeated/resized hand-buffer comparisons passed\n";
	} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
