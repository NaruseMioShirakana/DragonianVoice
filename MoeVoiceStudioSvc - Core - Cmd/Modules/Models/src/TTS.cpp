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
	if (!_Input.IsArray())
		throw std::exception("JSON Type Must Be Array");
	const auto _InpArr = _Input.GetArray();
	std::vector<MoeVSProjectSpace::MoeVSTTSSeq> _TTSInputSeqs;
	_TTSInputSeqs.reserve(_InpArr.size());
	for(const auto& iter : _InpArr)
	{
		MoeVSProjectSpace::MoeVSTTSSeq _Temp;
		const bool TokenFieldIsStr = iter.HasMember("Tokens") && iter["Tokens"].IsString() && !iter["Tokens"].Empty();
		const bool SeqFieldIsStr = iter.HasMember("Seq") && iter["Seq"].IsString() && !iter["Seq"].Empty();

		if (iter.HasMember("LanguageID") && iter["LanguageID"].IsString())
			_Temp.Langstr = iter["LanguageID"].GetString();
		if (iter.HasMember("LanguageID") && iter["LanguageID"].IsString() &&
			LanguageMap.find(iter["LanguageID"].GetString()) != LanguageMap.end())
			_Temp.TotLang = LanguageMap.at(iter["LanguageID"].GetString());
		else
			_Temp.TotLang = _InitParams.Language;

		const int64_t FirstToneIdx = GetLanguageToneIdx(_Temp.TotLang);

		if (iter.HasMember("G2PAdditionalInfo") && iter["G2PAdditionalInfo"].IsString() && !iter["G2PAdditionalInfo"].Empty())
			_Temp.AdditionalInfo = to_wide_string(iter["G2PAdditionalInfo"].GetString());
		else
			_Temp.AdditionalInfo = _InitParams.AdditionalInfo;

		if (iter.HasMember("PlaceHolderSymbol") && iter["PlaceHolderSymbol"].IsString())
			_Temp.PlaceHolderSymbol = to_wide_string(iter["PlaceHolderSymbol"].GetString());
		else
			_Temp.PlaceHolderSymbol = _InitParams.PlaceHolderSymbol;

		if(TokenFieldIsStr && SeqFieldIsStr)
		{
			_Temp.SeqStr = to_wide_string(iter["Tokens"].GetString());
			auto TempString = to_wide_string(iter["Seq"].GetString());
			if (TempString.find(L"[ph]") == 0)
				_Temp.Seq = Cleaner->DictReplace(TempString.substr(4), _Temp.PlaceHolderSymbol);
			else
				_Temp.Seq = Cleaner->DictReplace(Cleaner->G2p(TempString, _Temp.PlaceHolderSymbol, _Temp.AdditionalInfo, _Temp.TotLang), _Temp.PlaceHolderSymbol);
		}
		else if (TokenFieldIsStr)
			_Temp.SeqStr = to_wide_string(iter["Tokens"].GetString());
		else if(SeqFieldIsStr)
			_Temp.SeqStr = to_wide_string(iter["Seq"].GetString());
		else
			throw std::exception("You Should Input Tokens To Inference");
		if (iter.HasMember("Seq") && iter["Seq"].IsArray())
		{
			const auto SeqObject = iter["Seq"];
			if (!SeqObject.Empty())
				for (const auto& j : SeqObject.GetArray())
					_Temp.Seq.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : std::wstring());
			else if(_Temp.SeqStr.empty())
				throw std::exception("You Should Input Tokens To Inference");
		}

		if(_Temp.SeqStr.empty())
			throw std::exception("You Should Input Tokens To Inference");

		if (iter.HasMember("Tones") && iter["Tones"].IsArray())
			for (const auto& j : iter["Tones"].GetArray())
				_Temp.Tones.emplace_back(j.IsInt() ? j.GetInt() + FirstToneIdx : 0);
		if (iter.HasMember("Durations") && iter["Durations"].IsArray())
			for (const auto& j : iter["Durations"].GetArray())
				_Temp.Durations.emplace_back(j.IsInt() ? j.GetInt() : 0);
		if (iter.HasMember("Language") && iter["Language"].IsArray())
			for (const auto& j : iter["Language"].GetArray())
				_Temp.Language.emplace_back(j.IsInt() ? j.GetInt() : (j.IsString() ? LanguageMap.at(j.GetString()) : 0));
		if (iter.HasMember("SpeakerMix") && iter["SpeakerMix"].IsArray())
			for (const auto& j : iter["SpeakerMix"].GetArray())
				_Temp.SpeakerMix.emplace_back(j.IsFloat() ? j.GetFloat() : 0.f);
		else
			_Temp.SpeakerMix = _InitParams.SpeakerMix;
		if (iter.HasMember("EmotionPrompt") && iter["EmotionPrompt"].IsArray())
			for (const auto& j : iter["EmotionPrompt"].GetArray())
				_Temp.EmotionPrompt.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : std::wstring());
		else
			_Temp.EmotionPrompt = _InitParams.EmotionPrompt;
		if (iter.HasMember("NoiseScale") && iter["NoiseScale"].IsFloat())
			_Temp.NoiseScale = iter["NoiseScale"].GetFloat();
		else
			_Temp.NoiseScale = _InitParams.NoiseScale;
		if (iter.HasMember("LengthScale") && iter["LengthScale"].IsFloat())
			_Temp.LengthScale = iter["LengthScale"].GetFloat();
		else
			_Temp.LengthScale = _InitParams.LengthScale;
		if (iter.HasMember("RestTime") && iter["RestTime"].IsFloat())
			_Temp.RestTime = iter["RestTime"].GetFloat();
		else
			_Temp.RestTime = _InitParams.RestTime;
		if (iter.HasMember("DurationPredictorNoiseScale") && iter["DurationPredictorNoiseScale"].IsFloat())
			_Temp.DurationPredictorNoiseScale = iter["DurationPredictorNoiseScale"].GetFloat();
		else
			_Temp.DurationPredictorNoiseScale = _InitParams.DurationPredictorNoiseScale;
		if (iter.HasMember("FactorDpSdp") && iter["FactorDpSdp"].IsFloat())
			_Temp.FactorDpSdp = iter["FactorDpSdp"].GetFloat();
		else
			_Temp.FactorDpSdp = _InitParams.FactorDpSdp;
		if (iter.HasMember("GateThreshold") && iter["GateThreshold"].IsFloat())
			_Temp.GateThreshold = iter["GateThreshold"].GetFloat();
		else
			_Temp.GateThreshold = _InitParams.GateThreshold;
		if (iter.HasMember("MaxDecodeStep") && iter["MaxDecodeStep"].IsFloat())
			_Temp.MaxDecodeStep = iter["MaxDecodeStep"].GetInt();
		else
			_Temp.MaxDecodeStep = _InitParams.MaxDecodeStep;
		if (iter.HasMember("Seed") && iter["Seed"].IsInt())
			_Temp.Seed = iter["Seed"].GetInt();
		else
			_Temp.Seed = _InitParams.Seed;
		if (iter.HasMember("SpeakerId") && iter["SpeakerId"].IsInt())
			_Temp.SpeakerId = iter["SpeakerId"].GetInt();
		else
			_Temp.SpeakerId = _InitParams.SpeakerId;

		if (_Temp.MaxDecodeStep < 500) _Temp.MaxDecodeStep = 500;
		if (_Temp.GateThreshold > 0.98f) _Temp.GateThreshold = 0.98f;
		if (_Temp.GateThreshold < 0.2f) _Temp.GateThreshold = 0.2f;
		if (_Temp.FactorDpSdp > 1.f) _Temp.FactorDpSdp = 1.f;
		if (_Temp.FactorDpSdp < 0.f) _Temp.FactorDpSdp = 0.f;
		if (_Temp.DurationPredictorNoiseScale > 10.f) _Temp.DurationPredictorNoiseScale = 10.f;
		if (_Temp.DurationPredictorNoiseScale < 0.f) _Temp.DurationPredictorNoiseScale = 0.f;
		if (_Temp.RestTime > 30.f) _Temp.RestTime = 30.f;
		if (_Temp.LengthScale > 10.f) _Temp.LengthScale = 10.f;
		if (_Temp.LengthScale < 0.1f) _Temp.LengthScale = 0.1f;

		if (!_Temp.SeqStr.empty() && _Temp.Seq.empty())
		{
			if (_Temp.SeqStr.find(L"[ph]") == 0)
				_Temp.Seq = Cleaner->DictReplace(_Temp.SeqStr.substr(4), _Temp.PlaceHolderSymbol);
			else
				_Temp.Seq = Cleaner->DictReplace(Cleaner->G2p(_Temp.SeqStr, _Temp.PlaceHolderSymbol, _Temp.AdditionalInfo, _Temp.TotLang), _Temp.PlaceHolderSymbol);
		}
		_TTSInputSeqs.emplace_back(std::move(_Temp));
	}
	return _TTSInputSeqs;
}

std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& TextToSpeech::SpecializeInputSeqs(std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Seq)
{
	for (auto& _Temp : _Seq)
	{
		if (LanguageMap.find(_Temp.Langstr) != LanguageMap.end())
			_Temp.TotLang = LanguageMap.at(_Temp.Langstr);
		const int64_t FirstToneIdx = GetLanguageToneIdx(_Temp.TotLang);
		if(_Temp.Language.empty() && !_Temp.LangstrSeq.empty())
		{
			for(const auto& it : _Temp.LangstrSeq)
			{
				const auto rst = LanguageMap.find(it);
				if (rst != LanguageMap.end())
					_Temp.Language.emplace_back(rst->second);
				else
					_Temp.Language.emplace_back(_Temp.TotLang);
			}
		}

		if(!_Temp.TempStr.empty() && _Temp.Seq.empty())
		{
			if (_Temp.TempStr.find(L"[ph]") == 0)
				_Temp.Seq = Cleaner->DictReplace(_Temp.TempStr.substr(4), _Temp.PlaceHolderSymbol);
			else
				_Temp.Seq = Cleaner->DictReplace(Cleaner->G2p(_Temp.TempStr, _Temp.PlaceHolderSymbol, _Temp.AdditionalInfo, _Temp.TotLang), _Temp.PlaceHolderSymbol);
		}
		if (!_Temp.SeqStr.empty() && _Temp.Seq.empty())
		{
			if (_Temp.SeqStr.find(L"[ph]") == 0)
				_Temp.Seq = Cleaner->DictReplace(_Temp.SeqStr.substr(4), _Temp.PlaceHolderSymbol);
			else
				_Temp.Seq = Cleaner->DictReplace(Cleaner->G2p(_Temp.SeqStr, _Temp.PlaceHolderSymbol, _Temp.AdditionalInfo, _Temp.TotLang), _Temp.PlaceHolderSymbol);
		}

		std::vector<std::wstring> TempSeqVec;
		TempSeqVec.reserve(_Temp.Seq.size());
		std::vector<int64_t> TempToneVec;
		TempToneVec.reserve(_Temp.Seq.size());

		for (const auto& it : _Temp.Seq)
		{
			if(_ACCMAP.find(it) == _ACCMAP.end())
			{
				TempSeqVec.emplace_back(it);
				TempToneVec.emplace_back(0);
			}
			else
			{
				if(TempToneVec.empty())
					continue;
				*(TempToneVec.end() - 1) = _ACCMAP.at(it);
				if (TempToneVec.size() > 1 && *(TempToneVec.end() - 2) == 0)
					*(TempToneVec.end() - 2) = *(TempToneVec.end() - 1);
			}
		}
		if (TempToneVec.size() != _Temp.Tones.size())
			TempToneVec.resize(_Temp.Tones.size(), 0);
		_Temp.Seq = TempSeqVec;
		for (size_t i = 0; i < _Temp.Tones.size(); ++i)
			if (_Temp.Tones[i] == 0)
				_Temp.Tones[i] = TempToneVec[i];

		if (FirstToneIdx)
			for (auto& it : _Temp.Tones)
				it += FirstToneIdx;
	}
	return _Seq;
}

std::vector<MoeVSProjectSpace::MoeVSTTSSeq> TextToSpeech::GetInputSeqsStatic(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams)
{
	if (!_Input.IsArray())
		throw std::exception("JSON Type Must Be Array");
	const auto _InpArr = _Input.GetArray();
	std::vector<MoeVSProjectSpace::MoeVSTTSSeq> _TTSInputSeqs;
	_TTSInputSeqs.reserve(_InpArr.size());
	for (const auto& iter : _InpArr)
	{
		MoeVSProjectSpace::MoeVSTTSSeq _Temp;
		const bool TokenFieldIsStr = iter.HasMember("Tokens") && iter["Tokens"].IsString() && !iter["Tokens"].Empty();
		const bool SeqFieldIsStr = iter.HasMember("Seq") && iter["Seq"].IsString() && !iter["Seq"].Empty();

		if (iter.HasMember("LanguageID") && iter["LanguageID"].IsString())
			_Temp.Langstr = iter["LanguageID"].GetString();
		_Temp.TotLang = _InitParams.Language;

		if (iter.HasMember("G2PAdditionalInfo") && iter["G2PAdditionalInfo"].IsString() && !iter["G2PAdditionalInfo"].Empty())
			_Temp.AdditionalInfo = to_wide_string(iter["G2PAdditionalInfo"].GetString());
		else
			_Temp.AdditionalInfo = _InitParams.AdditionalInfo;

		if (iter.HasMember("PlaceHolderSymbol") && iter["PlaceHolderSymbol"].IsString())
			_Temp.PlaceHolderSymbol = to_wide_string(iter["PlaceHolderSymbol"].GetString());
		else
			_Temp.PlaceHolderSymbol = _InitParams.PlaceHolderSymbol;

		if (TokenFieldIsStr && SeqFieldIsStr)
		{
			_Temp.SeqStr = to_wide_string(iter["Tokens"].GetString());
			_Temp.TempStr = to_wide_string(iter["Seq"].GetString());
		}
		else if (TokenFieldIsStr)
			_Temp.SeqStr = to_wide_string(iter["Tokens"].GetString());
		else if (SeqFieldIsStr)
			_Temp.SeqStr = to_wide_string(iter["Seq"].GetString());
		else
			throw std::exception("You Should Input Tokens To Inference");
		if (iter.HasMember("Seq") && iter["Seq"].IsArray())
		{
			const auto SeqObject = iter["Seq"];
			if (!SeqObject.Empty())
				for (const auto& j : SeqObject.GetArray())
					_Temp.Seq.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : std::wstring());
			else if (_Temp.SeqStr.empty())
				throw std::exception("You Should Input Tokens To Inference");
		}

		if (_Temp.SeqStr.empty())
			throw std::exception("You Should Input Tokens To Inference");

		if (iter.HasMember("Tones") && iter["Tones"].IsArray())
			for (const auto& j : iter["Tones"].GetArray())
				_Temp.Tones.emplace_back(j.IsInt() ? j.GetInt() : 0);
		if (iter.HasMember("Durations") && iter["Durations"].IsArray())
			for (const auto& j : iter["Durations"].GetArray())
				_Temp.Durations.emplace_back(j.IsInt() ? j.GetInt() : 0);
		if (iter.HasMember("Language") && iter["Language"].IsArray())
		{
			const auto LanguageArr = iter["Language"].GetArray();
			if(!LanguageArr.empty())
			{
				if (LanguageArr[0].IsString())
					for (const auto& j : LanguageArr)
						_Temp.LangstrSeq.emplace_back(j.GetString());
				else
					for (const auto& j : LanguageArr)
						_Temp.Language.emplace_back(j.GetInt());
			}
		}
		if (iter.HasMember("SpeakerMix") && iter["SpeakerMix"].IsArray())
			for (const auto& j : iter["SpeakerMix"].GetArray())
				_Temp.SpeakerMix.emplace_back(j.IsFloat() ? j.GetFloat() : 0.f);
		else
			_Temp.SpeakerMix = _InitParams.SpeakerMix;
		if (iter.HasMember("EmotionPrompt") && iter["EmotionPrompt"].IsArray())
			for (const auto& j : iter["EmotionPrompt"].GetArray())
				_Temp.EmotionPrompt.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : std::wstring());
		else
			_Temp.EmotionPrompt = _InitParams.EmotionPrompt;
		if (iter.HasMember("NoiseScale") && iter["NoiseScale"].IsFloat())
			_Temp.NoiseScale = iter["NoiseScale"].GetFloat();
		else
			_Temp.NoiseScale = _InitParams.NoiseScale;
		if (iter.HasMember("LengthScale") && iter["LengthScale"].IsFloat())
			_Temp.LengthScale = iter["LengthScale"].GetFloat();
		else
			_Temp.LengthScale = _InitParams.LengthScale;
		if (iter.HasMember("RestTime") && iter["RestTime"].IsFloat())
			_Temp.RestTime = iter["RestTime"].GetFloat();
		else
			_Temp.RestTime = _InitParams.RestTime;
		if (iter.HasMember("DurationPredictorNoiseScale") && iter["DurationPredictorNoiseScale"].IsFloat())
			_Temp.DurationPredictorNoiseScale = iter["DurationPredictorNoiseScale"].GetFloat();
		else
			_Temp.DurationPredictorNoiseScale = _InitParams.DurationPredictorNoiseScale;
		if (iter.HasMember("FactorDpSdp") && iter["FactorDpSdp"].IsFloat())
			_Temp.FactorDpSdp = iter["FactorDpSdp"].GetFloat();
		else
			_Temp.FactorDpSdp = _InitParams.FactorDpSdp;
		if (iter.HasMember("GateThreshold") && iter["GateThreshold"].IsFloat())
			_Temp.GateThreshold = iter["GateThreshold"].GetFloat();
		else
			_Temp.GateThreshold = _InitParams.GateThreshold;
		if (iter.HasMember("MaxDecodeStep") && iter["MaxDecodeStep"].IsFloat())
			_Temp.MaxDecodeStep = iter["MaxDecodeStep"].GetInt();
		else
			_Temp.MaxDecodeStep = _InitParams.MaxDecodeStep;
		if (iter.HasMember("Seed") && iter["Seed"].IsInt())
			_Temp.Seed = iter["Seed"].GetInt();
		else
			_Temp.Seed = _InitParams.Seed;
		if (iter.HasMember("SpeakerId") && iter["SpeakerId"].IsInt())
			_Temp.SpeakerId = iter["SpeakerId"].GetInt();
		else
			_Temp.SpeakerId = _InitParams.SpeakerId;

		if (_Temp.MaxDecodeStep < 500) _Temp.MaxDecodeStep = 500;
		if (_Temp.GateThreshold > 0.98f) _Temp.GateThreshold = 0.98f;
		if (_Temp.GateThreshold < 0.2f) _Temp.GateThreshold = 0.2f;
		if (_Temp.FactorDpSdp > 1.f) _Temp.FactorDpSdp = 1.f;
		if (_Temp.FactorDpSdp < 0.f) _Temp.FactorDpSdp = 0.f;
		if (_Temp.DurationPredictorNoiseScale > 10.f) _Temp.DurationPredictorNoiseScale = 10.f;
		if (_Temp.DurationPredictorNoiseScale < 0.f) _Temp.DurationPredictorNoiseScale = 0.f;
		if (_Temp.RestTime > 30.f) _Temp.RestTime = 30.f;
		if (_Temp.LengthScale > 10.f) _Temp.LengthScale = 10.f;
		if (_Temp.LengthScale < 0.1f) _Temp.LengthScale = 0.1f;

		_TTSInputSeqs.emplace_back(std::move(_Temp));
	}
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
	if (_Seq.find(L"[ph]") != 0 && _Seq[0] == L'[')
		return Inference(GetInputSeqs({ to_byte_string(_Seq), true }, _InferParams));

	std::vector<std::wstring> SeqLens;
	std::wstring TmpSeq;
	for (const auto chari : _Seq)
	{
		if ((chari == L'\n') || (chari == L'\r'))
		{
			if (!TmpSeq.empty())
			{
				SeqLens.push_back(TmpSeq);
				TmpSeq.clear();
			}
			continue;
		}
		TmpSeq += chari;
	}
	if (!TmpSeq.empty())
		SeqLens.push_back(TmpSeq);

	std::vector<MoeVSProjectSpace::MoeVSTTSSeq> InputSeqs;
	InputSeqs.reserve(SeqLens.size());
	for(const auto& SeqL : SeqLens)
	{
		MoeVSProjectSpace::MoeVSTTSSeq TmpSeqData;
		if (SeqL.find(L"[ph]") == 0)
			TmpSeqData.Seq = Cleaner->DictReplace(SeqL.substr(4), _InferParams.PlaceHolderSymbol);
		else
			TmpSeqData.Seq = Cleaner->DictReplace(Cleaner->G2p(SeqL, _InferParams.PlaceHolderSymbol, _InferParams.AdditionalInfo, _InferParams.Language), _InferParams.PlaceHolderSymbol);
		TmpSeqData.SpeakerMix = _InferParams.SpeakerMix;
		TmpSeqData.EmotionPrompt = _InferParams.EmotionPrompt;
		TmpSeqData.PlaceHolderSymbol = _InferParams.PlaceHolderSymbol;
		TmpSeqData.NoiseScale = _InferParams.NoiseScale;
		TmpSeqData.LengthScale = _InferParams.LengthScale;
		TmpSeqData.DurationPredictorNoiseScale = _InferParams.DurationPredictorNoiseScale;
		TmpSeqData.FactorDpSdp = _InferParams.FactorDpSdp;
		TmpSeqData.GateThreshold = _InferParams.GateThreshold;
		TmpSeqData.MaxDecodeStep = _InferParams.MaxDecodeStep;
		TmpSeqData.Seed = _InferParams.Seed;
		TmpSeqData.SpeakerId = _InferParams.SpeakerId;
		TmpSeqData.RestTime = _InferParams.RestTime;
		InputSeqs.emplace_back(std::move(TmpSeqData));
	}
	return Inference(InputSeqs);
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
	std::vector<std::vector<int16_t>> PCM;
	std::vector<std::wstring> AudioFolders;

	if (_Seq.empty())
		return {};
	if (_Seq.find(L"[ph]") != 0 && _Seq[0] == L'[')
		PCM = Inference(GetInputSeqs({ to_byte_string(_Seq), true }, _InferParams));
	else
	{
		std::vector<std::wstring> SeqLens;
		std::wstring TmpSeq;
		for (const auto chari : _Seq)
		{
			if ((chari == L'\n') || (chari == L'\r'))
			{
				if (!TmpSeq.empty())
				{
					SeqLens.push_back(TmpSeq);
					TmpSeq.clear();
				}
				continue;
			}
			TmpSeq += chari;
		}
		if (!TmpSeq.empty())
			SeqLens.push_back(TmpSeq);

		std::vector<MoeVSProjectSpace::MoeVSTTSSeq> InputSeqs;
		InputSeqs.reserve(SeqLens.size());
		for (const auto& SeqL : SeqLens)
		{
			MoeVSProjectSpace::MoeVSTTSSeq TmpSeqData;
			if (SeqL.find(L"[ph]") == 0)
				TmpSeqData.Seq = Cleaner->DictReplace(SeqL.substr(4), _InferParams.PlaceHolderSymbol);
			else
				TmpSeqData.Seq = Cleaner->DictReplace(Cleaner->G2p(SeqL, _InferParams.PlaceHolderSymbol, _InferParams.AdditionalInfo, _InferParams.Language), _InferParams.PlaceHolderSymbol);
			TmpSeqData.SpeakerMix = _InferParams.SpeakerMix;
			TmpSeqData.EmotionPrompt = _InferParams.EmotionPrompt;
			TmpSeqData.PlaceHolderSymbol = _InferParams.PlaceHolderSymbol;
			TmpSeqData.NoiseScale = _InferParams.NoiseScale;
			TmpSeqData.LengthScale = _InferParams.LengthScale;
			TmpSeqData.DurationPredictorNoiseScale = _InferParams.DurationPredictorNoiseScale;
			TmpSeqData.FactorDpSdp = _InferParams.FactorDpSdp;
			TmpSeqData.GateThreshold = _InferParams.GateThreshold;
			TmpSeqData.MaxDecodeStep = _InferParams.MaxDecodeStep;
			TmpSeqData.Seed = _InferParams.Seed;
			TmpSeqData.SpeakerId = _InferParams.SpeakerId;
			TmpSeqData.RestTime = _InferParams.RestTime;
			InputSeqs.emplace_back(std::move(TmpSeqData));
		}
		PCM = Inference(InputSeqs);
	}

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
	MoeVSNotImplementedError;
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
				throw std::exception("AligError");
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