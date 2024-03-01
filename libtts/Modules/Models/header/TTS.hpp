/**
 * FileName: TTS.hpp
 * Note: MoeVoiceStudioCore TTS基类
 *
 * Copyright (C) 2022-2023 NaruseMioShirakana (shirakanamio@foxmail.com)
 *
 * This file is part of MoeVoiceStudioCore library.
 * MoeVoiceStudioCore library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License as published by the Free Software Foundation, either version 3
 * of the License, or any later version.
 *
 * MoeVoiceStudioCore library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
 *
 * date: 2023-11-9 Create
*/

#pragma once
#include <map>
#include "ModelBase.hpp"
#include "../../Logger/MoeSSLogger.hpp"
#include "../../InferTools/G2P/MoeVSG2P.hpp"
#include "MJson.h"

MoeVoiceStudioCoreHeader

class TextToSpeech : public MoeVoiceStudioModule
{
public:
	using DurationCallback = std::function<void(std::vector<float>&)>;

	TextToSpeech(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_ = 0);

	/**Preprocess**/

	[[nodiscard]] std::vector<MoeVSProjectSpace::MoeVSTTSSeq> GetInputSeqs(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams) const;

	[[nodiscard]] static std::vector<MoeVSProjectSpace::MoeVSTTSSeq> GetInputSeqsStatic(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams);

	[[nodiscard]] std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& SpecializeInputSeqs(std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Seq) const;

	/**Infer**/

	[[nodiscard]] std::vector<std::vector<int16_t>> Inference(const std::wstring& _Seq,
		const MoeVSProjectSpace::MoeVSParams& _InferParams = MoeVSProjectSpace::MoeVSParams()) const;

	[[nodiscard]] std::vector<std::wstring> Inference(const std::wstring& _Seq,
		const MoeVSProjectSpace::MoeVSParams& _InferParams, bool T) const;

	[[nodiscard]] std::vector<std::vector<int16_t>> Inference(const MJson& _Inputs,
		const MoeVSProjectSpace::MoeVSParams& _InferParams = MoeVSProjectSpace::MoeVSParams()) const;

	[[nodiscard]] virtual std::vector<std::vector<int16_t>> Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const;

	[[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Datas, const MoeVSProjectSpace::MoeVSParams& _InferParams, const InferTools::SlicerSettings& _SlicerSettings) const override;

	[[nodiscard]] std::vector<size_t> AligPhoneAttn(const std::string& LanguageStr, const std::vector<std::wstring>& PhoneSeq, size_t BertSize) const;

	[[nodiscard]] std::wstring TextNormalize(const std::wstring& _Input, int64_t LanguageId) const;

	/**MapCast**/

	[[nodiscard]] const std::wstring& GetTokenizerNameWithLanguageSymbol(const std::string& LanguageSymbol) const;

	[[nodiscard]] const MoeVSG2P::Tokenizer& GetTokenizerWithLanguageSymbol(const std::string& LanguageSymbol) const;

	[[nodiscard]] const MoeVSG2P::Tokenizer& GetTokenizer(const std::wstring& TokenizerName) const;

	[[nodiscard]] const std::string& GetLanguageSymbolWithLanguageId(int64_t LanguageId) const;

	[[nodiscard]] int64_t GetLanguageIdWithLanguageSymbol(const std::string& LanguageSymbol) const;

	[[nodiscard]] int64_t GetBertIdWithLanguageSymbol(const std::string& LanguageSymbol) const;

	[[nodiscard]] int64_t GetTonesBegin(const std::string& LanguageSymbol) const;

	[[nodiscard]] int64_t GetTonesBegin(int64_t LanguageId) const;

	[[nodiscard]] int64_t GetSpeakerIdWithSpeakerName(const std::wstring& SpeakerName) const;

	[[nodiscard]] int64_t GetBertIndexWithName(const std::wstring& BertName) const;

	[[nodiscard]] const std::wstring& GetBertNameWithIndex(int64_t Index) const;

	[[nodiscard]] const std::wstring& GetSpeakerNameWithSpeakerId(int64_t SpeakerID) const;

	[[nodiscard]] std::vector<std::wstring> CleanText(const std::wstring& SrcText, const std::wstring& PlaceholderSymbol,
		const std::wstring& ExtraInfo, const std::string& LanguageSymbol) const;

	[[nodiscard]] std::vector<int64_t> CleanedSeq2Indices(const std::vector<std::wstring>& Seq) const;

	~TextToSpeech() override = default;

protected:

	std::unordered_map<std::wstring, int64_t> SpeakerName2ID;
	std::unordered_map<int64_t, std::wstring> SpeakerID2Name;

	std::unordered_map<std::string, int64_t> LanguageSymbol2ID = { {"ZH", 0}, {"JP", 1}, {"EN", 2} };
	std::unordered_map<int64_t, std::string> LanguageID2Symbol = { {0, "ZH"}, {1, "JP"}, {2, "EN"} };
	std::unordered_map<std::string, int64_t> LanguageSymbol2TonesBegin = { {"ZH", 0}, {"JP", 0}, {"EN", 0} };

	std::unordered_map<std::wstring, int64_t> Symbols;

	std::unordered_map<std::wstring, MoeVSG2P::Tokenizer> Tokenizers;
	std::unordered_map<int64_t, std::wstring> Index2BertName;
	std::unordered_map<std::wstring, int64_t> BertName2Index;
	std::unordered_map<std::string, std::wstring> LanguageSymbol2TokenizerName;
	std::unordered_map<std::string, int64_t> LanguageSymbol2BertID;

	MoeVSG2P::MVSCleaner* Cleaner = nullptr;

	std::wstring NoneString = L"None";
	std::string NoneAString = "None";

	std::vector<std::string> EncoderBertInputNames;

public:

	template <typename T = float>
	void LinearCombination(std::vector<T>& _data, T Value = T(1.0)) const
	{
		_data.resize(SpeakerCount, 0.f);
		if (_data.empty())
		{
			_data = std::vector<T>(1, Value);
			return;
		}
		T Sum = T(0.0);
		for (const auto& i : _data)
			Sum += i;
		if (Sum < T(0.0001))
		{
			_data = std::vector<T>(_data.size(), T(0.0));
			_data[0] = Value;
			return;
		}
		Sum *= T(Value);
		for (auto& i : _data)
			i /= Sum;
	}
	[[nodiscard]] static std::vector<std::vector<bool>> generatePath(float* duration, size_t durationSize, size_t maskSize);
	[[nodiscard]] static std::vector<size_t> GetAligments(size_t DstLen, size_t SrcLen);
	[[nodiscard]] static std::tuple<std::vector<std::wstring>, std::vector<int64_t>> SplitTonesFromTokens(const std::vector<std::wstring>& _Src, const std::vector<int64_t>& _ToneRef, int64_t FirstToneIdx, const std::string& LanguageSymbol);
	[[nodiscard]] static std::vector<std::wstring> CleanText(const MoeVSG2P::MVSCleaner* TextCleaner, const std::wstring& SrcText, const std::wstring& PlaceholderSymbol,
		const std::wstring& ExtraInfo, const std::string& LanguageSymbol);

protected:
	bool AddBlank = true;
	int64_t SpeakerCount = 1;
	int64_t UNKID = 0;
	DurationCallback CustomDurationCallback;
};

MoeVoiceStudioCoreEnd