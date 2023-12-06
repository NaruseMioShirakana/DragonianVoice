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

class EmoLoader
{
public:
	static constexpr long startPos = 128;
	EmoLoader() = default;
	EmoLoader(const std::wstring& path)
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
		_wfopen_s(&emofile, path.c_str(), L"r");
		if (!emofile)
			LibDLVoiceCodecThrow("emoFile not exists")
	}
	~EmoLoader()
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
	}
	void close()
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
	}
	void open(const std::wstring& path)
	{
		if (emofile)
			fclose(emofile);
		emofile = nullptr;
		_wfopen_s(&emofile, path.c_str(), L"rb");
		if (!emofile)
			LibDLVoiceCodecThrow("emoFile not exists")
	}
	std::vector<float> operator[](long index) const
	{
		if (emofile)
		{
			fseek(emofile, index * 4096 + startPos, SEEK_SET);
			char buffer[4096];
			const auto buf = reinterpret_cast<float*>(buffer);
			const auto bufread = fread_s(buffer, 4096, 1, 4096, emofile);
			if (bufread == 4096)
				return { buf ,buf + 1024 };
			LibDLVoiceCodecThrow("emo index out of range")
		}
		LibDLVoiceCodecThrow("emo file not opened")
	}
private:
	FILE* emofile = nullptr;
};

class TextToSpeech : public MoeVoiceStudioModule
{
public:
	using DurationCallback = std::function<void(std::vector<float>&)>;

	TextToSpeech(const ExecutionProviders& ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_ = 0);

	[[nodiscard]] std::vector<MoeVSProjectSpace::MoeVSTTSSeq> GetInputSeqs(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams) const;

	std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& SpecializeInputSeqs(std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Seq) const;

	[[nodiscard]] static std::vector<MoeVSProjectSpace::MoeVSTTSSeq> GetInputSeqsStatic(const MJson& _Input, const MoeVSProjectSpace::MoeVSParams& _InitParams);

	static std::vector<std::vector<bool>> generatePath(float* duration, size_t durationSize, size_t maskSize);

	[[nodiscard]] std::vector<float> GetEmotionVector(const std::vector<std::wstring>& src) const;

	[[nodiscard]] std::vector<std::vector<int16_t>> Inference(const std::wstring& _Seq,
		const MoeVSProjectSpace::MoeVSParams& _InferParams = MoeVSProjectSpace::MoeVSParams()) const;

	[[nodiscard]] std::vector<std::wstring> Inference(const std::wstring& _Seq,
		const MoeVSProjectSpace::MoeVSParams& _InferParams, bool T) const;

	[[nodiscard]] std::vector<std::vector<int16_t>> Inference(const MJson& _Inputs,
		const MoeVSProjectSpace::MoeVSParams& _InferParams = MoeVSProjectSpace::MoeVSParams()) const;

	[[nodiscard]] virtual std::vector<std::vector<int16_t>> Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const;

	[[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Datas, const MoeVSProjectSpace::MoeVSParams& _InferParams, const InferTools::SlicerSettings& _SlicerSettings) const override;

	[[nodiscard]] static std::vector<size_t> GetAligments(size_t DstLen, size_t SrcLen);

	[[nodiscard]] std::wstring TextNormalize(const std::wstring& _Input, int64_t LanguageId) const;

	[[nodiscard]] int64_t GetLanguageToneIdx(int64_t _Index) const
	{
		std::string LanguageSymb;
		for(const auto& i : LanguageMap)
			if (_Index == i.second)
				LanguageSymb = i.first;
		if (LanguageSymb.empty())
			return 0;
		const auto Iter = LanguageTones.find(LanguageSymb);
		if (Iter != LanguageTones.end())
			return Iter->second;
		return 0;
	}

	[[nodiscard]] std::vector<size_t> AligPhoneAttn(const std::string& LanguageStr, const std::vector<std::wstring>& PhoneSeq, size_t BertSize) const;

	static int64_t find_max_idx(const std::vector<float>& inp)
	{
		int64_t idx = 0;
		for (size_t i = 1; i < inp.size(); ++i)
			if (inp[i] > inp[idx])
				idx = int64_t(i);
		return idx;
	}

	~TextToSpeech() override = default;

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

	MoeVSG2P::Tokenizer& GetTokenizer(const std::string& LanguageId)
	{
		return Tokenizers[LanguageMap.at(LanguageId)];
	}
protected:
	DurationCallback CustomDurationCallback;
	int64_t SpeakerCount = 1;
	std::unordered_map<std::wstring, int64_t> SpeakerMap;
	std::map<std::string, int64_t> LanguageMap = { {"ZH", 0}, {"JP", 1}, {"EN", 2} };
	std::map<std::string, int64_t> LanguageTones = { {"ZH", 0}, {"JP", 0}, {"EN", 0} };
	std::vector<MoeVSG2P::Tokenizer> Tokenizers;
	MoeVSG2P::MVSCleaner* Cleaner = nullptr;
	bool AddBlank = true;
	bool Emotion = false;
	std::map<std::wstring, int64_t> Symbols;
	EmoLoader EmoLoader;
	MJson EmoJson;
};

MoeVoiceStudioCoreEnd