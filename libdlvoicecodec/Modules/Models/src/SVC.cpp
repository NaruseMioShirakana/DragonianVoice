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

std::vector<int16_t> SingingVoiceConversion::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	MoeVSNotImplementedError;
}

std::vector<int16_t> SingingVoiceConversion::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process) const
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

MoeVSProjectSpace::MoeVoiceStudioSvcData SingingVoiceConversion::GetAudioSlice(const std::vector<int16_t>& input,
	const std::vector<size_t>& _slice_pos,
	const InferTools::SlicerSettings& _slicer)
{
	MoeVSProjectSpace::MoeVoiceStudioSvcData audio_slice;
	for (size_t i = 1; i < _slice_pos.size(); i++)
	{
		MoeVSProjectSpace::MoeVoiceStudioSvcSlice _CurSlice;
		const bool is_not_mute = abs(InferTools::getAvg((input.data() + _slice_pos[i - 1]), (input.data() + _slice_pos[i]))) > _slicer.Threshold;
		_CurSlice.IsNotMute = is_not_mute;
		_CurSlice.OrgLen = long(_slice_pos[i] - _slice_pos[i - 1]);
		if (is_not_mute)
			_CurSlice.Audio = { (input.data() + _slice_pos[i - 1]), (input.data() + _slice_pos[i]) };
		else
			_CurSlice.Audio.clear();
		audio_slice.Slices.emplace_back(std::move(_CurSlice));
	}
	return audio_slice;
}

void SingingVoiceConversion::PreProcessAudio(MoeVSProjectSpace::MoeVoiceStudioSvcData& input,
	int __SamplingRate, int __HopSize, const std::wstring& _f0_method)
{
	const auto F0Extractor = MoeVSF0Extractor::GetF0Extractor(_f0_method, __SamplingRate, __HopSize);
	const auto num_slice = input.Slices.size();
	for (size_t i = 0; i < num_slice; ++i)
	{
		if (input.Slices[i].IsNotMute)
		{
			input.Slices[i].F0 = F0Extractor->ExtractF0(input.Slices[i].Audio, input.Slices[i].Audio.size() / __HopSize);
			input.Slices[i].Volume = ExtractVolume(input.Slices[i].Audio, __HopSize);
		}
		else
		{
			input.Slices[i].F0.clear();
			input.Slices[i].Volume.clear();
		}
		input.Slices[i].Speaker.clear();
	}
}

MoeVoiceStudioCoreEnd