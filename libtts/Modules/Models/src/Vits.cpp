#include "../header/Vits.hpp"
#include "../../InferTools/inferTools.hpp"
#include <random>

MoeVoiceStudioCoreHeader
bool BertEnabled = false;

namespace BertSpace
{
	std::unordered_map<std::wstring, Ort::Session*> sessionBert;
	std::unordered_map<std::wstring, Ort::Session*> sessionClap;
}

Ort::Session* GetBertModel(const std::wstring& Name)
{
	const auto fd = BertSpace::sessionBert.find(Name);
	if (fd != BertSpace::sessionBert.end())
		return fd->second;
	return nullptr;
}

Ort::Session* GetClapModel(const std::wstring& Name)
{
	const auto fd = BertSpace::sessionClap.find(Name);
	if (fd != BertSpace::sessionClap.end())
		return fd->second;
	return nullptr;
}

void SetBertEnabled(bool cond)
{
	BertEnabled = cond;
}

void DestoryAllBerts()
{
	for (const auto& it : BertSpace::sessionBert)
		delete it.second;
	for (const auto& it : BertSpace::sessionClap)
		delete it.second;
	BertSpace::sessionBert.clear();
	BertSpace::sessionClap.clear();
}

Vits::~Vits()
{
	logger.log(L"[Info] unloading Vits Models");
	destory();
	logger.log(L"[Info] Vits Models unloaded");
}

Vits::Vits(const MJson& _Config, const ProgressCallback& _ProgressCallback,
	const DurationCallback& _DurationCallback,
	ExecutionProviders ExecutionProvider_,
	unsigned DeviceID_, unsigned ThreadCount_) :
	TextToSpeech(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	//Check Folder
	if (_Config["Folder"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"folder\" (Model Folder)")
	if (!_Config["Folder"].IsString())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Must Be String")
	const auto _folder = to_wide_string(_Config["Folder"].GetString());
	if (_folder.empty())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Can Not Be Empty")
	const std::wstring _path = GetCurrentFolder() + L"/Models/" + _folder + L"/" + _folder;

	std::map<std::string, std::wstring> _PathDict;

	if(_Config.HasMember("EmotionalPath") && _Config["EmotionalPath"].IsString())
	{
		const auto emoStringload = to_wide_string(_Config["EmotionalPath"].GetString());
		if(!emoStringload.empty())
		{
			_PathDict["EmotionalPath"] = GetCurrentFolder() + L"/emotion/" + emoStringload + L".npy";
			_PathDict["EmotionalDictPath"] = GetCurrentFolder() + L"/emotion/" + emoStringload + L".json";
		}
	}

	if(_Config.HasMember("Clap") && _Config["Clap"].IsString())
	{
		const auto ClapStringload = to_wide_string(_Config["Clap"].GetString());
		if (!ClapStringload.empty())
			_PathDict["Clap"] = GetCurrentFolder() + L"/clap/" + ClapStringload;
	}

	_PathDict["Decoder"] = _path + L"_dec.onnx";
	_PathDict["StochasticDurationPredictor"] = _path + L"_sdp.onnx";
	_PathDict["DurationPredictor"] = _path + L"_dp.onnx";
	_PathDict["Encoder"] = _path + L"_enc_p.onnx";
	_PathDict["FlowNet"] = _path + L"_flow.onnx";
	_PathDict["Embidding"] = _path + L"_emb.onnx";

	if (_Config.HasMember("Dict") && _Config["Dict"].IsString() && !_Config["Dict"].Empty())
		_PathDict["Dict"] = GetCurrentFolder() + L"/Dict/" + to_wide_string(_Config["Dict"].GetString()) + L".json";

	std::vector<std::wstring> _BertPaths;
	if (_Config.HasMember("BertPath") && _Config["BertPath"].IsArray() && !_Config["BertPath"].Empty())
	{
		for(const auto& BPH : _Config["BertPath"].GetArray())
		{
			const auto BertPath = to_wide_string(BPH.GetString());
			if(!BertPath.empty())
				_BertPaths.emplace_back(GetCurrentFolder() + L"/Bert/" + BertPath);
		}
	}

	load(_PathDict, _Config, _ProgressCallback, _DurationCallback, _BertPaths);
}

void Vits::load(const std::map<std::string, std::wstring>& _PathDict,
	const MJson& _Config, const ProgressCallback& _ProgressCallback,
	const DurationCallback& _DurationCallback, const std::vector<std::wstring>& _BertPaths)
{
	if (_Config["Type"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Type\" (ModelType)");
	if (!_Config["Type"].IsString())
		LibDLVoiceCodecThrow("[Error] Field \"Type\" (ModelType) Must Be String");

	VitsType = _Config["Type"].GetString();
	if (VitsType == "Pits")
	{
		UseTone = true;
		UseLength = false;
	}
	else if (VitsType == "BertVits")
	{
		UseLength = false;
		UseTone = true;
		UseBert = true;
		UseLanguage = true;
		EncoderG = true;
	}

	//Load Cleaner
	Cleaner = MoeVSG2P::GetDefCleaner();
	if (_PathDict.find("Dict") != _PathDict.end() && (_waccess(_PathDict.at("Dict").c_str(), 0) != -1))
		Cleaner->loadDict(_PathDict.at("Dict"));
	else
		Cleaner->loadDict(L"");

	//Load Speaker Map
	if (_Config.HasMember("Characters"))
	{
		const auto Characters = _Config["Characters"];
		if(Characters.IsArray())
		{
			SpeakerCount = (int64_t)Characters.Size();
			int64_t SpkIdx = 0;
			for (const auto& iterator : Characters.GetArray())
			{
				const auto SpeakerName = to_wide_string(iterator.GetString());
				SpeakerName2ID[SpeakerName] = SpkIdx;
				SpeakerID2Name[SpkIdx] = SpeakerName;
				++SpkIdx;
			}
		}
		else if(Characters.GetMemberCount())
		{
			const auto Members = Characters.GetMemberArray();
			for (const auto& pair : Members)
			{
				const auto SpeakerName = to_wide_string(pair.first);
				const int64_t SpkIdx = pair.second.GetInt64();
				SpeakerName2ID[SpeakerName] = SpkIdx;
				SpeakerID2Name[SpkIdx] = SpeakerName;
			}
		}
	}

	//Load Language Map
	if (UseLanguage && (_Config["LanguageMap"].IsNull() || !_Config.HasMember("LanguageMap")))
		LibDLVoiceCodecThrow("[Error] Missing field \"LanguageMap\" (LanguageMap)");
	if (_Config.HasMember("LanguageMap"))
	{
		const auto LanguageMap = _Config["LanguageMap"];
		if (LanguageMap.GetMemberCount())
			for (const auto& Item : LanguageMap.GetMemberArray())
			{
				if (!Item.second.IsArray())
					continue;
				const auto LangArr = Item.second.GetArray();
				if (LangArr.size() < 2)
					continue;
				LanguageSymbol2ID[Item.first] = LangArr[0].GetInt();
				LanguageID2Symbol[LangArr[0].GetInt()] = Item.first;
				LanguageSymbol2TonesBegin[Item.first] = LangArr[1].GetInt();
			}
		else
			logger.log("[Warn] Field \"LanguageMap\" Is Empty, Use Default Value");
	}
	else
		logger.log("[Warn] Field \"LanguageMap\" Is Missing, Use Default Value");

	//Load Default Bert Dim
	if (_Config.HasMember("BertSize") && _Config["BertSize"].IsInt64())
		DefBertSize = _Config["BertSize"].GetInt64();
	else
		logger.log(L"[Warn] Field \"BertSize\" Is Missing, Use Default Value");

	//Enable Bert Reference
	if (_Config.HasMember("ReferenceBert") && _Config["ReferenceBert"].IsBool())
		ReferenceBert = _Config["ReferenceBert"].GetBool();
	else
		logger.log(L"[Warn] Field \"ReferenceBert\" Is Missing, Use Default Value");

	//Load SamplingRate
	if (_Config["Rate"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Rate\" (SamplingRate)");
	if (_Config["Rate"].IsInt() || _Config["Rate"].IsInt64())
		_samplingRate = _Config["Rate"].GetInt();
	else
		LibDLVoiceCodecThrow("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64");

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	//Check Symbol & AddBlank
	if (!_Config.HasMember("Symbol") || _Config["Symbol"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Symbol\" (PhSymbol)");
	if (_Config.HasMember("AddBlank") && !_Config["AddBlank"].IsNull())
		AddBlank = _Config["AddBlank"].GetBool();
	else
		logger.log(L"[Warn] Field \"AddBlank\" Is Missing, Use Default Value");

	//Load Symbols
	int64_t iter_symb = 0;
	if (_Config["Symbol"].IsArray())
	{
		logger.log(L"[Info] Use Phs");
		if (_Config["Symbol"].Empty())
			LibDLVoiceCodecThrow("[Error] Field \"Symbol\" (PhSymbol) Can Not Be Empty");
		const auto SymbolArr = _Config["Symbol"].GetArray();
		if (!SymbolArr[0].IsString())
			LibDLVoiceCodecThrow("[Error] Field \"Symbol\" (PhSymbol) Must Be Array<String> or String");
		for (const auto& it : SymbolArr)
			Symbols.insert({ to_wide_string(it.GetString()), iter_symb++ });
	}
	else
	{
		if (!_Config["Symbol"].IsString())
			LibDLVoiceCodecThrow("[Error] Field \"Symbol\" (PhSymbol) Must Be Array<String> or String");
		logger.log(L"[Info] Use Symbols");
		const std::wstring SymbolsStr = to_wide_string(_Config["Symbol"].GetString());
		if (SymbolsStr.empty())
			LibDLVoiceCodecThrow("[Error] Field \"Symbol\" (PhSymbol) Can Not Be Empty");
		for (size_t i = 0; i < SymbolsStr.length(); ++i)
			Symbols.insert({ SymbolsStr.substr(i,1) , iter_symb++ });
	}

	for (const auto& iter : Symbols)
		if (iter.first.find(L"UNK") != std::wstring::npos || iter.first.find(L"unk") != std::wstring::npos)
			UNKID = iter.second;

	if (_Config.HasMember("VQSize") && _Config["VQSize"].IsInt64())
		VQCodeBookSize = _Config["VQSize"].GetInt64();
	else
		logger.log(L"[Warn] Field \"VQSize\" Is Missing, Use Default Value");

	//Load Emo-Vector
	try
	{
		if (_PathDict.find("EmotionalPath") != _PathDict.end())
		{
			const auto EmotionPath = _PathDict.at("EmotionalPath");
			if (!EmotionPath.empty())
			{
				logger.log(L"[Info] Loading EmotionVector");
				EmoLoader.open(EmotionPath);
				logger.log(L"[Info] EmotionVector Loaded");
				Emotion = true;
			}
		}
		if (_PathDict.find("EmotionalDictPath") != _PathDict.end())
		{
			const auto EmotionPath = _PathDict.at("EmotionalDictPath");
			if (!EmotionPath.empty())
			{
				MJson EmoJson = { EmotionPath };
				for (const auto& Member : EmoJson.GetMemberArray())
					Emo2Id[to_wide_string(Member.first)] = Member.second.GetInt();
			}
		}
	}
	catch (std::exception& e)
	{
		logger.log((std::string("[Warn] EmotionPath Error ") + e.what()).c_str());
	}

	//InputNames
	if (UseLength)
		EncoderInputNames.emplace_back("x_lengths");
	if (UseTone)
		EncoderInputNames.emplace_back("t");
	if (Emotion)
		EncoderInputNames.emplace_back("emotion");
	if (UseLanguage)
		EncoderInputNames.emplace_back("language");

	//Load Bert
	if (UseBert)
	{
		size_t BertInputCount = _BertPaths.size();

		if (_Config.HasMember("BertInputs") && _Config["BertInputs"].IsArray() && _Config["BertInputs"].Size() != BertInputCount)
		{
			BertInputCount = _Config["BertInputs"].Size();
			int64_t Index = 0;
			for (const auto& Member : _Config["BertInputs"].GetArray())
			{
				const auto BertName = to_wide_string(Member.GetString());
				Index2BertName[Index] = BertName;
				BertName2Index[BertName] = Index;
				++Index;
			}
		}
		else
		{
			int64_t Index = 0;
			for (const auto& Path : _BertPaths)
			{
				const auto BertName = Path.substr(Path.rfind(L'/') + 1);
				Index2BertName[Index] = BertName;
				BertName2Index[BertName] = Index;
				++Index;
			}
		}

		if (_Config.HasMember("Lang2BertId") && _Config["Lang2BertId"].GetMemberCount() == LanguageSymbol2ID.size())
		{
			for (const auto& Member : _Config["Lang2BertId"].GetMemberArray())
			{
				LanguageSymbol2TokenizerName[Member.first] = GetBertNameWithIndex(Member.second.GetInt64());
				LanguageSymbol2BertID[Member.first] = Member.second.GetInt64();
			}
		}
		else if(LanguageSymbol2ID.size() <= BertInputCount)
		{
			for(const auto& Pair : LanguageSymbol2ID)
			{
				LanguageSymbol2TokenizerName[Pair.first] = GetBertNameWithIndex(Pair.second);
				LanguageSymbol2BertID[Pair.first] = Pair.second;
			}
		}
		else if(BertInputCount == 1)
		{
			for (const auto& Pair : LanguageSymbol2ID)
			{
				LanguageSymbol2TokenizerName[Pair.first] = GetBertNameWithIndex(0);
				LanguageSymbol2BertID[Pair.first] = 0;
			}
		}
		else
			LibDLVoiceCodecThrow("You Must Have Field \"Lang2BertId\"")

		EncoderBertInputNames.reserve(BertInputCount * 2);
		for (size_t i = 0; i < BertInputCount; ++i)
			EncoderBertInputNames.emplace_back("bert_" + std::to_string(i));
		for (const auto& NameInp : EncoderBertInputNames)
			EncoderInputNames.emplace_back(NameInp.data());

		for(const auto& Path : _BertPaths)
		{
			const auto BertName = Path.substr(Path.rfind(L'/') + 1);
			Ort::Session* SessionBert = nullptr;
			if (BertSpace::sessionBert.find(BertName) != BertSpace::sessionBert.end() && BertSpace::sessionBert.at(BertName) != nullptr)
				SessionBert = BertSpace::sessionBert.at(BertName);
			if (_waccess(Path.c_str(), 0) != -1)
			{
				if(!SessionBert)
				{
					try
					{
						SessionBert = new Ort::Session(*env, (Path + L"/model.onnx").c_str(), *session_options);
					}
					catch(Ort::Exception& e)
					{
						logger.log(L"[Warn] " + to_wide_string(e.what()));
						delete SessionBert;
						SessionBert = nullptr;
					}
				}
				if (_waccess((Path + L"/Tokenizer.json").c_str(), 0) != -1)
				{
					Tokenizers.insert({ BertName ,Path + L"/Tokenizer.json" });
					Tokenizers[BertName].BondCleaner(Cleaner);
				}
				else if (SessionBert)
				{
					delete SessionBert;
					LibDLVoiceCodecThrow("Bert Must Have a Tokenizer");
				}
			}
			BertSpace::sessionBert[BertName] = SessionBert;
		}
		for (auto& iter : BertSpace::sessionBert)
			if (BertName2Index.find(iter.first) == BertName2Index.end())
			{
				delete iter.second;
				iter.second = nullptr;
			}
	}

	if(_BertPaths.empty())
	{
		for (const auto& iter : BertSpace::sessionBert)
			delete iter.second;
		BertSpace::sessionBert.clear();
	}

	if (VitsType == "BertVits" && _PathDict.find("Clap") != _PathDict.end())
	{
		EncoderInputNames.emplace_back("emo");
		UseClap = true;
		const auto& Path = _PathDict.at("Clap");
		ClapName = Path.substr(Path.rfind(L'/') + 1);
		Ort::Session* SessionClap = nullptr;
		if ((BertSpace::sessionClap.find(ClapName) != BertSpace::sessionClap.end() && BertSpace::sessionClap.at(ClapName) != nullptr))
			SessionClap = BertSpace::sessionClap.at(ClapName);
		if (!SessionClap)
		{
			try
			{
				SessionClap = new Ort::Session(*env, (Path + L".onnx").c_str(), *session_options);
			}
			catch (Ort::Exception& e)
			{
				logger.log(L"[Warn] " + to_wide_string(e.what()));
				delete SessionClap;
				SessionClap = nullptr;
			}
		}
		if (_waccess((Path + L".json").c_str(), 0) != -1)
		{
			Tokenizers.insert({ ClapName , Path + L".json" });
			Tokenizers[ClapName].BondCleaner(Cleaner);
		}
		else if (SessionClap)
		{
			delete SessionClap;
			LibDLVoiceCodecThrow("Clap Must Have a Tokenizer");
		}
		BertSpace::sessionClap[ClapName] = SessionClap;
		for (auto& iter : BertSpace::sessionClap)
			if (ClapName != iter.first)
			{
				delete iter.second;
				iter.second = nullptr;
			}
	}
	else
	{
		for (auto& iter : BertSpace::sessionClap)
			delete iter.second;
		BertSpace::sessionClap.clear();
		UseClap = false;
	}

	_callback = _ProgressCallback;
	CustomDurationCallback = _DurationCallback;

	//LoadModels
	try
	{
		logger.log(L"[Info] loading Vits Models");
		sessionDec = new Ort::Session(*env, _PathDict.at("Decoder").c_str(), *session_options);
		sessionEnc_p = new Ort::Session(*env, _PathDict.at("Encoder").c_str(), *session_options);
		sessionFlow = new Ort::Session(*env, _PathDict.at("FlowNet").c_str(), *session_options);

		if (_waccess(_PathDict.at("Embidding").c_str(), 0) != -1)
			sessionEmb = new Ort::Session(*env, _PathDict.at("Embidding").c_str(), *session_options);
		else
			sessionEmb = nullptr;

		if (_waccess(_PathDict.at("DurationPredictor").c_str(), 0) != -1)
			sessionDp = new Ort::Session(*env, _PathDict.at("DurationPredictor").c_str(), *session_options);
		else
			sessionDp = nullptr;

		if (_waccess(_PathDict.at("StochasticDurationPredictor").c_str(), 0) != -1)
			sessionSdp = new Ort::Session(*env, _PathDict.at("StochasticDurationPredictor").c_str(), *session_options);
		else
			sessionSdp = nullptr;

		if (!sessionDp && !sessionSdp)
		{
			destory();
			LibDLVoiceCodecThrow("You must have a duration predictor");
		}

		logger.log(L"[Info] Vits Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		destory();
		LibDLVoiceCodecThrow(_exception.what());
	}

	if (sessionEmb)
	{
		if(EncoderG) EncoderInputNames.emplace_back("g");
		SdpInputNames.emplace_back("g");
		DpInputNames.emplace_back("g");
		FlowInputNames.emplace_back("g");
		DecInputNames.emplace_back("g");
	}

	if(VitsType == "BertVits" && sessionEnc_p->GetInputCount() == EncoderInputNames.size() + 2)
	{
		EncoderInputNames.emplace_back("vqidx");
		EncoderInputNames.emplace_back("sid");
		UseVQ = true;
	}
}

Vits::Vits(const std::map<std::string, std::wstring>& _PathDict, 
	const MJson& _Config, const ProgressCallback& _ProgressCallback,
	const DurationCallback& _DurationCallback, const std::vector<std::wstring>& _BertPaths,
	ExecutionProviders ExecutionProvider_,
	unsigned DeviceID_, unsigned ThreadCount_) :
	TextToSpeech(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	load(_PathDict, _Config, _ProgressCallback, _DurationCallback, _BertPaths);
}

std::vector<float> Vits::GetEmotionVector(const std::vector<std::wstring>& src) const
{
	if (src.empty())
		return EmoLoader[0];
	std::vector<float> dst(1024, 0.0);
	uint64_t mul = 0;
	for (const auto& iter : src)
	{
		long emoId;
		if (Emo2Id.find(iter) != Emo2Id.end())
			emoId = Emo2Id.at(iter);
		else
			emoId = _wtoi(iter.c_str());
		auto emoVec = EmoLoader[emoId];
		for (size_t i = 0; i < 1024; ++i)
			dst[i] = dst[i] + (emoVec[i] - dst[i]) / (float)(mul + 1ull);
		++mul;
	}
	return dst;
}

std::tuple<bool, std::vector<std::wstring>, std::vector<std::wstring>, std::vector<int64_t>, std::vector<int64_t>, std::vector<int64_t>, std::vector<size_t>, bool> Vits::PreProcessSeq(const MoeVSProjectSpace::MoeVSTTSSeq& Seq, int64_t CurLanguageIdx) const
{
	std::vector<std::wstring> PhonemeSeq, TokenSeq;
	std::vector<int64_t> ToneSeq;
	std::vector<int64_t> LanguageSeq;
	std::vector<int64_t> DurationSeq;
	std::vector<size_t> BertAligSeq;
	bool HasBertSeq = false;
	bool Skip = false;
	if (AddBlank)
		BertAligSeq.emplace_back(0);

	std::vector<MoeVSProjectSpace::MoeVSTTSToken> TmpToken;
	const std::vector<MoeVSProjectSpace::MoeVSTTSToken>* TokensPtr = &Seq.SlicedTokens;
	if (BertEnabled && UseBert && Seq.SlicedTokens.empty())
	{
		HasBertSeq = true;
		if (Seq.LanguageSymbol != "ZH")
			TokenSeq = GetTokenizerWithLanguageSymbol(Seq.LanguageSymbol).Tokenize(Seq.TextSeq);
		else
		{
			auto NewData = GetTokenizerWithLanguageSymbol(Seq.LanguageSymbol).Tokenize(Seq.TextSeq);
			if (NewData.empty())
			{
				Skip = true;
				return { Skip, std::move(PhonemeSeq), std::move(TokenSeq), std::move(ToneSeq), std::move(LanguageSeq), std::move(DurationSeq), std::move(BertAligSeq), HasBertSeq };
			}
			TmpToken.reserve(NewData.size());
			for (const auto& it : NewData)
			{
				MoeVSProjectSpace::MoeVSTTSToken Tok;
				Tok.Text = it;
				TmpToken.emplace_back(std::move(Tok));
			}
			TokensPtr = &TmpToken;
		}
	}

	if (UseBert && TokensPtr && !TokensPtr->empty())
	{
		size_t TokensIndex = 1;
		for (const auto& iter : *TokensPtr)
		{
			if (iter.Text.empty())
			{
				logger.log("[Info] Skip Empty Slice");
				continue;
			}
			TokenSeq.emplace_back(iter.Text);
			if (iter.Text == L"[CLS]")
				continue;
			if (iter.Text == L"[SEP]")
				break;
			std::vector<std::wstring> const* PhonemesPtr;
			std::vector<std::wstring> TempPhonemes;
			std::vector<int64_t> TempToneVec;
			if (iter.Phonemes.empty())
			{
				std::vector<std::wstring> TempSeqVec;
				auto TempText = iter.Text;
				if (TempText == L"[UNK]")
					TempPhonemes = { L"[UNK]" };
				else
				{
					if (TempText.find(L"##") == 0)
						TempText = TempText.substr(2);
					if (TempText.find(L'▁') == 0)
						TempText = TempText.substr(1);
					TempPhonemes = CleanText(TempText, Seq.PlaceHolderSymbol, L"/[WithTone]" + Seq.AdditionalInfo, Seq.LanguageSymbol);
				}
				const int64_t FirstToneIdx = GetTonesBegin(Seq.LanguageSymbol);
				auto RtnData = SplitTonesFromTokens(TempPhonemes, iter.Tones, FirstToneIdx, Seq.LanguageSymbol);
				TempPhonemes = std::move(std::get<0>(RtnData));
				TempToneVec = std::move(std::get<1>(RtnData));
				PhonemesPtr = &TempPhonemes;
			}
			else
				PhonemesPtr = &iter.Phonemes;

			PhonemeSeq.insert(PhonemeSeq.end(), PhonemesPtr->begin(), PhonemesPtr->end());
			if (PhonemesPtr->size() == iter.Language.size())
				for (const auto& LanguageSymbol : iter.Language)
					LanguageSeq.emplace_back(GetLanguageIdWithLanguageSymbol(LanguageSymbol));
			else
				LanguageSeq.insert(LanguageSeq.end(), PhonemesPtr->size(), CurLanguageIdx);
			if (PhonemesPtr->size() == iter.Durations.size())
				DurationSeq.insert(DurationSeq.end(), iter.Durations.begin(), iter.Durations.end());
			if (PhonemesPtr->size() == iter.Tones.size())
				ToneSeq.insert(ToneSeq.end(), iter.Tones.begin(), iter.Tones.end());
			else if (PhonemesPtr->size() == TempToneVec.size())
				ToneSeq.insert(ToneSeq.end(), TempToneVec.begin(), TempToneVec.end());
			else
				ToneSeq.insert(ToneSeq.end(), PhonemesPtr->size(), GetTonesBegin(Seq.LanguageSymbol));
			for (size_t idxl = 0; idxl < PhonemesPtr->size(); ++idxl)
			{
				BertAligSeq.emplace_back(TokensIndex);
				if (AddBlank)
					BertAligSeq.emplace_back(TokensIndex);
			}
			++TokensIndex;
		}
		if ((AddBlank && BertAligSeq.size() == PhonemeSeq.size() * 2 + 1) || (!AddBlank && BertAligSeq.size() == PhonemeSeq.size()))
			HasBertSeq = true;
		else
			HasBertSeq = false;
	}
	else if (!Seq.TextSeq.empty())
	{
		PhonemeSeq = CleanText(Seq.TextSeq, Seq.PlaceHolderSymbol, L"/[WithTone]" + Seq.AdditionalInfo, Seq.LanguageSymbol);
		LanguageSeq = std::vector(PhonemeSeq.size(), CurLanguageIdx);
		auto RtnVal = SplitTonesFromTokens(PhonemeSeq, {}, GetTonesBegin(Seq.LanguageSymbol), Seq.LanguageSymbol);
		PhonemeSeq = std::move(std::get<0>(RtnVal));
		ToneSeq = std::move(std::get<1>(RtnVal));
	}
	else
	{
		logger.log("[Info] Skip Empty TextSeq");
		Skip = true;
	}
	return { Skip, std::move(PhonemeSeq), std::move(TokenSeq), std::move(ToneSeq), std::move(LanguageSeq), std::move(DurationSeq), std::move(BertAligSeq), HasBertSeq };
}

std::vector<Ort::Value> Vits::GetBertFeature(size_t IndexOfBert, const std::vector<std::wstring>& TokenSeq, Ort::Session* CurBertSession) const
{
	auto input_ids = GetTokenizer(GetBertNameWithIndex((int64_t)IndexOfBert))(TokenSeq);
	std::vector<int64_t> attention_mask(input_ids.size(), 1), token_type_ids(input_ids.size(), 0);
	const int64_t AttentionShape[2] = { 1, (int64_t)input_ids.size() };
	std::vector<Ort::Value> AttentionInput, AttentionOutput;
	AttentionInput.emplace_back(Ort::Value::CreateTensor(
		*memory_info, input_ids.data(), input_ids.size(), AttentionShape, 2));
	if (CurBertSession->GetInputCount() == 3)
		AttentionInput.emplace_back(Ort::Value::CreateTensor(*memory_info, attention_mask.data(), attention_mask.size(), AttentionShape, 2));
	AttentionInput.emplace_back(Ort::Value::CreateTensor(
		*memory_info, token_type_ids.data(), token_type_ids.size(), AttentionShape, 2));
	try
	{
		AttentionOutput = CurBertSession->Run(Ort::RunOptions{ nullptr },
			BertInputNames.data(),
			AttentionInput.data(),
			CurBertSession->GetInputCount(),
			BertOutputNames.data(),
			1);
	}
	catch (Ort::Exception& e)
	{
		LibDLVoiceCodecThrow((std::string("Locate: Bert\n") + e.what()))
	}
	return AttentionOutput;
}

std::vector<int16_t> Vits::Infer(const MoeVSProjectSpace::MoeVSTTSSeq& Seq) const
{
	std::vector<int16_t> PCMDATA;
	auto CurLanguageIdx = GetLanguageIdWithLanguageSymbol(Seq.LanguageSymbol);

	auto PreprocessedSeq = PreProcessSeq(Seq, CurLanguageIdx);
	bool Skip = std::get<0>(PreprocessedSeq);
	auto PhonemeSeq = std::move(std::get<1>(PreprocessedSeq));
	auto TokenSeq = std::move(std::get<2>(PreprocessedSeq));
	std::vector<int64_t> ToneSeq = std::move(std::get<3>(PreprocessedSeq));
	auto LanguageSeq = std::move(std::get<4>(PreprocessedSeq));
	auto DurationSeq = std::move(std::get<5>(PreprocessedSeq));
	auto BertAligSeq = std::move(std::get<6>(PreprocessedSeq));
	bool HasBertSeq = std::get<7>(PreprocessedSeq);

	if (PhonemeSeq.empty() || Skip)
	{
		logger.log("[Info] Skip Empty Seq");
		return {};
	}

	std::vector<Ort::Value> EncoderInputs, EncoderOutputs;

	std::mt19937 gen(static_cast<unsigned int>(Seq.Seed));
	std::normal_distribution FloatRandFn(0.f, 1.f);

	std::vector<int64_t> TextSeq = CleanedSeq2Indices(PhonemeSeq);
	const int64_t TextSeqShape[2] = { 1,(int64_t)TextSeq.size() };

	int64_t TextSeqLength[] = { TextSeqShape[1] };
	constexpr int64_t LengthShape[1] = { 1 };

	//Indices Seq
	EncoderInputs.push_back(Ort::Value::CreateTensor(
		*memory_info, TextSeq.data(), TextSeqLength[0], TextSeqShape, 2));

	//Indices Length
	if (UseLength)
		EncoderInputs.push_back(Ort::Value::CreateTensor(
			*memory_info, TextSeqLength, 1, LengthShape, 1));

	//Emotional Vits
	std::vector<float> EmotionVector;
	constexpr int64_t EmotionShape[1] = { 1024 };
	if (Emotion)
	{
		EmotionVector = GetEmotionVector(Seq.EmotionPrompt);
		EncoderInputs.push_back(Ort::Value::CreateTensor(
			*memory_info, EmotionVector.data(), 1024, EmotionShape, 1));
	}

	//Tone Seq
	std::vector ToneIn(TextSeq.size(), 0i64);
	if (UseTone)
	{
		if (ToneIn.size() == ToneSeq.size())
			ToneIn = ToneSeq;
		else if (AddBlank && ToneIn.size() == ToneSeq.size() * 2 + 1)
			for (size_t i = 1; i < ToneIn.size(); i += 2)
				ToneIn[i] = ToneSeq[i / 2];
		else if (ToneIn.size() * 2 + 1 == ToneSeq.size())
			for (size_t i = 1; i < ToneSeq.size(); i += 2)
				ToneIn[i / 2] = ToneSeq[i];
		EncoderInputs.push_back(Ort::Value::CreateTensor(
			*memory_info, ToneIn.data(), TextSeqLength[0], TextSeqShape, 2));
	}

	//Language Seq
	std::vector LanguageIn(TextSeq.size(), CurLanguageIdx);
	if (UseLanguage)
	{
		if (LanguageIn.size() == LanguageSeq.size())
			LanguageIn = LanguageSeq;
		else if (AddBlank && LanguageIn.size() == LanguageSeq.size() * 2 + 1)
			for (size_t i = 1; i < LanguageIn.size(); i += 2)
			{
				LanguageIn[i] = LanguageSeq[i / 2];
				LanguageIn[i - 1] = LanguageSeq[i / 2];
			}
		else if (LanguageIn.size() * 2 + 1 == LanguageSeq.size())
			for (size_t i = 1; i < LanguageSeq.size(); i += 2)
				LanguageIn[i / 2] = LanguageSeq[i];
		EncoderInputs.push_back(Ort::Value::CreateTensor(
			*memory_info, LanguageIn.data(), TextSeqLength[0], TextSeqShape, 2));
	}

	//Bert Seqs
	std::vector BertVecs(Index2BertName.size(), std::vector<float>(0));
	std::vector<int64_t[2]> BertShapes(Index2BertName.size());
	if (UseBert)
	{
		for (size_t IndexOfBert = 0; IndexOfBert < Index2BertName.size(); ++IndexOfBert)
		{
			auto& BertData = BertVecs[IndexOfBert];
			auto& BertShape = BertShapes[IndexOfBert];
			BertShape[0] = TextSeqLength[0];
			const auto CurBertSession = GetBertModel(GetBertNameWithIndex((int64_t)IndexOfBert));
			if (CurBertSession &&
				IndexOfBert == size_t(GetBertIdWithLanguageSymbol(Seq.LanguageSymbol)) &&
				BertEnabled &&
				HasBertSeq)
			{
				const auto SeqBert = GetBertFeature(IndexOfBert, TokenSeq, CurBertSession);
				int64_t BertSize = SeqBert[0].GetTensorTypeAndShapeInfo().GetShape().back();
				BertShape[1] = BertSize;
				BertData.resize(BertShape[1] * BertShape[0]);
				const auto AttnData = SeqBert[0].GetTensorData<float>();
				if (BertAligSeq.size() == 1)
					BertAligSeq = GetAligments(TextSeqLength[0], SeqBert[0].GetTensorTypeAndShapeInfo().GetShape()[0] - 2);
				for (int64_t IndexOfSrcVector = 0; IndexOfSrcVector < TextSeqLength[0]; ++IndexOfSrcVector)
					memcpy(BertData.data() + IndexOfSrcVector * BertSize, AttnData + (BertAligSeq[IndexOfSrcVector]) * BertSize, BertSize * sizeof(float));
			}
			else
			{
				BertShape[1] = DefBertSize;
				BertData.resize(BertShape[1] * BertShape[0], 0.f);
			}
			EncoderInputs.emplace_back(Ort::Value::CreateTensor(
				*memory_info, BertData.data(), BertData.size(), BertShape, 2));
		}
	}

	//Clap Emotion Seq
	auto ClapData = std::vector(512, 0.f);
	int64_t ClapShape[2] = { 512, 1 };
	
	if (UseClap)
	{
		const auto CurClapSession = GetClapModel(ClapName);
		std::vector<Ort::Value> ClapInput;
		std::wstring ClapSeq;
		if (CurClapSession && !ClapSeq.empty())
		{
			for (const auto& itt : Seq.EmotionPrompt)
				ClapSeq += itt + L" ";
			std::vector<Ort::Value> ClapOutput;
			const auto& CurClapTokenizer = GetTokenizer(ClapName);
			auto input_ids = CurClapTokenizer(CurClapTokenizer.Tokenize(ClapSeq), true);
			std::vector<int64_t> attention_mask(input_ids.size(), 1);
			int64_t AttentionShape[2] = { 1, (int64_t)input_ids.size() };
			ClapInput.emplace_back(Ort::Value::CreateTensor(
				*memory_info, input_ids.data(), input_ids.size(), AttentionShape, 2));
			ClapInput.emplace_back(Ort::Value::CreateTensor(
				*memory_info, attention_mask.data(), attention_mask.size(), AttentionShape, 2));
			try
			{
				ClapOutput = CurClapSession->Run(Ort::RunOptions{ nullptr },
					BertInputNames.data(),
					ClapInput.data(),
					CurClapSession->GetInputCount(),
					BertOutputNames.data(),
					1);
			}
			catch (Ort::Exception& e)
			{
				LibDLVoiceCodecThrow((std::string("Locate: Clap\n") + e.what()))
			}
			ClapData = { ClapOutput[0].GetTensorData<float>(),ClapOutput[0].GetTensorData<float>() + 512 };
		}
		EncoderInputs.emplace_back(Ort::Value::CreateTensor(*memory_info, ClapData.data(), ClapData.size(), ClapShape, 2));
	}

	int64_t VQIndices[] = { 0 };
	int64_t SidIndices[] = { GetSpeakerIdWithSpeakerName(Seq.SpeakerName)};

	std::vector<float> GEmbidding;
	std::vector<int64_t> GOutShape;
	if (sessionEmb)
	{
		auto SpeakerMixData = Seq.SpeakerMix;
		if (!SpeakerMixData.empty() && SpeakerCount > 1)
		{
			LinearCombination(SpeakerMixData);
			int64_t csid = 0;
			for (const auto& CharaP : SpeakerMixData)
			{
				std::vector<Ort::Value> EmbiddingInput;
				std::vector<Ort::Value> EmbiddingOutput;
				if (csid >= SpeakerCount)
					break;
				if (CharaP < 0.0001f)
				{
					++csid;
					continue;
				}
				int64_t Character[1] = { csid };
				EmbiddingInput.push_back(Ort::Value::CreateTensor(
					*memory_info, Character, 1, LengthShape, 1));
				try
				{
					EmbiddingOutput = sessionEmb->Run(Ort::RunOptions{ nullptr },
						EmbiddingInputNames.data(),
						EmbiddingInput.data(),
						EmbiddingInput.size(),
						EmbiddingOutputNames.data(),
						EmbiddingOutputNames.size());
				}
				catch (Ort::Exception& e)
				{
					LibDLVoiceCodecThrow((std::string("Locate: emb\n") + e.what()))
				}
				const auto GOutCount = EmbiddingOutput[0].GetTensorTypeAndShapeInfo().GetElementCount();
				if (GOutShape.empty())
				{
					GEmbidding = std::vector(EmbiddingOutput[0].GetTensorData<float>(), EmbiddingOutput[0].GetTensorData<float>() + GOutCount);
					GOutShape = EmbiddingOutput[0].GetTensorTypeAndShapeInfo().GetShape();
					GOutShape.emplace_back(1);
					for (auto idx : GEmbidding)
						idx *= float(CharaP);
				}
				else
					for (size_t i = 0; i < GOutCount; ++i)
						GEmbidding[i] += EmbiddingOutput[0].GetTensorData<float>()[i] * float(CharaP);
				++csid;
			}
		}
		else
		{
			std::vector<Ort::Value> EmbiddingInput;
			std::vector<Ort::Value> EmbiddingOutput;
			EmbiddingInput.push_back(Ort::Value::CreateTensor(
				*memory_info, SidIndices, 1, LengthShape, 1));
			try
			{
				EmbiddingOutput = sessionEmb->Run(Ort::RunOptions{ nullptr },
					EmbiddingInputNames.data(),
					EmbiddingInput.data(),
					EmbiddingInput.size(),
					EmbiddingOutputNames.data(),
					EmbiddingOutputNames.size());
			}
			catch (Ort::Exception& e)
			{
				LibDLVoiceCodecThrow((std::string("Locate: emb\n") + e.what()))
			}
			const auto GOutCount = EmbiddingOutput[0].GetTensorTypeAndShapeInfo().GetElementCount();
			GEmbidding = std::vector(EmbiddingOutput[0].GetTensorData<float>(), EmbiddingOutput[0].GetTensorData<float>() + GOutCount);
			GOutShape = EmbiddingOutput[0].GetTensorTypeAndShapeInfo().GetShape();
			GOutShape.emplace_back(1);
		}
		if (EncoderG)
			EncoderInputs.push_back(Ort::Value::CreateTensor(*memory_info, GEmbidding.data(), GEmbidding.size(), GOutShape.data(), 3));
	}

	if (UseVQ)
	{
		if (!Seq.EmotionPrompt.empty())
			VQIndices[0] = _wtoi64(Seq.EmotionPrompt[0].c_str());
		EncoderInputs.push_back(Ort::Value::CreateTensor(*memory_info, VQIndices, 1, LengthShape, 1));
		EncoderInputs.push_back(Ort::Value::CreateTensor(*memory_info, SidIndices, 1, LengthShape, 1));
	}

	try
	{
		EncoderOutputs = sessionEnc_p->Run(Ort::RunOptions{ nullptr },
			EncoderInputNames.data(),
			EncoderInputs.data(),
			EncoderInputs.size(),
			EncoderOutputNames.data(),
			EncoderOutputNames.size());
	}
	catch (Ort::Exception& e)
	{
		LibDLVoiceCodecThrow((std::string("Locate: enc_p\n") + e.what()))
	}

	std::vector<float>
		m_p(EncoderOutputs[1].GetTensorData<float>(), EncoderOutputs[1].GetTensorData<float>() + EncoderOutputs[1].GetTensorTypeAndShapeInfo().GetElementCount()),
		logs_p(EncoderOutputs[2].GetTensorData<float>(), EncoderOutputs[2].GetTensorData<float>() + EncoderOutputs[2].GetTensorTypeAndShapeInfo().GetElementCount()),
		x_mask(EncoderOutputs[3].GetTensorData<float>(), EncoderOutputs[3].GetTensorData<float>() + EncoderOutputs[3].GetTensorTypeAndShapeInfo().GetElementCount());

	const auto xshape = EncoderOutputs[0].GetTensorTypeAndShapeInfo().GetShape();

	std::vector w_ceil(TextSeqLength[0], 1.f);
	bool enable_dp = false;
	if (DurationSeq.size() == w_ceil.size() || DurationSeq.size() == w_ceil.size() / 2)
		enable_dp = true;

	const int64_t zinputShape[3] = { xshape[0],2,xshape[2] };
	const int64_t zinputCount = xshape[0] * xshape[2] * 2;
	std::vector<float> zinput(zinputCount, 0.0);
	for (auto& it : zinput)
		it = FloatRandFn(gen) * Seq.DurationPredictorNoiseScale;
	std::vector<Ort::Value> DurationPredictorInput;
	DurationPredictorInput.push_back(std::move(EncoderOutputs[0]));
	DurationPredictorInput.push_back(std::move(EncoderOutputs[3]));
	DurationPredictorInput.push_back(Ort::Value::CreateTensor(
		*memory_info, zinput.data(), zinputCount, zinputShape, 3));
	if (sessionEmb)
		DurationPredictorInput.push_back(Ort::Value::CreateTensor(*memory_info, GEmbidding.data(), GEmbidding.size(), GOutShape.data(), 3));
	if (sessionSdp)
	{
		std::vector<Ort::Value> StochasticDurationPredictorOutput;
		try
		{
			StochasticDurationPredictorOutput = sessionSdp->Run(Ort::RunOptions{ nullptr },
				SdpInputNames.data(),
				DurationPredictorInput.data(),
				DurationPredictorInput.size(),
				SdpOutputNames.data(),
				SdpOutputNames.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow((std::string("Locate: dp\n") + e.what()))
		}
		const auto w_data = StochasticDurationPredictorOutput[0].GetTensorMutableData<float>();
		const auto w_data_length = StochasticDurationPredictorOutput[0].GetTensorTypeAndShapeInfo().GetElementCount();
		if (w_data_length != w_ceil.size())
			w_ceil.resize(w_data_length, 0.f);
		float SdpFactor = 1.f - Seq.FactorDpSdp;
		if (sessionDp)
			for (size_t i = 0; i < w_ceil.size(); ++i)
				w_ceil[i] = ceil(exp(w_data[i] * SdpFactor) * x_mask[i] * Seq.LengthScale);
		else
			for (size_t i = 0; i < w_ceil.size(); ++i)
				w_ceil[i] = ceil(exp(w_data[i]) * x_mask[i] * Seq.LengthScale);
	}
	if (sessionDp)
	{
		std::vector<Ort::Value> DurationPredictorOutput;
		DurationPredictorInput.erase(DurationPredictorInput.begin() + 2);
		try
		{
			DurationPredictorOutput = sessionDp->Run(Ort::RunOptions{ nullptr },
				DpInputNames.data(),
				DurationPredictorInput.data(),
				DurationPredictorInput.size(),
				DpOutputNames.data(),
				DpOutputNames.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow((std::string("Locate: dp\n") + e.what()))
		}
		const auto w_data = DurationPredictorOutput[0].GetTensorMutableData<float>();
		const auto w_data_length = DurationPredictorOutput[0].GetTensorTypeAndShapeInfo().GetElementCount();
		if (w_data_length != w_ceil.size())
			w_ceil.resize(w_data_length, 0.f);
		if (sessionSdp)
			for (size_t i = 0; i < w_ceil.size(); ++i)
				w_ceil[i] += ceil(exp(w_data[i] * Seq.FactorDpSdp) * x_mask[i] * Seq.LengthScale);
		else
			for (size_t i = 0; i < w_ceil.size(); ++i)
				w_ceil[i] = ceil(exp(w_data[i]) * x_mask[i] * Seq.LengthScale);
	}
	if (enable_dp)
	{
		if (DurationSeq.size() == TextSeq.size())
			for (size_t i = 0; i < w_ceil.size(); ++i)
				w_ceil[i] = float(DurationSeq[i]);
		else if (AddBlank && DurationSeq.size() == TextSeq.size() / 2ull)
			for (size_t i = 0; i < DurationSeq.size(); ++i)
				w_ceil[1 + i * 2] = float(DurationSeq[i]);
	}
	CustomDurationCallback(w_ceil);
	const auto maskSize = x_mask.size();
	float y_length_f = 0.0;
	int64_t y_length;
	for (size_t i = 0; i < w_ceil.size(); ++i)
		y_length_f += w_ceil[i];
	if (y_length_f < 1.0f)
		y_length = 1;
	else
		y_length = (int64_t)y_length_f;

	auto attn = generatePath(w_ceil.data(), y_length, maskSize);
	std::vector logVec(192, std::vector(y_length, 0.0f));
	std::vector mpVec(192, std::vector(y_length, 0.0f));
	std::vector<float> nlogs_pData(192 * y_length);
	for (size_t i = 0; i < static_cast<size_t>(y_length); ++i)
	{
		for (size_t j = 0; j < 192; ++j)
		{
			for (size_t k = 0; k < maskSize; k++)
			{
				if (attn[i][k])
				{
					mpVec[j][i] += m_p[j * maskSize + k];
					logVec[j][i] += logs_p[j * maskSize + k];
				}
			}
			nlogs_pData[j * y_length + i] = mpVec[j][i] + FloatRandFn(gen) * exp(logVec[j][i]) * Seq.NoiseScale;
		}
	}
	std::vector y_mask(y_length, 1.0f);
	const int64_t zshape[3] = { 1,192,y_length };
	const int64_t yshape[3] = { 1,1,y_length };

	std::vector<Ort::Value> FlowDecInputs, FlowDecOutputs;

	FlowDecInputs.push_back(Ort::Value::CreateTensor<float>(
		*memory_info, nlogs_pData.data(), 192 * y_length, zshape, 3));
	FlowDecInputs.push_back(Ort::Value::CreateTensor<float>(
		*memory_info, y_mask.data(), y_length, yshape, 3));
	if (sessionEmb)
		FlowDecInputs.push_back(Ort::Value::CreateTensor<float>(
			*memory_info, GEmbidding.data(), GEmbidding.size(), GOutShape.data(), 3));

	try
	{
		FlowDecOutputs = sessionFlow->Run(Ort::RunOptions{ nullptr },
			FlowInputNames.data(),
			FlowDecInputs.data(),
			FlowDecInputs.size(),
			FlowOutputNames.data(),
			FlowOutputNames.size());
	}
	catch (Ort::Exception& e)
	{
		LibDLVoiceCodecThrow((std::string("Locate: dec & flow\n") + e.what()))
	}
	FlowDecInputs[0] = std::move(FlowDecOutputs[0]);
	if (sessionEmb)
		FlowDecInputs[1] = std::move(FlowDecInputs[2]);
	FlowDecInputs.pop_back();
	try
	{

		FlowDecOutputs = sessionDec->Run(Ort::RunOptions{ nullptr },
			DecInputNames.data(),
			FlowDecInputs.data(),
			FlowDecInputs.size(),
			DecOutputNames.data(),
			DecOutputNames.size());
	}
	catch (Ort::Exception& e)
	{
		LibDLVoiceCodecThrow((std::string("Locate: dec & flow\n") + e.what()))
	}
	const auto shapeOut = FlowDecOutputs[0].GetTensorTypeAndShapeInfo().GetShape();
	const auto outData = FlowDecOutputs[0].GetTensorData<float>();
	PCMDATA.reserve(shapeOut[2] * 2);
	for (int bbb = 0; bbb < shapeOut[2]; bbb++)
		PCMDATA.emplace_back(static_cast<int16_t>(outData[bbb] * 32768.0f));
	return PCMDATA;
}

std::vector<std::vector<int16_t>> Vits::Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const
{
	std::vector<std::vector<int16_t>> PCM;
	PCM.reserve(_Input.size());
	logger.log("[Inference] Vits Inference Begin");
	size_t proc = 0;
	_callback(proc, _Input.size());
	for (const auto& Seq : _Input)
	{
		_callback(proc++, _Input.size());
		if (Seq.RestTime < 0.f || &Seq == &_Input.front())
			PCM.emplace_back();

		const auto _Audio = Infer(Seq);
		auto& BackOfPCMDatas = PCM.back();
		if (Seq.RestTime > 0.f)
			BackOfPCMDatas.insert(BackOfPCMDatas.end(), size_t(double(Seq.RestTime) * double(_samplingRate)), 0);
		BackOfPCMDatas.insert(BackOfPCMDatas.end(), _Audio.begin(), _Audio.end());
	}
	_callback(proc, _Input.size());
	logger.log("[Inference] Vits Inference Fin");
	return PCM;
}

MoeVoiceStudioCoreEnd