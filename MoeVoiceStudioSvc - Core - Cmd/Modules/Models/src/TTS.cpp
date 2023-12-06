#include "../header/TTS.hpp"
#include <set>

MoeVoiceStudioCoreHeader
	std::unordered_map<std::wstring, int64_t> _ACCMAP{
	{ L"[acc_6]", 6 }, { L"[acc_5]", 5 }, { L"[acc_4]", 4 }, { L"[acc_3]", 3 }, { L"[acc_2]", 2 }, { L"[acc_1]", 1 },
	{ L"[acc_-4]", -4 }, { L"[acc_-3]", -3 }, { L"[acc_-2]", -2 }, { L"[acc_-1]", -1 },
};

TextToSpeech::TextToSpeech(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_) : MoeVoiceStudioModule(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioTextToSpeech");
}

std::vector<MoeVSProjectSpace::MoeVSTTSSeq> TextToSpeech::GetInputSeqs(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams) const
{
	auto InputSeq = GetInputSeqsStatic(_Input, _InitParams);
	return SpecializeInputSeqs(InputSeq);
}

std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& TextToSpeech::SpecializeInputSeqs(std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Seq) const
{
	for (auto& _Temp : _Seq)
	{
		const int64_t FirstToneIdx = LanguageTones.at(_Temp.LanguageSymbol);

		for(auto& _iter : _Temp.SlicedTokens)
		{
			if (!_iter.Phonemes.empty())
			{
				std::vector<std::wstring> TempSeqVec;
				TempSeqVec.reserve(_iter.Phonemes.size());
				std::vector<int64_t> TempToneVec;
				TempToneVec.reserve(_iter.Phonemes.size());

				for (const auto& it : _iter.Phonemes)
				{
					if (_ACCMAP.find(it) == _ACCMAP.end())
					{
						TempSeqVec.emplace_back(it);
						TempToneVec.emplace_back(0);
					}
					else
					{
						if (TempToneVec.empty())
							continue;
						*(TempToneVec.end() - 1) = _ACCMAP.at(it);
						if (TempToneVec.size() > 1 && *(TempToneVec.end() - 2) == 0)
							*(TempToneVec.end() - 2) = *(TempToneVec.end() - 1);
					}
				}
				if (TempToneVec.size() != _iter.Tones.size())
					TempToneVec.resize(_iter.Tones.size(), 0);
				_iter.Phonemes = TempSeqVec;
				for (size_t i = 0; i < _iter.Tones.size(); ++i)
					if (_iter.Tones[i] == 0)
						_iter.Tones[i] = TempToneVec[i];
			}

			if (FirstToneIdx)
				for (auto& it : _iter.Tones)
					it += FirstToneIdx;
		}
	}
	return _Seq;
}

std::vector<MoeVSProjectSpace::MoeVSTTSSeq> TextToSpeech::GetInputSeqsStatic(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams)
{
	if (!_Input.IsArray())
		LibDLVoiceCodecThrow("JSON Type Must Be Array")
	const auto _InpArr = _Input.GetArray();
	std::vector<MoeVSProjectSpace::MoeVSTTSSeq> _TTSInputSeqs;
	_TTSInputSeqs.reserve(_InpArr.size());
	for (const auto& iter : _InpArr)
		_TTSInputSeqs.emplace_back(iter, _InitParams);
	return _TTSInputSeqs;
}

std::vector<float> TextToSpeech::GetEmotionVector(const std::vector<std::wstring>& src) const
{
	if (src.empty())
		return EmoLoader[0];
	std::vector<float> dst(1024, 0.0);
	uint64_t mul = 0;
	for(const auto& iter : src)
	{
		long emoId;
		const auto emoStr = to_byte_string(iter);
		if (!EmoJson[emoStr].Empty())
			emoId = EmoJson[emoStr].GetInt();
		else
			emoId = atoi(emoStr.c_str());
		auto emoVec = EmoLoader[emoId];
		for (size_t i = 0; i < 1024; ++i)
			dst[i] = dst[i] + (emoVec[i] - dst[i]) / (float)(mul + 1ull);
		++mul;
	}
	return dst;
}

std::vector<std::vector<bool>> TextToSpeech::generatePath(float* duration, size_t durationSize, size_t maskSize)
{
	for (size_t i = 1; i < maskSize; ++i)
		duration[i] = duration[i - 1] + duration[i];
	std::vector<std::vector<bool>> path(durationSize, std::vector<bool>(maskSize, false));
	//const auto path = new float[maskSize * durationSize];
	/*
	for (size_t i = 0; i < maskSize; ++i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] = (j < (size_t)duration[i] ? 1.0f : 0.0f);
	for (size_t i = maskSize - 1; i > 0ull; --i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] -= path[i-1][j];
	 */
	auto dur = (size_t)duration[0];
	for (size_t j = 0; j < dur; ++j)
		path[j][0] = true;
	/*
	for (size_t i = maskSize - 1; i > 0ull; --i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] = (j < (size_t)duration[i] && j >= (size_t)duration[i - 1]);
	std::vector<std::vector<float>> tpath(durationSize, std::vector<float>(maskSize));
	for (size_t i = 0; i < maskSize; ++i)
		for (size_t j = 0; j < durationSize; ++j)
			tpath[j][i] = path[i][j];
	 */
	for (size_t j = maskSize - 1; j > 0ull; --j)
	{
		dur = (size_t)duration[j];
		for (auto i = (size_t)duration[j - 1]; i < dur && i < durationSize; ++i)
			path[i][j] = true;
	}
	return path;
}

std::vector<std::vector<int16_t>> TextToSpeech::Inference(const std::wstring& _Seq, const MoeVSProjectSpace::MoeVSParams& _InferParams) const
{
	if (_Seq.empty())
		return {};
	return Inference(GetInputSeqs({ to_byte_string(_Seq), true }, _InferParams));
}

std::vector<std::vector<int16_t>> TextToSpeech::Inference(const MJson& _Inputs, const MoeVSProjectSpace::MoeVSParams& _InferParams) const
{
	return Inference(GetInputSeqs(_Inputs, _InferParams));
}

std::vector<std::wstring> TextToSpeech::Inference(std::wstring& _Datas, const MoeVSProjectSpace::MoeVSParams& _InferParams, const InferTools::SlicerSettings& _SlicerSettings) const
{
	return Inference(_Datas, _InferParams,false);
}

std::vector<std::wstring> TextToSpeech::Inference(const std::wstring& _Seq, const MoeVSProjectSpace::MoeVSParams& _InferParams, bool T) const
{
	const std::vector<std::vector<int16_t>> PCM = Inference(GetInputSeqs({ to_byte_string(_Seq), true }, _InferParams));
	std::vector<std::wstring> AudioFolders;

	if (_Seq.empty())
		return {};

	AudioFolders.reserve(PCM.size());
	for (const auto& i : PCM)
	{
		std::wstring OutFolder = GetCurrentFolder() + L"/Outputs/BatchInference";
		if (_waccess((OutFolder + L".wav").c_str(), 0) != -1)
		{
			for (size_t idx = 0; idx < 99999999; ++idx)
				if (_waccess((OutFolder + L" (" + std::to_wstring(idx) + L").wav").c_str(), 0) == -1)
				{
					OutFolder += L" (" + std::to_wstring(idx) + L").wav";
					break;
				}
		}
		else
			OutFolder += L".wav";
		AudioFolders.emplace_back(OutFolder);
		InferTools::Wav::WritePCMData(_samplingRate, 1, i, OutFolder);
	}
	return AudioFolders;
}

std::vector<std::vector<int16_t>> TextToSpeech::Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const
{
	MoeVSNotImplementedError
}

std::vector<size_t> TextToSpeech::GetAligments(size_t DstLen, size_t SrcLen)
{
	std::vector<size_t> bert2ph(DstLen + 1, 0);

	size_t startFrame = 0;
	const double ph_durs = static_cast<double>(DstLen) / static_cast<double>(SrcLen);
	for (size_t iph = 0; iph < SrcLen; ++iph)
	{
		const auto endFrame = static_cast<size_t>(round(static_cast<double>(iph) * ph_durs + ph_durs));
		for (auto j = startFrame; j < endFrame + 1; ++j)
			bert2ph[j] = static_cast<long long>(iph) + 1;
		startFrame = endFrame + 1;
	}
	return bert2ph;
}

std::set ChineseVowel{L'a', L'e', L'i', L'o', L'u', L'A', L'E', L'I', L'O', L'U'};
std::set ByteSymbol{L',', L'.', L'?', L'/', L'\"', L'\'', L';', L':', L'!', L' ', L'…' };

std::vector<size_t> TextToSpeech::AligPhoneAttn(const std::string& LanguageStr, const std::vector<std::wstring>& PhoneSeq, size_t BertSize) const
{
	size_t PhoneSize = PhoneSeq.size();
	if (AddBlank)
		PhoneSize = PhoneSize * 2 + 1;
	if(LanguageStr == "ZH")
	{
		std::vector<size_t> bert2ph(PhoneSize, 0);
		size_t startFrame = 1;
		for (size_t iph = 0; iph < PhoneSeq.size(); ++iph)
		{
			if (startFrame == BertSize)
				LibDLVoiceCodecThrow("AligError")
			if (AddBlank)
			{
				bert2ph[(iph + 1) * 2] = startFrame;
				bert2ph[(iph + 1) * 2 - 1] = startFrame;
			}
			else
				bert2ph[iph] = startFrame;
			if (!PhoneSeq[iph].empty() && ChineseVowel.find(PhoneSeq[iph][0]) != ChineseVowel.end())
				++startFrame;
			else if (ByteSymbol.find(PhoneSeq[iph][0]) != ByteSymbol.end())
				++startFrame;
			else if (Symbols.find(PhoneSeq[iph]) == Symbols.end())
				++startFrame;
			else if (PhoneSeq[iph] == L"UNK")
				++startFrame;
		}
		bert2ph.back() = startFrame;
		return bert2ph;
	}
	return GetAligments(PhoneSize, BertSize);
}

std::wstring TextToSpeech::TextNormalize(const std::wstring& _Input, int64_t LanguageId) const
{
	auto Iterator = LanguageMap.begin();
	while(Iterator != LanguageMap.end())
	{
		if (Iterator->second == LanguageId)
			break;
		++Iterator;
	}

	if (Iterator != LanguageMap.end())
		return MoeVSG2P::NormalizeText(_Input, Iterator->first);
	return _Input;
}

MoeVoiceStudioCoreEnd