#include "../header/MoeVSProject.hpp"
#include <regex>
#include "../../InferTools/inferTools.hpp"

namespace MoeVSProjectSpace
{
	std::wregex _REP_REG_1(L"\\\\"), _REP_REG_2(L"\\\""), _REP_REG_3(L"\r"),
		_REP_REG_4(L"\n"), _REP_REG_5(L"\t"), _REP_REG_6(L"\b"), _REP_REG_7(L"\f");

	std::wstring ReplaceSpecialTokens(const std::wstring& input)
	{
		auto output = input;
		output = std::regex_replace(output, _REP_REG_1, L"\\\\");
		output = std::regex_replace(output, _REP_REG_2, L"\\\"");
		output = std::regex_replace(output, _REP_REG_3, L"\\r");
		output = std::regex_replace(output, _REP_REG_4, L"\\b");
		output = std::regex_replace(output, _REP_REG_5, L"\\t");
		output = std::regex_replace(output, _REP_REG_6, L"\\b");
		output = std::regex_replace(output, _REP_REG_7, L"\\f");
		return output;
	}

	std::vector<std::wstring> ReplaceSpecialTokens(const std::vector<std::wstring>& input)
	{
		std::vector<std::wstring> output;
		output.reserve(input.size());
		for (const auto& i : input)
			output.emplace_back(ReplaceSpecialTokens(i));
		return output;
	}

	std::vector<std::string> ReplaceSpecialTokens(const std::vector<std::string>& input)
	{
		std::vector<std::string> output;
		output.reserve(input.size());
		for (const auto& i : input)
			output.emplace_back(to_byte_string(ReplaceSpecialTokens(to_wide_string(i))));
		return output;
	}

	std::wstring MoeVSTTSToken::Serialization() const
	{
		if (Text.empty())
			return L"\t\t\t{ }";
		std::wstring rtn = L"\t\t\t{\n";
		rtn += L"\t\t\t\t\"Text\": \"" + ReplaceSpecialTokens(Text) + L"\",\n";
		rtn += L"\t\t\t\t\"Phonemes\": " + wstring_vector_to_string(ReplaceSpecialTokens(Phonemes)) + L",\n";
		rtn += L"\t\t\t\t\"Tones\": " + vector_to_string(Tones) + L",\n";
		rtn += L"\t\t\t\t\"Durations\": " + vector_to_string(Durations) + L",\n";
		rtn += L"\t\t\t\t\"Language\": " + string_vector_to_string(ReplaceSpecialTokens(Language)) + L"\n\t\t\t}";
		return rtn;
	}

	MoeVSTTSToken::MoeVSTTSToken(const MJsonValue& _JsonDocument, const MoeVSParams& _InitParams)
	{
		if (_JsonDocument.HasMember("Text") && _JsonDocument["Text"].IsString() && !_JsonDocument["Text"].Empty())
			Text = to_wide_string(_JsonDocument["Text"].GetString());
		else
			LibDLVoiceCodecThrow("Field \"Text\" Should Not Be Empty")

		if (_JsonDocument.HasMember("Phonemes") && _JsonDocument["Phonemes"].IsArray())
			for (const auto& j : _JsonDocument["Phonemes"].GetArray())
				Phonemes.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : L"[UNK]");

		if (_JsonDocument.HasMember("Tones") && _JsonDocument["Tones"].IsArray())
			for (const auto& j : _JsonDocument["Tones"].GetArray())
				Tones.emplace_back(j.IsInt() ? j.GetInt() : 0);

		if (_JsonDocument.HasMember("Durations") && _JsonDocument["Durations"].IsArray())
			for (const auto& j : _JsonDocument["Durations"].GetArray())
				Durations.emplace_back(j.IsInt() ? j.GetInt() : 0);

		if (_JsonDocument.HasMember("Language") && _JsonDocument["Language"].IsArray())
		{
			const auto LanguageArr = _JsonDocument["Language"].GetArray();
			if (!LanguageArr.empty() && LanguageArr[0].IsString())
				for (const auto& j : LanguageArr)
					Language.emplace_back(j.GetString());
		}
	}

	std::wstring MoeVSTTSSeq::Serialization() const
	{
		if (TextSeq.empty())
			return L"";
		std::wstring rtn = L"\t{\n";
		rtn += L"\t\t\"TextSeq\": \"" + ReplaceSpecialTokens(TextSeq) + L"\",\n";
		rtn += L"\t\t\"SlicedTokens\": [\n";
		for(const auto& iter : SlicedTokens)
			rtn += iter.Serialization() + L",\n";
		if (!SlicedTokens.empty())
			rtn.erase(rtn.end() - 2);
		rtn += L"\t\t],\n";
		rtn += L"\t\t\"SpeakerMix\": " + vector_to_string(SpeakerMix) + L",\n";
		rtn += L"\t\t\"EmotionPrompt\": " + wstring_vector_to_string(ReplaceSpecialTokens(EmotionPrompt)) + L",\n";
		rtn += L"\t\t\"NoiseScale\": " + std::to_wstring(NoiseScale) + L",\n";
		rtn += L"\t\t\"LengthScale\": " + std::to_wstring(LengthScale) + L",\n";
		rtn += L"\t\t\"DurationPredictorNoiseScale\": " + std::to_wstring(DurationPredictorNoiseScale) + L",\n";
		rtn += L"\t\t\"FactorDpSdp\": " + std::to_wstring(FactorDpSdp) + L",\n";
		rtn += L"\t\t\"GateThreshold\": " + std::to_wstring(GateThreshold) + L",\n";
		rtn += L"\t\t\"MaxDecodeStep\": " + std::to_wstring(MaxDecodeStep) + L",\n";
		rtn += L"\t\t\"Seed\": " + std::to_wstring(Seed) + L",\n";
		rtn += L"\t\t\"SpeakerName\": \"" + ReplaceSpecialTokens(SpeakerName) + L"\",\n";
		rtn += L"\t\t\"RestTime\": " + std::to_wstring(RestTime) + L",\n";
		rtn += L"\t\t\"PlaceHolderSymbol\": \"" + ReplaceSpecialTokens(PlaceHolderSymbol) + L"\",\n";
		rtn += L"\t\t\"LanguageSymbol\": \"" + ReplaceSpecialTokens(to_wide_string(LanguageSymbol)) + L"\",\n";
		rtn += L"\t\t\"G2PAdditionalInfo\": \"" + ReplaceSpecialTokens(AdditionalInfo) + L"\"\n\t}";
		return rtn;
	}

	MoeVSTTSSeq::MoeVSTTSSeq(const MJsonValue& _JsonDocument, const MoeVSParams& _InitParams)
	{
		if (_JsonDocument.HasMember("LanguageSymbol") && _JsonDocument["LanguageSymbol"].IsString() && !_JsonDocument["LanguageSymbol"].Empty())
			LanguageSymbol = _JsonDocument["LanguageSymbol"].GetString();
		else
			LanguageSymbol = _InitParams.LanguageSymbol;

		if (_JsonDocument.HasMember("G2PAdditionalInfo") && _JsonDocument["G2PAdditionalInfo"].IsString() && !_JsonDocument["G2PAdditionalInfo"].Empty())
			AdditionalInfo = to_wide_string(_JsonDocument["G2PAdditionalInfo"].GetString());
		else
			AdditionalInfo = _InitParams.AdditionalInfo;

		if (_JsonDocument.HasMember("PlaceHolderSymbol") && _JsonDocument["PlaceHolderSymbol"].IsString() && !_JsonDocument["PlaceHolderSymbol"].Empty())
			PlaceHolderSymbol = to_wide_string(_JsonDocument["PlaceHolderSymbol"].GetString());
		else
			PlaceHolderSymbol = _InitParams.PlaceHolderSymbol;

		if (_JsonDocument.HasMember("TextSeq") && _JsonDocument["TextSeq"].IsString() && !_JsonDocument["TextSeq"].Empty())
			TextSeq = to_wide_string(_JsonDocument["TextSeq"].GetString());
		else
			LibDLVoiceCodecThrow("Field \"TextSeq\" Should Not Be Empty")

		if (_JsonDocument.HasMember("SlicedTokens") && _JsonDocument["SlicedTokens"].IsArray())
		{
			const auto TokensArrayObject = _JsonDocument["SlicedTokens"].GetArray();
			for (const auto& iter : TokensArrayObject)
				SlicedTokens.emplace_back(iter, _InitParams);
		}

		if (_JsonDocument.HasMember("SpeakerMix") && _JsonDocument["SpeakerMix"].IsArray())
			for (const auto& j : _JsonDocument["SpeakerMix"].GetArray())
				SpeakerMix.emplace_back(j.IsFloat() ? j.GetFloat() : 0.f);
		else
			SpeakerMix = _InitParams.SpeakerMix;
		if (_JsonDocument.HasMember("EmotionPrompt") && _JsonDocument["EmotionPrompt"].IsArray())
			for (const auto& j : _JsonDocument["EmotionPrompt"].GetArray())
				EmotionPrompt.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : std::wstring());
		else
			EmotionPrompt = _InitParams.EmotionPrompt;
		if (_JsonDocument.HasMember("NoiseScale") && _JsonDocument["NoiseScale"].IsFloat())
			NoiseScale = _JsonDocument["NoiseScale"].GetFloat();
		else
			NoiseScale = _InitParams.NoiseScale;
		if (_JsonDocument.HasMember("LengthScale") && _JsonDocument["LengthScale"].IsFloat())
			LengthScale = _JsonDocument["LengthScale"].GetFloat();
		else
			LengthScale = _InitParams.LengthScale;
		if (_JsonDocument.HasMember("RestTime") && _JsonDocument["RestTime"].IsFloat())
			RestTime = _JsonDocument["RestTime"].GetFloat();
		else
			RestTime = _InitParams.RestTime;
		if (_JsonDocument.HasMember("DurationPredictorNoiseScale") && _JsonDocument["DurationPredictorNoiseScale"].IsFloat())
			DurationPredictorNoiseScale = _JsonDocument["DurationPredictorNoiseScale"].GetFloat();
		else
			DurationPredictorNoiseScale = _InitParams.DurationPredictorNoiseScale;
		if (_JsonDocument.HasMember("FactorDpSdp") && _JsonDocument["FactorDpSdp"].IsFloat())
			FactorDpSdp = _JsonDocument["FactorDpSdp"].GetFloat();
		else
			FactorDpSdp = _InitParams.FactorDpSdp;
		if (_JsonDocument.HasMember("GateThreshold") && _JsonDocument["GateThreshold"].IsFloat())
			GateThreshold = _JsonDocument["GateThreshold"].GetFloat();
		else
			GateThreshold = _InitParams.GateThreshold;
		if (_JsonDocument.HasMember("MaxDecodeStep") && _JsonDocument["MaxDecodeStep"].IsFloat())
			MaxDecodeStep = _JsonDocument["MaxDecodeStep"].GetInt();
		else
			MaxDecodeStep = _InitParams.MaxDecodeStep;
		if (_JsonDocument.HasMember("Seed") && _JsonDocument["Seed"].IsInt())
			Seed = _JsonDocument["Seed"].GetInt();
		else
			Seed = _InitParams.Seed;
		if (_JsonDocument.HasMember("SpeakerName") && _JsonDocument["SpeakerName"].IsString())
			SpeakerName = to_wide_string(_JsonDocument["SpeakerName"].GetString());
		else
			SpeakerName = _InitParams.SpeakerName;

		if (MaxDecodeStep < 2000) MaxDecodeStep = 2000;
		if (MaxDecodeStep > 20000) MaxDecodeStep = 20000;
		if (GateThreshold > 0.90f) GateThreshold = 0.90f;
		if (GateThreshold < 0.2f) GateThreshold = 0.2f;
		if (FactorDpSdp > 1.f) FactorDpSdp = 1.f;
		if (FactorDpSdp < 0.f) FactorDpSdp = 0.f;
		if (NoiseScale > 10.f) NoiseScale = 10.f;
		if (NoiseScale < 0.f) NoiseScale = 0.f;
		if (DurationPredictorNoiseScale > 10.f) DurationPredictorNoiseScale = 10.f;
		if (DurationPredictorNoiseScale < 0.f) DurationPredictorNoiseScale = 0.f;
		if (RestTime > 30.f) RestTime = 30.f;
		if (LengthScale > 10.f) LengthScale = 10.f;
		if (LengthScale < 0.1f) LengthScale = 0.1f;
	}

	bool MoeVSTTSSeq::operator==(const MoeVSTTSSeq& right) const
	{
		return Serialization() == right.Serialization();
	}
}
