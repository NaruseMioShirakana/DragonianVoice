#include "MoeVSCoreTensorExtractor.hpp"
#include <random>
#include "../../Logger/MoeSSLogger.hpp"
#include "../inferTools.hpp"

MoeVoiceStudioTensorExtractorHeader

MoeVoiceStudioTensorExtractor::Inputs SoVits2TensorExtractor::Extract(const std::vector<float>& HiddenUnit, const std::vector<float>& F0, const std::vector<float>& Volume, const std::vector<std::vector<float>>& SpkMap, Params params)
{
	Inputs SvcTensors;

	const auto HubertSize = HiddenUnit.size();
	const auto HubertLen = int64_t(HubertSize) / int64_t(_HiddenSize);
	SvcTensors.Data.FrameShape = { 1, HubertLen };
	SvcTensors.Data.HiddenUnitShape = { 1, HubertLen, int64_t(_HiddenSize) };
	// SvcTensors.Data.SpkShape = { SvcTensors.Data.FrameShape[1], int64_t(_NSpeaker) };

	SvcTensors.Data.HiddenUnit = HiddenUnit;
	SvcTensors.Data.Length[0] = HubertLen;
	SvcTensors.Data.F0 = InferTools::InterpFunc(F0, (long)F0.size(), (long)HubertLen);
	for (auto& it : SvcTensors.Data.F0)
		it *= (float)pow(2.0, static_cast<double>(params.upKeys) / 12.0);
	SvcTensors.Data.NSFF0 = GetNSFF0(SvcTensors.Data.F0);
	SvcTensors.Data.Speaker[0] = params.Chara;

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.HiddenUnit.data(),
		HubertSize,
		SvcTensors.Data.HiddenUnitShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Length,
		1,
		SvcTensors.Data.OneShape,
		1
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.NSFF0.data(),
		SvcTensors.Data.NSFF0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Speaker,
		1,
		SvcTensors.Data.OneShape,
		1
	));

	SvcTensors.InputNames = InputNames.data();

	return SvcTensors;
}

MoeVoiceStudioTensorExtractor::Inputs SoVits3TensorExtractor::Extract(const std::vector<float>& HiddenUnit, const std::vector<float>& F0, const std::vector<float>& Volume, const std::vector<std::vector<float>>& SpkMap, Params params)
{
	Inputs SvcTensors;

	auto HubertSize = HiddenUnit.size();
	const auto HubertLen = int64_t(HubertSize) / int64_t(_HiddenSize);
	SvcTensors.Data.FrameShape = { 1, int64_t(params.AudioSize * _SamplingRate / _SrcSamplingRate / _HopSize) };
	SvcTensors.Data.HiddenUnitShape = { 1, HubertLen, int64_t(_HiddenSize) };
	// SvcTensors.Data.SpkShape = { SvcTensors.Data.FrameShape[1], int64_t(_NSpeaker) };
	const int64_t upSample = int64_t(_SamplingRate) / 16000;
	const auto srcHubertSize = SvcTensors.Data.HiddenUnitShape[1];
	SvcTensors.Data.HiddenUnitShape[1] *= upSample;
	HubertSize *= upSample;
	SvcTensors.Data.FrameShape[1] = SvcTensors.Data.HiddenUnitShape[1];
	SvcTensors.Data.HiddenUnit.reserve(HubertSize * (upSample + 1));
	for (int64_t itS = 0; itS < srcHubertSize; ++itS)
		for (int64_t itSS = 0; itSS < upSample; ++itSS)
			SvcTensors.Data.HiddenUnit.insert(SvcTensors.Data.HiddenUnit.end(), HiddenUnit.begin() + itS * (int64_t)_HiddenSize, HiddenUnit.begin() + (itS + 1) * (int64_t)_HiddenSize);
	SvcTensors.Data.F0 = GetInterpedF0(InferTools::InterpFunc(F0, long(F0.size()), long(SvcTensors.Data.HiddenUnitShape[1])));
	for (auto& it : SvcTensors.Data.F0)
		it *= (float)pow(2.0, static_cast<double>(params.upKeys) / 12.0);
	SvcTensors.Data.Speaker[0] = params.Chara;
	SvcTensors.Data.Length[0] = SvcTensors.Data.HiddenUnitShape[1];

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.HiddenUnit.data(),
		HubertSize,
		SvcTensors.Data.HiddenUnitShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Length,
		1,
		SvcTensors.Data.OneShape,
		1
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.F0.data(),
		SvcTensors.Data.F0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Speaker,
		1,
		SvcTensors.Data.OneShape,
		1
	));

	SvcTensors.InputNames = InputNames.data();

	return SvcTensors;
}

MoeVoiceStudioTensorExtractor::Inputs SoVits4TensorExtractor::Extract(const std::vector<float>& HiddenUnit, const std::vector<float>& F0, const std::vector<float>& Volume, const std::vector<std::vector<float>>& SpkMap, Params params)
{
	Inputs SvcTensors;
	std::mt19937 gen(int(params.Seed));
	std::normal_distribution<float> normal(0, 1);
	const auto HubertSize = HiddenUnit.size();
	const auto HubertLen = int64_t(HubertSize) / int64_t(_HiddenSize);
	SvcTensors.Data.FrameShape = { 1, int64_t(params.AudioSize * _SamplingRate / _SrcSamplingRate / _HopSize) };
	SvcTensors.Data.HiddenUnitShape = { 1, HubertLen, int64_t(_HiddenSize) };
	SvcTensors.Data.SpkShape = { SvcTensors.Data.FrameShape[1], int64_t(_NSpeaker) };
	SvcTensors.Data.NoiseShape = { 1, 192, SvcTensors.Data.FrameShape[1] };
	const auto NoiseSize = SvcTensors.Data.NoiseShape[1] * SvcTensors.Data.NoiseShape[2] * SvcTensors.Data.NoiseShape[0];

	SvcTensors.Data.HiddenUnit = HiddenUnit;
	SvcTensors.Data.F0 = GetInterpedF0(InferTools::InterpFunc(F0, long(F0.size()), long(SvcTensors.Data.FrameShape[1])));
	for (auto& it : SvcTensors.Data.F0)
		it *= (float)pow(2.0, static_cast<double>(params.upKeys) / 12.0);
	SvcTensors.Data.Alignment = GetAligments(SvcTensors.Data.FrameShape[1], HubertLen);
	SvcTensors.Data.UnVoice = GetUV(F0);
	SvcTensors.Data.Noise = std::vector(NoiseSize, 0.f);
	for (auto& it : SvcTensors.Data.Noise)
		it = normal(gen) * params.NoiseScale;
	SvcTensors.Data.Speaker[0] = params.Chara;

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.HiddenUnit.data(),
		HubertSize,
		SvcTensors.Data.HiddenUnitShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.F0.data(),
		SvcTensors.Data.F0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Alignment.data(),
		SvcTensors.Data.Alignment.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.UnVoice.data(),
		SvcTensors.Data.UnVoice.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Noise.data(),
		SvcTensors.Data.Noise.size(),
		SvcTensors.Data.NoiseShape.data(),
		3
	));

	if (_SpeakerMix)
	{
		SvcTensors.Data.SpkMap = GetCurrectSpkMixData(SpkMap, SvcTensors.Data.FrameShape[1], params.Chara);
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.SpkMap.data(),
			SvcTensors.Data.SpkMap.size(),
			SvcTensors.Data.SpkShape.data(),
			2
		));
	}
	else
	{
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Speaker,
			1,
			SvcTensors.Data.OneShape,
			1
		));
	}

	if (_Volume)
	{
		SvcTensors.InputNames = InputNamesVol.data();
		SvcTensors.Data.Volume = InferTools::InterpFunc(Volume, long(Volume.size()), long(SvcTensors.Data.FrameShape[1]));
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Volume.data(),
			SvcTensors.Data.FrameShape[1],
			SvcTensors.Data.FrameShape.data(),
			2
		));
	}
	else
		SvcTensors.InputNames = InputNames.data();

	return SvcTensors;
}

MoeVoiceStudioTensorExtractor::Inputs SoVits4DDSPTensorExtractor::Extract(const std::vector<float>& HiddenUnit, const std::vector<float>& F0, const std::vector<float>& Volume, const std::vector<std::vector<float>>& SpkMap, Params params)
{
	Inputs SvcTensors;
	std::mt19937 gen(int(params.Seed));
	std::normal_distribution<float> normal(0, 1);
	const auto HubertSize = HiddenUnit.size();
	const auto HubertLen = int64_t(HubertSize) / int64_t(_HiddenSize);
	SvcTensors.Data.FrameShape = { 1, int64_t(params.AudioSize * _SamplingRate / _SrcSamplingRate / _HopSize) };
	SvcTensors.Data.HiddenUnitShape = { 1, HubertLen, int64_t(_HiddenSize) };
	SvcTensors.Data.SpkShape = { SvcTensors.Data.FrameShape[1], int64_t(_NSpeaker) };
	SvcTensors.Data.NoiseShape = { 1, 192, SvcTensors.Data.FrameShape[1] };
	const auto NoiseSize = SvcTensors.Data.NoiseShape[1] * SvcTensors.Data.NoiseShape[2] * SvcTensors.Data.NoiseShape[0];
	SvcTensors.Data.DDSPNoiseShape = { 1, 2048, SvcTensors.Data.FrameShape[1] };
	const int64_t IstftCount = SvcTensors.Data.FrameShape[1] * 2048;

	SvcTensors.Data.HiddenUnit = HiddenUnit;
	SvcTensors.Data.F0 = GetInterpedF0(InferTools::InterpFunc(F0, long(F0.size()), long(SvcTensors.Data.FrameShape[1])));
	for (auto& it : SvcTensors.Data.F0)
		it *= (float)pow(2.0, static_cast<double>(params.upKeys) / 12.0);
	SvcTensors.Data.Alignment = GetAligments(SvcTensors.Data.FrameShape[1], HubertLen);
	SvcTensors.Data.DDSPNoise = std::vector(IstftCount, params.DDSPNoiseScale);
	SvcTensors.Data.Noise = std::vector(NoiseSize, 0.f);
	for (auto& it : SvcTensors.Data.Noise)
		it = normal(gen) * params.NoiseScale;
	SvcTensors.Data.Speaker[0] = params.Chara;

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.HiddenUnit.data(),
		HubertSize,
		SvcTensors.Data.HiddenUnitShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.F0.data(),
		SvcTensors.Data.F0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Alignment.data(),
		SvcTensors.Data.Alignment.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.DDSPNoise.data(),
		SvcTensors.Data.DDSPNoise.size(),
		SvcTensors.Data.DDSPNoiseShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Noise.data(),
		SvcTensors.Data.Noise.size(),
		SvcTensors.Data.NoiseShape.data(),
		3
	));

	if (_SpeakerMix)
	{
		SvcTensors.Data.SpkMap = GetCurrectSpkMixData(SpkMap, SvcTensors.Data.FrameShape[1], params.Chara);
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.SpkMap.data(),
			SvcTensors.Data.SpkMap.size(),
			SvcTensors.Data.SpkShape.data(),
			2
		));
	}
	else
	{
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Speaker,
			1,
			SvcTensors.Data.OneShape,
			1
		));
	}

	if (_Volume)
	{
		SvcTensors.InputNames = InputNamesVol.data();
		SvcTensors.Data.Volume = InferTools::InterpFunc(Volume, long(Volume.size()), long(SvcTensors.Data.FrameShape[1]));
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Volume.data(),
			SvcTensors.Data.FrameShape[1],
			SvcTensors.Data.FrameShape.data(),
			2
		));
	}
	else
		SvcTensors.InputNames = InputNames.data();

	return SvcTensors;
}

MoeVoiceStudioTensorExtractor::Inputs RVCTensorExtractor::Extract(const std::vector<float>& HiddenUnit, const std::vector<float>& F0, const std::vector<float>& Volume, const std::vector<std::vector<float>>& SpkMap, Params params)
{
	Inputs SvcTensors;
	std::mt19937 gen(int(params.Seed));
	std::normal_distribution<float> normal(0, 1);
	auto HubertSize = HiddenUnit.size();
	const auto HubertLen = int64_t(HubertSize) / int64_t(_HiddenSize);
	SvcTensors.Data.FrameShape = { 1, int64_t(params.AudioSize * _SamplingRate / _SrcSamplingRate / _HopSize) };
	SvcTensors.Data.HiddenUnitShape = { 1, HubertLen, int64_t(_HiddenSize) };
	constexpr int64_t upSample = 2;
	const auto srcHubertSize = SvcTensors.Data.HiddenUnitShape[1];
	SvcTensors.Data.HiddenUnitShape[1] *= upSample;
	HubertSize *= upSample;
	SvcTensors.Data.FrameShape[1] = SvcTensors.Data.HiddenUnitShape[1];
	SvcTensors.Data.SpkShape = { SvcTensors.Data.FrameShape[1], int64_t(_NSpeaker) };
	SvcTensors.Data.NoiseShape = { 1, 192, SvcTensors.Data.FrameShape[1] };
	const auto NoiseSize = SvcTensors.Data.NoiseShape[1] * SvcTensors.Data.NoiseShape[2] * SvcTensors.Data.NoiseShape[0];

	SvcTensors.Data.HiddenUnit.reserve(HubertSize);
	for (int64_t itS = 0; itS < srcHubertSize; ++itS)
		for (int64_t itSS = 0; itSS < upSample; ++itSS)
			SvcTensors.Data.HiddenUnit.insert(SvcTensors.Data.HiddenUnit.end(), HiddenUnit.begin() + itS * (int64_t)_HiddenSize, HiddenUnit.begin() + (itS + 1) * (int64_t)_HiddenSize);
	SvcTensors.Data.Length[0] = SvcTensors.Data.HiddenUnitShape[1];
	SvcTensors.Data.F0 = GetInterpedF0(InferTools::InterpFunc(F0, long(F0.size()), long(SvcTensors.Data.HiddenUnitShape[1])));
	for (auto& it : SvcTensors.Data.F0)
		it *= (float)pow(2.0, static_cast<double>(params.upKeys) / 12.0);
	SvcTensors.Data.NSFF0 = GetNSFF0(SvcTensors.Data.F0);
	SvcTensors.Data.Alignment = GetAligments(SvcTensors.Data.FrameShape[1], HubertLen);
	SvcTensors.Data.Noise = std::vector(NoiseSize, 0.f);
	for (auto& it : SvcTensors.Data.Noise)
		it = normal(gen) * params.NoiseScale;
	SvcTensors.Data.Speaker[0] = params.Chara;

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.HiddenUnit.data(),
		HubertSize,
		SvcTensors.Data.HiddenUnitShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Length,
		1,
		SvcTensors.Data.OneShape,
		1
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.NSFF0.data(),
		SvcTensors.Data.NSFF0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.F0.data(),
		SvcTensors.Data.F0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	if (_SpeakerMix)
	{
		SvcTensors.Data.SpkMap = GetCurrectSpkMixData(SpkMap, SvcTensors.Data.FrameShape[1], params.Chara);
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.SpkMap.data(),
			SvcTensors.Data.SpkMap.size(),
			SvcTensors.Data.SpkShape.data(),
			2
		));
	}
	else
	{
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Speaker,
			1,
			SvcTensors.Data.OneShape,
			1
		));
	}

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Noise.data(),
		SvcTensors.Data.Noise.size(),
		SvcTensors.Data.NoiseShape.data(),
		3
	));

	if (_Volume)
	{
		SvcTensors.InputNames = InputNamesVol.data();
		SvcTensors.Data.Volume = InferTools::InterpFunc(Volume, long(Volume.size()), long(SvcTensors.Data.FrameShape[1]));
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Volume.data(),
			SvcTensors.Data.FrameShape[1],
			SvcTensors.Data.FrameShape.data(),
			2
		));
	}
	else
		SvcTensors.InputNames = InputNames.data();

	return SvcTensors;
}

MoeVoiceStudioTensorExtractor::Inputs DiffSvcTensorExtractor::Extract(const std::vector<float>& HiddenUnit, const std::vector<float>& F0, const std::vector<float>& Volume, const std::vector<std::vector<float>>& SpkMap, Params params)
{
	Inputs SvcTensors;
	const auto HubertSize = HiddenUnit.size();
	const auto HubertLen = int64_t(HubertSize) / int64_t(_HiddenSize);
	SvcTensors.Data.FrameShape = { 1, int64_t(params.AudioSize * _SamplingRate / _SrcSamplingRate / _HopSize) };
	SvcTensors.Data.HiddenUnitShape = { 1, HubertLen, int64_t(_HiddenSize) };

	SvcTensors.Data.HiddenUnit = HiddenUnit;
	SvcTensors.Data.F0 = GetInterpedF0log(InferTools::InterpFunc(F0, long(F0.size()), long(SvcTensors.Data.FrameShape[1])), true);
	for (auto& it : SvcTensors.Data.F0)
		it *= (float)pow(2.0, static_cast<double>(params.upKeys) / 12.0);
	SvcTensors.Data.Alignment = GetAligments(SvcTensors.Data.FrameShape[1], HubertLen);
	SvcTensors.Data.Speaker[0] = params.Chara;

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.HiddenUnit.data(),
		HubertSize,
		SvcTensors.Data.HiddenUnitShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Alignment.data(),
		SvcTensors.Data.Alignment.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Speaker,
		1,
		SvcTensors.Data.OneShape,
		1
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.F0.data(),
		SvcTensors.Data.F0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.InputNames = InputNames.data();

	SvcTensors.OutputNames = OutputNames.data();

	SvcTensors.OutputCount = OutputNames.size();

	return SvcTensors;
}

MoeVoiceStudioTensorExtractor::Inputs DiffusionSvcTensorExtractor::Extract(const std::vector<float>& HiddenUnit, const std::vector<float>& F0, const std::vector<float>& Volume, const std::vector<std::vector<float>>& SpkMap, Params params)
{
	Inputs SvcTensors;
	const auto HubertSize = HiddenUnit.size();
	const auto HubertLen = int64_t(HubertSize) / int64_t(_HiddenSize);
	SvcTensors.Data.FrameShape = { 1, int64_t(params.AudioSize * _SamplingRate / _SrcSamplingRate / _HopSize) };
	SvcTensors.Data.HiddenUnitShape = { 1, HubertLen, int64_t(_HiddenSize) };
	SvcTensors.Data.SpkShape = { SvcTensors.Data.FrameShape[1], int64_t(_NSpeaker) };

	SvcTensors.Data.HiddenUnit = HiddenUnit;
	SvcTensors.Data.F0 = GetInterpedF0(InferTools::InterpFunc(F0, long(F0.size()), long(SvcTensors.Data.FrameShape[1])));
	for (auto& it : SvcTensors.Data.F0)
		it *= (float)pow(2.0, static_cast<double>(params.upKeys) / 12.0);
	SvcTensors.Data.Alignment = GetAligments(SvcTensors.Data.FrameShape[1], HubertLen);
	SvcTensors.Data.Speaker[0] = params.Chara;

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.HiddenUnit.data(),
		HubertSize,
		SvcTensors.Data.HiddenUnitShape.data(),
		3
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.Alignment.data(),
		SvcTensors.Data.Alignment.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
		Memory,
		SvcTensors.Data.F0.data(),
		SvcTensors.Data.F0.size(),
		SvcTensors.Data.FrameShape.data(),
		2
	));

	if (_Volume)
	{
		SvcTensors.InputNames = InputNamesVol.data();
		SvcTensors.Data.Volume = InferTools::InterpFunc(Volume, long(Volume.size()), long(SvcTensors.Data.FrameShape[1]));
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Volume.data(),
			SvcTensors.Data.FrameShape[1],
			SvcTensors.Data.FrameShape.data(),
			2
		));
	}
	else
		SvcTensors.InputNames = InputNames.data();

	if (_SpeakerMix)
	{
		SvcTensors.Data.SpkMap = GetCurrectSpkMixData(SpkMap, SvcTensors.Data.FrameShape[1], params.Chara);
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.SpkMap.data(),
			SvcTensors.Data.SpkMap.size(),
			SvcTensors.Data.SpkShape.data(),
			2
		));
	}
	else
	{
		SvcTensors.Tensor.emplace_back(Ort::Value::CreateTensor(
			Memory,
			SvcTensors.Data.Speaker,
			1,
			SvcTensors.Data.OneShape,
			1
		));
	}

	SvcTensors.OutputNames = OutputNames.data();

	SvcTensors.OutputCount = OutputNames.size();

	return SvcTensors;
}

MoeVoiceStudioTensorExtractorEnd