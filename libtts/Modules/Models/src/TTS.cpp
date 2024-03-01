#include "../header/TTS.hpp"
#include <set>

MoeVoiceStudioCoreHeader

std::unordered_map<std::wstring, int64_t> _ACCMAP{
	{ L"[acc_6]", 6 }, { L"[acc_5]", 5 }, { L"[acc_4]", 4 }, { L"[acc_3]", 3 }, { L"[acc_2]", 2 }, { L"[acc_1]", 1 },
	{ L"[acc_-4]", -4 }, { L"[acc_-3]", -3 }, { L"[acc_-2]", -2 }, { L"[acc_-1]", -1 },
};

std::unordered_map<std::string, std::string> _LANGREPMAP{
	{"CHINESE", "ZH"}, { "CHINA", "ZH" },
	{ "JAPAN", "JP" }, { "JAPANESE", "JP" }, { "JA", "JP" },
	{ "ENGLISH", "EN" }
};

std::set ChineseVowel{L'a', L'e', L'i', L'o', L'u', L'A', L'E', L'I', L'O', L'U'};

std::set ByteSymbol{L',', L'.', L'?', L'/', L'\"', L'\'', L';', L':', L'!', L' ', L'…' };

void PreventNoobsInputErrors(std::string& _Src)
{
	for (auto& i : _Src)
		i = char(toupper(i));
	const auto res = _LANGREPMAP.find(_Src);
	if (res != _LANGREPMAP.end())
		_Src = res->second;
}

TextToSpeech::TextToSpeech(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_) : MoeVoiceStudioModule(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioTextToSpeech");
}

/**Preprocess**/

std::vector<MoeVSProjectSpace::MoeVSTTSSeq> TextToSpeech::GetInputSeqs(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams) const
{
	auto InputSeq = GetInputSeqsStatic(_Input, _InitParams);
	return SpecializeInputSeqs(InputSeq);
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

std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& TextToSpeech::SpecializeInputSeqs(std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Seq) const
{
	for (auto& _Temp : _Seq)
	{
		PreventNoobsInputErrors(_Temp.LanguageSymbol);
		if (LanguageSymbol2ID.find(_Temp.LanguageSymbol) == LanguageSymbol2ID.end())
			_Temp.LanguageSymbol = LanguageSymbol2ID.begin()->first;

		const int64_t FirstToneIdx = LanguageSymbol2TonesBegin.at(_Temp.LanguageSymbol);

		for(auto& _iter : _Temp.SlicedTokens)
		{
			if (!_iter.Phonemes.empty())
			{
				auto RtnData = SplitTonesFromTokens(_iter.Phonemes, _iter.Tones, FirstToneIdx, _Temp.LanguageSymbol);
				_iter.Phonemes = std::move(std::get<0>(RtnData));
				_iter.Tones = std::move(std::get<1>(RtnData));
			}
			else if (FirstToneIdx)
				for (auto& it : _iter.Tones)
					it += FirstToneIdx;
		}
	}
	return _Seq;
}

/**Infer**/

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
	return MoeVSG2P::NormalizeText(_Input, GetLanguageSymbolWithLanguageId(LanguageId));
}

/**MapCast**/

const std::wstring& TextToSpeech::GetTokenizerNameWithLanguageSymbol(const std::string& LanguageSymbol) const
{
	const auto TokenizerNamePair = LanguageSymbol2TokenizerName.find(LanguageSymbol);
	if (TokenizerNamePair != LanguageSymbol2TokenizerName.end())
		return TokenizerNamePair->second;
	return NoneString;
}

const MoeVSG2P::Tokenizer& TextToSpeech::GetTokenizer(const std::wstring& TokenizerName) const
{
	const auto TokenizerPair = Tokenizers.find(TokenizerName);
	if (TokenizerPair != Tokenizers.end())
		return TokenizerPair->second;
	return MoeVSG2P::GetEmptyVocabTokenizer();
}

const MoeVSG2P::Tokenizer& TextToSpeech::GetTokenizerWithLanguageSymbol(const std::string& LanguageSymbol) const
{
	const auto& TokenizerName = GetTokenizerNameWithLanguageSymbol(LanguageSymbol);
	return GetTokenizer(TokenizerName);
}

const std::string& TextToSpeech::GetLanguageSymbolWithLanguageId(int64_t LanguageId) const
{
	const auto LanguageIDPair = LanguageID2Symbol.find(LanguageId);
	if (LanguageIDPair != LanguageID2Symbol.end())
		return LanguageIDPair->second;
	return NoneAString;
}

int64_t TextToSpeech::GetLanguageIdWithLanguageSymbol(const std::string& LanguageSymbol) const
{
	const auto LanguageIDPair = LanguageSymbol2ID.find(LanguageSymbol);
	if (LanguageIDPair != LanguageSymbol2ID.end())
		return LanguageIDPair->second;
	return _atoi64(LanguageSymbol.c_str());
}

int64_t TextToSpeech::GetBertIdWithLanguageSymbol(const std::string& LanguageSymbol) const
{
	const auto LanguageIDPair = LanguageSymbol2BertID.find(LanguageSymbol);
	if (LanguageIDPair != LanguageSymbol2BertID.end())
		return LanguageIDPair->second;
	return -1;
}

int64_t TextToSpeech::GetTonesBegin(const std::string& LanguageSymbol) const
{
	const auto LanguageIDPair = LanguageSymbol2TonesBegin.find(LanguageSymbol);
	if (LanguageIDPair != LanguageSymbol2TonesBegin.end())
		return LanguageIDPair->second;
	return _atoi64(LanguageSymbol.c_str());
}

int64_t TextToSpeech::GetTonesBegin(int64_t LanguageId) const
{
	const auto& LanguageSymbol = GetLanguageSymbolWithLanguageId(LanguageId);
	return GetTonesBegin(LanguageSymbol);
}

int64_t TextToSpeech::GetSpeakerIdWithSpeakerName(const std::wstring& SpeakerName) const
{
	const auto SpeakerIDPair = SpeakerName2ID.find(SpeakerName);
	if (SpeakerIDPair != SpeakerName2ID.end())
		return SpeakerIDPair->second;
	return _wtoi64(SpeakerName.c_str());
}

const std::wstring& TextToSpeech::GetSpeakerNameWithSpeakerId(int64_t SpeakerID) const
{
	const auto SpeakerIDPair = SpeakerID2Name.find(SpeakerID);
	if (SpeakerIDPair != SpeakerID2Name.end())
		return SpeakerIDPair->second;
	return NoneString;
}

int64_t TextToSpeech::GetBertIndexWithName(const std::wstring& BertName) const
{
	const auto BertIDPair = BertName2Index.find(BertName);
	if (BertIDPair != BertName2Index.end())
		return BertIDPair->second;
	return _wtoi64(BertName.c_str());
}

const std::wstring& TextToSpeech::GetBertNameWithIndex(int64_t Index) const
{
	const auto BertIDPair = Index2BertName.find(Index);
	if (BertIDPair != Index2BertName.end())
		return BertIDPair->second;
	return NoneString;
}

std::vector<std::wstring> TextToSpeech::CleanText(const std::wstring& SrcText, const std::wstring& PlaceholderSymbol,
	const std::wstring& ExtraInfo, const std::string& LanguageSymbol) const
{
	return CleanText(Cleaner, SrcText, PlaceholderSymbol, ExtraInfo, LanguageSymbol);
}

std::vector<int64_t> TextToSpeech::CleanedSeq2Indices(const std::vector<std::wstring>& Seq) const
{
	std::vector<int64_t> Indices;
	Indices.reserve(Seq.size() * 3);
	for (const auto& i : Seq)
	{
		if (AddBlank)
			Indices.emplace_back(0);
		const auto Res = Symbols.find(i);
		if (Res != Symbols.end())
			Indices.emplace_back(Res->second);
		else
			Indices.emplace_back(UNKID);
	}
	if (AddBlank)
		Indices.emplace_back(0);
	return Indices;
}

/**STATIC**/

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

std::tuple<std::vector<std::wstring>, std::vector<int64_t>> TextToSpeech::SplitTonesFromTokens(const std::vector<std::wstring>& SrcSeq, const std::vector<int64_t>& TonesRef, int64_t TonesBegin, const std::string& LanguageSymbol)
{
	if (SrcSeq.empty())
		return{ {},{} };
	std::vector<std::wstring> TempSeqVec;
	std::vector<int64_t> TempToneVec;
	TempSeqVec.reserve(SrcSeq.size());
	TempToneVec.reserve(SrcSeq.size());
	for (const auto& it : SrcSeq)
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
			if (TempToneVec.size() > 1 && *(TempToneVec.end() - 2) == 0 && LanguageSymbol == "ZH")
				*(TempToneVec.end() - 2) = *(TempToneVec.end() - 1);
		}
	}
	if (TempToneVec.size() == TonesRef.size())
	{
		for (size_t i = 0; i < TonesRef.size(); ++i)
			if (TonesRef[i] != 0)
				TempToneVec[i] = TonesRef[i];
	}

	if (TonesBegin)
		for (auto& it : TempToneVec)
			it += TonesBegin;

	return { std::move(TempSeqVec) ,std::move(TempToneVec) };
}

std::wregex PhonemeRegex(LR"(<ph>(.*)</ph>)");
std::vector<std::wstring> TextToSpeech::CleanText(const MoeVSG2P::MVSCleaner* TextCleaner, const std::wstring& SrcText, const std::wstring& PlaceholderSymbol, const std::wstring& ExtraInfo, const std::string& LanguageSymbol)
{
	std::wsmatch mat;
	if(TextCleaner)
	{
		if (std::regex_match(SrcText, mat, PhonemeRegex))
			return TextCleaner->DictReplace(mat[1].str(), PlaceholderSymbol);
		if (TextCleaner->G2pEnabled())
			return TextCleaner->DictReplace(TextCleaner->G2p(SrcText, PlaceholderSymbol, ExtraInfo, LanguageSymbol), PlaceholderSymbol);
		return TextCleaner->DictReplace(SrcText, PlaceholderSymbol);
	}
	return { SrcText };
}

MoeVoiceStudioCoreEnd