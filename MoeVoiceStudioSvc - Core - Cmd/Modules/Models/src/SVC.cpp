#include "../header/SVC.hpp"
#include "../../InferTools/F0Extractor/F0ExtractorManager.hpp"

MoeVoiceStudioCoreHeader

SingingVoiceConversion::SingingVoiceConversion(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_) : MoeVoiceStudioModule(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioSingingVoiceConversion");
}

SingingVoiceConversion::~SingingVoiceConversion()
{
	delete hubert;
	hubert = nullptr;
}

std::vector<std::wstring> SingingVoiceConversion::Inference(std::wstring& _Paths,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
	const InferTools::SlicerSettings& _SlicerSettings) const
{
	MoeVSNotImplementedError;
}

std::vector<int16_t> SingingVoiceConversion::InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	MoeVSNotImplementedError;
}

std::vector<int16_t> SingingVoiceConversion::SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	MoeVSNotImplementedError;
}

std::vector<int16_t> SingingVoiceConversion::SliceInference(const MoeVSProjectSpace::MoeVSAudioSliceRef& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	MoeVSNotImplementedError;
}

std::vector<float> SingingVoiceConversion::ExtractVolume(const std::vector<int16_t>& OrgAudio, int hop_size)
{
	std::vector<double> Audio;
	Audio.reserve(OrgAudio.size() * 2);
	Audio.insert(Audio.end(), hop_size, double(OrgAudio[0]) / 32768.);
	for (const auto i : OrgAudio)
		Audio.emplace_back((double)i / 32768.);
	Audio.insert(Audio.end(), hop_size, double(OrgAudio[OrgAudio.size() - 1]) / 32768.);
	const size_t n_frames = (OrgAudio.size() / hop_size) + 1;
	std::vector<float> volume(n_frames);
	for (auto& i : Audio)
		i = pow(i, 2);
	int64_t index = 0;
	for (auto& i : volume)
	{
		i = sqrt((float)InferTools::getAvg(Audio.begin()._Ptr + index * hop_size, Audio.begin()._Ptr + (index + 1) * hop_size));
		++index;
	}
	return volume;
}

std::vector<float> SingingVoiceConversion::ExtractVolume(const std::vector<double>& OrgAudio) const
{
	std::vector<double> Audio;
	Audio.reserve(OrgAudio.size() * 2);
	Audio.insert(Audio.end(), HopSize, OrgAudio[0]);
	Audio.insert(Audio.end(), OrgAudio.begin(), OrgAudio.end());
	Audio.insert(Audio.end(), HopSize, OrgAudio[OrgAudio.size() - 1]);
	const size_t n_frames = (OrgAudio.size() / HopSize) + 1;
	std::vector<float> volume(n_frames);
	for (auto& i : Audio)
		i = pow(i, 2);
	int64_t index = 0;
	for (auto& i : volume)
	{
		i = sqrt((float)InferTools::getAvg(Audio.begin()._Ptr + index * HopSize, Audio.begin()._Ptr + (index + 1) * HopSize));
		++index;
	}
	return volume;
}

MoeVSProjectSpace::MoeVSAudioSlice SingingVoiceConversion::GetAudioSlice(const std::vector<int16_t>& input,
	const std::vector<size_t>& _slice_pos,
	const InferTools::SlicerSettings& _slicer)
{
	MoeVSProjectSpace::MoeVSAudioSlice audio_slice;
	for (size_t i = 1; i < _slice_pos.size(); i++)
	{
		const bool is_not_mute = abs(InferTools::getAvg((input.data() + _slice_pos[i - 1]), (input.data() + _slice_pos[i]))) > _slicer.Threshold;
		audio_slice.IsNotMute.emplace_back(is_not_mute);
		audio_slice.OrgLen.emplace_back(_slice_pos[i] - _slice_pos[i - 1]);
		if (is_not_mute)
			audio_slice.Audio.emplace_back((input.data() + _slice_pos[i - 1]), (input.data() + _slice_pos[i]));
		else
			audio_slice.Audio.emplace_back();
	}
	return audio_slice;
}

void SingingVoiceConversion::PreProcessAudio(MoeVSProjectSpace::MoeVSAudioSlice& input,
	int __SamplingRate, int __HopSize, const std::wstring& _f0_method)
{
	const auto F0Extractor = MoeVSF0Extractor::GetF0Extractor(_f0_method, __SamplingRate, __HopSize);
	const auto num_slice = input.Audio.size();
	for (size_t i = 0; i < num_slice; ++i)
	{
		if (input.IsNotMute[i])
		{
			input.F0.emplace_back(F0Extractor->ExtractF0(input.Audio[i], input.Audio[i].size() / __HopSize));
			input.Volume.emplace_back(ExtractVolume(input.Audio[i], __HopSize));
		}
		else
		{
			input.F0.emplace_back();
			input.Volume.emplace_back();
		}
		input.Speaker.emplace_back();
	}
}

MoeVoiceStudioCoreEnd