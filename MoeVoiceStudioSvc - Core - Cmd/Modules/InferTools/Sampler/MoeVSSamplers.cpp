#include "MoeVSSamplers.hpp"
#include <deque>
#include <random>
#include "../inferTools.hpp"

MoeVoiceStudioSamplerHeader
	PndmSampler::PndmSampler(Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory) :
	MoeVSBaseSampler(alpha, dfn, pred, Mel_Bins, _ProgressCallback, memory) {}

DDimSampler::DDimSampler(Ort::Session* alpha, Ort::Session* dfn, Ort::Session* pred, int64_t Mel_Bins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory) :
	MoeVSBaseSampler(alpha, dfn, pred, Mel_Bins, _ProgressCallback, memory) {}

std::vector<Ort::Value> PndmSampler::Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, int64_t SpeedUp, float NoiseScale, int64_t Seed, size_t& Process)
{
	std::vector<int64_t> diffusionSteps;
	for (int64_t itt = Steps - SpeedUp; itt >= 0; itt -= SpeedUp)
		diffusionSteps.push_back(itt);

	std::vector<Ort::Value> DenoiseIn;
	std::vector<Ort::Value> DenoiseOut;
	std::vector<Ort::Value> PredIn;
	std::vector<Ort::Value> PredOut;
	std::vector<Ort::Value> DiffOut;

	std::deque<std::vector<float>> noiseList;

	constexpr int64_t timeshape[1] = { 1 }; // PLMS SAMPLING

	for (const auto& t : diffusionSteps)
	{
		int64_t time[1] = { t };
		int64_t time_prev[1] = { t - SpeedUp > 0 ? t - SpeedUp : 0 };
		if (noiseList.empty())
		{
			DenoiseIn.emplace_back(std::move(Tensors[1])); // noise  DenoiseIn[0]
			DenoiseIn.emplace_back(Ort::Value::CreateTensor(*Memory, time,
				1, timeshape, 1)); // time  DenoiseIn[1]
			DenoiseIn.emplace_back(std::move(Tensors[0])); // condition  DenoiseIn[2]
			try
			{
				DenoiseOut = DenoiseFn->Run(Ort::RunOptions{ nullptr },
					denoiseInput.data(),
					DenoiseIn.data(),
					DenoiseIn.size(),
					denoiseOutput.data(),
					denoiseOutput.size());
			}
			catch (Ort::Exception& e1)
			{
				throw std::exception((std::string("Locate: denoise\n") + e1.what()).c_str());
			}

			noiseList.emplace_back(std::vector<float>(DenoiseOut[0].GetTensorData<float>(),
				DenoiseOut[0].GetTensorData<float>() +
				DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount())); // NoiseListExpand

			PredIn.emplace_back(std::move(DenoiseIn[0])); // noise  PredIn[0]
			PredIn.emplace_back(std::move(DenoiseOut[0])); // noise_pred  PredIn[1]
			PredIn.emplace_back(std::move(DenoiseIn[1])); // time  PredIn[2]
			PredIn.emplace_back(Ort::Value::CreateTensor(*Memory, time_prev,
				1, timeshape, 1)); // time_prev  PredIn[3]
			try
			{
				PredOut = NoisePredictor->Run(Ort::RunOptions{ nullptr },
					predInput.data(),
					PredIn.data(),
					PredIn.size(),
					predOutput.data(),
					predOutput.size());
			}
			catch (Ort::Exception& e1)
			{
				throw std::exception((std::string("Locate: pred\n") + e1.what()).c_str());
			}
			DenoiseIn[0] = std::move(PredOut[0]); // x_pred
			DenoiseIn[1] = std::move(PredIn[3]); // time_prev
			//DenoiseIn[2] = std::move(DenoiseIn[2]); // condition
			try
			{
				DenoiseOut = DenoiseFn->Run(Ort::RunOptions{ nullptr },
					denoiseInput.data(),
					DenoiseIn.data(),
					DenoiseIn.size(),
					denoiseOutput.data(),
					denoiseOutput.size());
			}
			catch (Ort::Exception& e1)
			{
				throw std::exception((std::string("Locate: denoise\n") + e1.what()).c_str());
			}
			auto noise_pred_prev = DenoiseOut[0].GetTensorMutableData<float>();
			for (const auto it : noiseList[0])
				(*(noise_pred_prev++) += it) /= 2.0f;
			//PredIn[0] = std::move(PredIn[0]); // noise
			PredIn[1] = std::move(DenoiseOut[0]); // noise_pred_prime
			//PredIn[2] = std::move(PredIn[2]); // time
			PredIn[3] = std::move(DenoiseIn[1]); // time_prev
		}
		else
		{
			DenoiseIn[0] = std::move(PredOut[0]); // x
			DenoiseIn[1] = Ort::Value::CreateTensor(*Memory, time, 1,
				timeshape, 1); // time
			//DenoiseIn[2] = std::move(DenoiseIn[2]); // condition
			try
			{
				DenoiseOut = DenoiseFn->Run(Ort::RunOptions{ nullptr },
					denoiseInput.data(),
					DenoiseIn.data(),
					DenoiseIn.size(),
					denoiseOutput.data(),
					denoiseOutput.size());
			}
			catch (Ort::Exception& e1)
			{
				throw std::exception((std::string("Locate: denoise\n") + e1.what()).c_str());
			}
			if (noiseList.size() < 4)
				noiseList.emplace_back(std::vector<float>(DenoiseOut[0].GetTensorData<float>(), DenoiseOut[0].GetTensorData<float>() + DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount()));
			else
			{
				noiseList.pop_front();
				noiseList.emplace_back(std::vector<float>(DenoiseOut[0].GetTensorData<float>(), DenoiseOut[0].GetTensorData<float>() + DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount()));
			}
			auto noise_pred_prime = DenoiseOut[0].GetTensorMutableData<float>();
			if (noiseList.size() == 2)
				for (size_t it = 0; it < noiseList[0].size(); ++it)
					((*(noise_pred_prime++) *= 3.0f) -= noiseList[0][it]) /= 2.0f;
			if (noiseList.size() == 3)
				for (size_t it = 0; it < noiseList[0].size(); ++it)
					(((*(noise_pred_prime++) *= 23.0f) -= noiseList[1][it] * 16.0f) += noiseList[0][it] * 5.0f) /= 12.0f;
			if (noiseList.size() == 4)
				for (size_t it = 0; it < noiseList[0].size(); ++it)
					((((*(noise_pred_prime++) *= 55.0f) -= noiseList[2][it] * 59.0f) += noiseList[1][it] * 37.0f) -= noiseList[0][it] * 9.0f) /= 24.0f;
			PredIn[0] = std::move(DenoiseIn[0]); // x
			PredIn[1] = std::move(DenoiseOut[0]); // noise_pred_prime
			PredIn[2] = std::move(DenoiseIn[1]); // time
			PredIn[3] = Ort::Value::CreateTensor(*Memory, time_prev, 1, timeshape, 1);
		}
		try
		{
			PredOut = NoisePredictor->Run(Ort::RunOptions{ nullptr },
				predInput.data(),
				PredIn.data(),
				PredIn.size(),
				predOutput.data(),
				predOutput.size());
		}
		catch (Ort::Exception& e1)
		{
			throw std::exception((std::string("Locate: pred\n") + e1.what()).c_str());
		}
		_callback(++Process, 1);
	}
	return PredOut;
}

std::vector<Ort::Value> DDimSampler::Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, int64_t SpeedUp, float NoiseScale, int64_t Seed, size_t& Process)
{
	std::vector<int64_t> diffusionSteps;
	for (int64_t itt = Steps - SpeedUp; itt >= 0; itt -= SpeedUp)
		diffusionSteps.push_back(itt);
	std::vector<Ort::Value> DenoiseIn;
	std::vector<Ort::Value> AlphaIn1, AlphaIn2;

	int64_t time[1] = { 0 };
	int64_t time_prev[1] = { 0 };

	constexpr int64_t timeshape[1] = { 1 }; // DDIM SAMPLING
	AlphaIn1.emplace_back(Ort::Value::CreateTensor(*Memory, time, 1, timeshape, 1));
	AlphaIn2.emplace_back(Ort::Value::CreateTensor(*Memory, time_prev, 1, timeshape, 1));
	DenoiseIn.emplace_back(std::move(Tensors[1])); // noise  DenoiseIn[0]
	DenoiseIn.emplace_back(Ort::Value::CreateTensor(*Memory, time, 1, timeshape, 1)); // time  DenoiseIn[1]
	DenoiseIn.emplace_back(std::move(Tensors[0])); // condition  DenoiseIn[2]
	for (const auto& t : diffusionSteps)
	{
		std::vector<Ort::Value> DenoiseOut;
		float a_t, a_prev;
		try
		{
			time[0] = t;
			a_t = Alpha->Run(Ort::RunOptions{ nullptr },
				alphain.data(),
				AlphaIn1.data(),
				AlphaIn1.size(),
				alphaout.data(),
				alphaout.size())[0].GetTensorData<float>()[0];
			time_prev[0] = (((t - SpeedUp) > 0) ? t - SpeedUp : 0);
			a_prev = Alpha->Run(Ort::RunOptions{ nullptr },
				alphain.data(),
				AlphaIn2.data(),
				AlphaIn2.size(),
				alphaout.data(),
				alphaout.size())[0].GetTensorData<float>()[0];
		}
		catch (Ort::Exception& e1)
		{
			throw std::exception((std::string("Locate: alphas\n") + e1.what()).c_str());
		}
		try
		{
			DenoiseOut = DenoiseFn->Run(Ort::RunOptions{ nullptr },
				denoiseInput.data(),
				DenoiseIn.data(),
				DenoiseIn.size(),
				denoiseOutput.data(),
				denoiseOutput.size());
		}
		catch (Ort::Exception& e1)
		{
			throw std::exception((std::string("Locate: denoise\n") + e1.what()).c_str());
		}
		const auto x = DenoiseIn[0].GetTensorMutableData<float>();
		const auto noise_pred = DenoiseOut[0].GetTensorMutableData<float>();
		const auto noise_size = DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
		const auto sq_aprev = sqrt(a_prev);
		const auto sq_at = sqrt(a_t);
		const auto np_m_op = (sqrt((1 - a_prev) / a_prev) - sqrt((1 - a_t) / a_t));
#ifndef MoeVoiceStudioAvxAcc
		for (size_t i = 0; i < noise_size; ++i)
			noise_pred[i] = (x[i] / sq_at + (sqrt((1 - a_prev) / a_prev) - sqrt((1 - a_t) / a_t)) * noise_pred[i]) * sq_aprev;
#else
		InferTools::FloatTensorWrapper XWrapper(x, noise_size);
		InferTools::FloatTensorWrapper PredWrapper(noise_pred, noise_size);
		XWrapper /= sq_at;
		((PredWrapper *= np_m_op) += XWrapper) *= sq_aprev;
#endif
		DenoiseIn[0] = std::move(DenoiseOut[0]);
		_callback(++Process, 1);
	}
	std::vector<Ort::Value> out;
	out.emplace_back(std::move(DenoiseIn[0]));
	return out;
}

MoeVoiceStudioSamplerEnd