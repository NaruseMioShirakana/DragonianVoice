#include "../../../header/InferTools/Sampler/MoeVSSamplers.hpp"
#include <deque>
#include <random>
#include "../../../header/InferTools/inferTools.hpp"

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
				LibDLVoiceCodecThrow(std::string("Locate: denoise\n") + e1.what());
			}

			noiseList.emplace_back(DenoiseOut[0].GetTensorData<float>(),
				DenoiseOut[0].GetTensorData<float>() +
				DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount()); // NoiseListExpand

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
				LibDLVoiceCodecThrow(std::string("Locate: pred\n") + e1.what());
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
				LibDLVoiceCodecThrow(std::string("Locate: denoise\n") + e1.what());
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
				LibDLVoiceCodecThrow(std::string("Locate: denoise\n") + e1.what());
			}
			if (noiseList.size() < 4)
				noiseList.emplace_back(DenoiseOut[0].GetTensorData<float>(), DenoiseOut[0].GetTensorData<float>() + DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount());
			else
			{
				noiseList.pop_front();
				noiseList.emplace_back(DenoiseOut[0].GetTensorData<float>(), DenoiseOut[0].GetTensorData<float>() + DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount());
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
			LibDLVoiceCodecThrow(std::string("Locate: pred\n") + e1.what());
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
			LibDLVoiceCodecThrow(std::string("Locate: alphas\n") + e1.what());
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
			LibDLVoiceCodecThrow(std::string("Locate: denoise\n") + e1.what());
		}
		const auto x = DenoiseIn[0].GetTensorMutableData<float>();
		const auto noise_pred = DenoiseOut[0].GetTensorMutableData<float>();
		const auto noise_size = DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
		const auto sq_aprev = sqrt(a_prev);
		const auto sq_at = sqrt(a_t);
		const auto np_m_op = (sqrt((1 - a_prev) / a_prev) - sqrt((1 - a_t) / a_t));
#ifndef MoeVoiceStudioAvxAcc
		for (size_t i = 0; i < noise_size; ++i)
			noise_pred[i] = (x[i] / sq_at + np_m_op * noise_pred[i]) * sq_aprev;
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

ReflowEularSampler::ReflowEularSampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory) :MoeVSReflowBaseSampler(Velocity, MelBins, _ProgressCallback, memory) {}

ReflowHeunSampler::ReflowHeunSampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory) :MoeVSReflowBaseSampler(Velocity, MelBins, _ProgressCallback, memory) {}

ReflowPececeSampler::ReflowPececeSampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory) :MoeVSReflowBaseSampler(Velocity, MelBins, _ProgressCallback, memory) {}

ReflowRk4Sampler::ReflowRk4Sampler(Ort::Session* Velocity, int64_t MelBins, const ProgressCallback& _ProgressCallback, Ort::MemoryInfo* memory) :MoeVSReflowBaseSampler(Velocity, MelBins, _ProgressCallback, memory) {}

std::vector<Ort::Value> ReflowEularSampler::Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process)
{
	std::vector<Ort::Value> Spec ,TensorsOut;
	Spec.emplace_back(std::move(Tensors[0]));
	Spec.emplace_back(std::move(Tensors[1]));
	Spec.emplace_back(std::move(Tensors[2]));
	const auto x_size = Spec[0].GetTensorTypeAndShapeInfo().GetElementCount();
	auto t = *Spec[1].GetTensorMutableData<float>();
	int64_t tvec[] = { 0 };
	constexpr int64_t tshape[] = { 1 };
	Spec[1] = Ort::Value::CreateTensor(*Memory_, tvec, 1, tshape, 1);
	for (int64_t i = 0; i < Steps; ++i)
	{
		*Spec[1].GetTensorMutableData<int64_t>() = int64_t(t * Scale);
		try
		{
			TensorsOut = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				Spec.data(),
				Spec.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		auto x_it = Spec[0].GetTensorMutableData<float>();
		auto k_it = TensorsOut[0].GetTensorMutableData<float>();
#ifndef MoeVoiceStudioAvxAcc
		const auto x_end = Spec[0].GetTensorMutableData<float>() + x_size;
		while (x_it != x_end) { *(x_it++) += (*(k_it++) * dt); }
#else
		InferTools::FloatTensorWrapper XWrapper(x_it, x_size);
		InferTools::FloatTensorWrapper KWrapper(k_it, x_size);
		KWrapper *= dt;
		XWrapper += KWrapper
#endif
		t += dt;
		Callback_(++Process, 1);
	}
	return Spec;
}

std::vector<Ort::Value> ReflowHeunSampler::Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process)
{
	std::vector<Ort::Value> X1;
	X1.emplace_back(std::move(Tensors[0]));
	X1.emplace_back(std::move(Tensors[1]));
	X1.emplace_back(std::move(Tensors[2]));
	auto T = *X1[1].GetTensorMutableData<float>();
	const auto XSize = X1[0].GetTensorTypeAndShapeInfo().GetElementCount();
	const auto Shape = X1[0].GetTensorTypeAndShapeInfo().GetShape();
	int64_t tvec[] = { 0 };
	constexpr int64_t tshape[] = { 1 };
	X1[1] = Ort::Value::CreateTensor(*Memory_, tvec, 1, tshape, 1);
	for (int64_t i = 0; i < Steps; ++i)
	{
		std::vector<Ort::Value> X2, K1, K2;
		std::vector K2X(X1[0].GetTensorData<float>(), X1[0].GetTensorData<float>() + XSize);
		//K1
		*X1[1].GetTensorMutableData<int64_t>() = int64_t(T * Scale);
		try
		{
			K1 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X1.data(),
				X1.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		//K2
		auto k_it = K1[0].GetTensorData<float>();
		for (auto& iter : K2X)
			iter += ((*(k_it++)) * dt);
		X2.emplace_back(Ort::Value::CreateTensor(*Memory_, K2X.data(), K2X.size(), Shape.data(), Shape.size()));
		X2.emplace_back(std::move(X1[1]));
		X2.emplace_back(std::move(X1[2]));
		*X2[1].GetTensorMutableData<int64_t>() = int64_t((T + dt) * Scale);
		try
		{
			K2 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X2.data(),
				X2.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}

		auto x_it = X1[0].GetTensorMutableData<float>();
		auto k1_it = K1[0].GetTensorMutableData<float>();
		auto k2_it = K2[0].GetTensorMutableData<float>();
#ifndef MoeVoiceStudioAvxAcc
		const auto x_end = X1[0].GetTensorMutableData<float>() + XSize;
		while (x_it != x_end) { *(x_it++) += ((*(k1_it++) + *(k2_it++)) * dt / 2.f); }
#else
		InferTools::FloatTensorWrapper XWrapper(x_it, XSize);
		InferTools::FloatTensorWrapper K1Wrapper(k1_it, XSize);
		InferTools::FloatTensorWrapper K2Wrapper(k2_it, XSize);
		((K1Wrapper += K2Wrapper) *= dt) /= 2.f;
		XWrapper += K1Wrapper;
#endif
		X1[1] = std::move(X2[1]);
		X1[2] = std::move(X2[2]);
		T += dt;
		Callback_(++Process, 1);
	}
	return X1;
}

std::vector<Ort::Value> ReflowPececeSampler::Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process)
{
	std::vector<Ort::Value> X1;
	X1.emplace_back(std::move(Tensors[0]));
	X1.emplace_back(std::move(Tensors[1]));
	X1.emplace_back(std::move(Tensors[2]));
	auto T = *X1[1].GetTensorMutableData<float>();
	const auto Shape = X1[0].GetTensorTypeAndShapeInfo().GetShape();
	const auto XSize = X1[0].GetTensorTypeAndShapeInfo().GetElementCount();
	int64_t tvec[] = { 0 };
	constexpr int64_t tshape[] = { 1 };
	X1[1] = Ort::Value::CreateTensor(*Memory_, tvec, 1, tshape, 1);
	for (int64_t i = 0; i < Steps; ++i)
	{
		std::vector<Ort::Value> X2, X3, X4, K1, K2, K3, K4;
		std::vector K2X(X1[0].GetTensorData<float>(), X1[0].GetTensorData<float>() + XSize);
		auto K3X = K2X;
		//K1
		*X1[1].GetTensorMutableData<int64_t>() = int64_t(T * Scale);
		try
		{
			K1 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X1.data(),
				X1.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		//K2
		auto k_it = K1[0].GetTensorData<float>();
		for (auto& iter : K2X)
			iter += ((*(k_it++)) * dt);
		X2.emplace_back(Ort::Value::CreateTensor(*Memory_, K2X.data(), K2X.size(), Shape.data(), Shape.size()));
		X2.emplace_back(std::move(X1[1]));
		X2.emplace_back(std::move(X1[2]));
		*X2[1].GetTensorMutableData<int64_t>() = int64_t((T + dt) * Scale);
		try
		{
			K2 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X2.data(),
				X2.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		//K3
		auto k_it1 = K1[0].GetTensorData<float>();
		auto k_it2 = K2[0].GetTensorData<float>();
		for (auto& iter : K3X)
			iter += (0.5f * (*(k_it1++) + *(k_it2++)) * dt);
		X3.emplace_back(Ort::Value::CreateTensor(*Memory_, K3X.data(), K3X.size(), Shape.data(), Shape.size()));
		X3.emplace_back(std::move(X2[1]));
		X3.emplace_back(std::move(X2[2]));
		*X3[1].GetTensorMutableData<int64_t>() = int64_t((T + dt) * Scale);
		try
		{
			K3 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X3.data(),
				X3.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		//K4
		k_it = K3[0].GetTensorData<float>();
		auto K4X = K3X;
		for (auto& iter : K4X)
			iter += (*(k_it++) * dt);
		X4.emplace_back(Ort::Value::CreateTensor(*Memory_, K4X.data(), K4X.size(), Shape.data(), Shape.size()));
		X4.emplace_back(std::move(X3[1]));
		X4.emplace_back(std::move(X3[2]));
		*X4[1].GetTensorMutableData<int64_t>() = int64_t((T + 2.f * dt) * Scale);
		try
		{
			K4 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X4.data(),
				X4.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}

		auto x_it = X1[0].GetTensorMutableData<float>();
		auto k3_it = K3[0].GetTensorMutableData<float>();
		auto k4_it = K4[0].GetTensorMutableData<float>();
#ifndef MoeVoiceStudioAvxAcc
		const auto x_end = X1[0].GetTensorMutableData<float>() + XSize;
		while (x_it != x_end) { *(x_it++) += ((*(k3_it++) + *(k4_it++)) * dt / 2.f); }
#else
		InferTools::FloatTensorWrapper XWrapper(x_it, XSize);
		InferTools::FloatTensorWrapper K3Wrapper(k3_it, XSize);
		InferTools::FloatTensorWrapper K4Wrapper(k4_it, XSize);
		((K3Wrapper += K4Wrapper) *= dt) /= 2.f;
		XWrapper += K3Wrapper;
#endif
		X1[1] = std::move(X4[1]);
		X1[2] = std::move(X4[2]);
		T += dt;
		Callback_(++Process, 1);
	}
	return X1;
}

std::vector<Ort::Value> ReflowRk4Sampler::Sample(std::vector<Ort::Value>& Tensors, int64_t Steps, float dt, float Scale, size_t& Process)
{
	std::vector<Ort::Value> X1;
	X1.emplace_back(std::move(Tensors[0]));
	X1.emplace_back(std::move(Tensors[1]));
	X1.emplace_back(std::move(Tensors[2]));
	auto T = *X1[1].GetTensorMutableData<float>();
	const auto Shape = X1[0].GetTensorTypeAndShapeInfo().GetShape();
	const auto XSize = X1[0].GetTensorTypeAndShapeInfo().GetElementCount();
	int64_t tvec[] = { 0 };
	constexpr int64_t tshape[] = { 1 };
	X1[1] = Ort::Value::CreateTensor(*Memory_, tvec, 1, tshape, 1);
	for (int64_t i = 0; i < Steps; ++i)
	{
		std::vector<Ort::Value> X2, X3, X4, K1, K2, K3, K4;
		std::vector K2X(X1[0].GetTensorData<float>(), X1[0].GetTensorData<float>() + XSize);
		auto K3X = K2X, K4X = K2X;
		//K1
		*X1[1].GetTensorMutableData<int64_t>() = int64_t(T * Scale);
		try
		{
			K1 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X1.data(),
				X1.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		//K2
		auto k_it = K1[0].GetTensorData<float>();
		for (auto& iter : K2X)
			iter += (0.5f * (*(k_it++)) * dt);
		X2.emplace_back(Ort::Value::CreateTensor(*Memory_, K2X.data(), K2X.size(), Shape.data(), Shape.size()));
		X2.emplace_back(std::move(X1[1]));
		X2.emplace_back(std::move(X1[2]));
		*X2[1].GetTensorMutableData<int64_t>() = int64_t((T + 0.5f * dt) * Scale);
		try
		{
			K2 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X2.data(),
				X2.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		//K3
		k_it = K2[0].GetTensorData<float>();
		for (auto& iter : K3X)
			iter += (0.5f * (*(k_it++)) * dt);
		X3.emplace_back(Ort::Value::CreateTensor(*Memory_, K3X.data(), K3X.size(), Shape.data(), Shape.size()));
		X3.emplace_back(std::move(X2[1]));
		X3.emplace_back(std::move(X2[2]));
		*X3[1].GetTensorMutableData<int64_t>() = int64_t((T + 0.5f * dt) * Scale);
		try
		{
			K3 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X3.data(),
				X3.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}
		//K4
		k_it = K3[0].GetTensorData<float>();
		for (auto& iter : K4X)
			iter += (*(k_it++) * dt);
		X4.emplace_back(Ort::Value::CreateTensor(*Memory_, K4X.data(), K4X.size(), Shape.data(), Shape.size()));
		X4.emplace_back(std::move(X3[1]));
		X4.emplace_back(std::move(X3[2]));
		*X4[1].GetTensorMutableData<int64_t>() = int64_t((T + dt) * Scale);
		try
		{
			K4 = Velocity_->Run(Ort::RunOptions{ nullptr },
				velocityInput.data(),
				X4.data(),
				X4.size(),
				velocityOutput.data(),
				velocityOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow(std::string("Locate: Velocity\n") + e.what());
		}

		auto x_it = X1[0].GetTensorMutableData<float>();
		auto k1_it = K1[0].GetTensorMutableData<float>();
		auto k2_it = K2[0].GetTensorMutableData<float>();
		auto k3_it = K3[0].GetTensorMutableData<float>();
		auto k4_it = K4[0].GetTensorMutableData<float>();
#ifndef MoeVoiceStudioAvxAcc
		const auto x_end = X1[0].GetTensorMutableData<float>() + XSize;
		while (x_it != x_end) { *(x_it++) += ((*(k1_it++) + (*(k2_it++) * 2.f) + (*(k3_it++) * 2.f) + *(k4_it++)) * dt / 6.f); }
#else
		InferTools::FloatTensorWrapper XWrapper(x_it, XSize);
		InferTools::FloatTensorWrapper K1Wrapper(k1_it, XSize);
		InferTools::FloatTensorWrapper K2Wrapper(k2_it, XSize);
		InferTools::FloatTensorWrapper K3Wrapper(k3_it, XSize);
		InferTools::FloatTensorWrapper K4Wrapper(k4_it, XSize);
		K2Wrapper *= 2.f;
		K3Wrapper *= 2.f;
		((((K1Wrapper += K2Wrapper) += K3Wrapper) += K4Wrapper) *= dt) /= 6.f;
		XWrapper += K1Wrapper;
#endif
		X1[1] = std::move(X4[1]);
		X1[2] = std::move(X4[2]);
		T += dt;
		Callback_(++Process, 1);
	}
	return X1;
}

MoeVoiceStudioSamplerEnd