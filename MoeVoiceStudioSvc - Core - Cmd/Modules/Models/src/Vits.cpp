#include "../header/Vits.hpp"
#include <random>

MoeVoiceStudioCoreHeader

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
		throw std::exception("[Error] Missing field \"folder\" (Model Folder)");
	if (!_Config["Folder"].IsString())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Must Be String");
	const auto _folder = to_wide_string(_Config["Folder"].GetString());
	if (_folder.empty())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Can Not Be Empty");
	const std::wstring _path = GetCurrentFolder() + L"\\Models\\" + _folder + L"\\" + _folder;

	std::map<std::string, std::wstring> _PathDict;

	if(_Config.HasMember("EmotionalPath") && _Config["EmotionalPath"].IsString())
	{
		const auto emoStringload = to_wide_string(_Config["EmotionalPath"].GetString());
		if(!emoStringload.empty())
		{
			_PathDict["EmotionalPath"] = GetCurrentFolder() + L"\\emotion\\" + emoStringload + L".npy";
			_PathDict["EmotionalDictPath"] = GetCurrentFolder() + L"\\emotion\\" + emoStringload + L".json";
		}
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
		throw std::exception("[Error] Missing field \"Type\" (ModelType)");
	if (!_Config["Type"].IsString())
		throw std::exception("[Error] Field \"Type\" (ModelType) Must Be String");
	VitsType = _Config["Type"].GetString();
	if (VitsType == "Pits")
	{
		UseTone = true;
		UseLength = false;
	}
	if (VitsType == "BertVits")
	{
		UseLength = false;
		UseTone = true;
		UseBert = true;
		UseLanguage = true;
		EncoderG = true;
	}

	Cleaner = MoeVSG2P::GetDefCleaner();
	if (_PathDict.find("Dict") != _PathDict.end())
		if (_waccess(_PathDict.at("Dict").c_str(), 0) != -1)
			Cleaner->loadDict(_PathDict.at("Dict"));

	if (_Config.HasMember("LanguageMap") && !_Config["LanguageMap"].IsNull())
		for (const auto& CMember : _Config["LanguageMap"].GetMemberArray())
			LanguageMap[CMember.first] = CMember.second.GetInt();
	else
		logger.log("[Warn] Field \"LanguageMap\" Is Missing, Use Default Value");

	if (UseLength)
		EncoderInputNames.emplace_back("x_lengths");
	if (UseTone)
		EncoderInputNames.emplace_back("t");
	if(Emotion)
		EncoderInputNames.emplace_back("emotion");
	if (UseLanguage)
		EncoderInputNames.emplace_back("language");

	//Check SamplingRate
	if (_Config["Rate"].IsNull())
		throw std::exception("[Error] Missing field \"Rate\" (SamplingRate)");
	if (_Config["Rate"].IsInt() || _Config["Rate"].IsInt64())
		_samplingRate = _Config["Rate"].GetInt();
	else
		throw std::exception("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64");

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	//Check Symbol
	if (!_Config.HasMember("Symbol") || _Config["Symbol"].IsNull())
		throw std::exception("[Error] Missing field \"Symbol\" (PhSymbol)");
	if (_Config.HasMember("AddBlank") && !_Config["AddBlank"].IsNull())
		AddBlank = _Config["AddBlank"].GetBool();
	else
		logger.log(L"[Warn] Field \"AddBlank\" Is Missing, Use Default Value");

	//Load Symbol
	int64_t iter = 0;
	if (_Config["Symbol"].IsArray())
	{
		logger.log(L"[Info] Use Phs");
		if (_Config["Symbol"].Empty())
			throw std::exception("[Error] Field \"Symbol\" (PhSymbol) Can Not Be Empty");
		const auto SymbolArr = _Config["Symbol"].GetArray();
		if (!SymbolArr[0].IsString())
			throw std::exception("[Error] Field \"Symbol\" (PhSymbol) Must Be Array<String> or String");
		for (const auto& it : SymbolArr)
			Symbols.insert({ to_wide_string(it.GetString()), iter++ });
	}
	else
	{
		if (!_Config["Symbol"].IsString())
			throw std::exception("[Error] Field \"Symbol\" (PhSymbol) Must Be Array<String> or String");
		logger.log(L"[Info] Use Symbols");
		const std::wstring SymbolsStr = to_wide_string(_Config["Symbol"].GetString());
		if (SymbolsStr.empty())
			throw std::exception("[Error] Field \"Symbol\" (PhSymbol) Can Not Be Empty");
		for (size_t i = 0; i < SymbolsStr.length(); ++i)
			Symbols.insert({ SymbolsStr.substr(i,1) , iter++ });
	}

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
				EmoJson = { to_byte_string(EmotionPath).c_str() };
		}
	}
	catch (std::exception& e)
	{
		logger.log((std::string("[Warn] EmotionPath Error ") + e.what()).c_str());
	}

	if (_Config.HasMember("Characters") && _Config["Characters"].IsArray())
		SpeakerCount = (int64_t)_Config["Characters"].Size();

	if(UseLanguage)
	{
		if (_Config["LanguageMap"].IsNull() || !_Config.HasMember("LanguageMap"))
			throw std::exception("[Error] Missing field \"LanguageMap\" (LanguageMap)");
		for(const auto& Item : _Config["LanguageMap"].GetMemberArray())
		{
			if (!Item.second.IsArray())
				continue;
			const auto LangArr = Item.second.GetArray();
			if (LangArr.size() != 2)
				continue;
			LanguageMap[Item.first] = LangArr[0].GetInt();
			LanguageTones[Item.first] = LangArr[1].GetInt();
		}
	}

	if (UseBert)
	{
		if (LanguageMap.size() != _BertPaths.size())
			EncoderInputNames.emplace_back("bert");
		else
		{
			BertNames.reserve(_BertPaths.size() * 2);
			for (size_t i = 0; i < _BertPaths.size(); ++i)
				BertNames.emplace_back("bert_" + std::to_string(i));
			for(const auto& NameInp : BertNames)
				EncoderInputNames.emplace_back(NameInp.data());
		}
		for(const auto& Path : _BertPaths)
		{
			if (_waccess(Path.c_str(), 0) != -1)
			{
				Ort::Session* SessionBert = nullptr;
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
				sessionBert.emplace_back(SessionBert);
				if (_waccess((Path + L"/Tokenizer.json").c_str(), 0) != -1)
				{
					Tokenizers.emplace_back(Path + L"/Tokenizer.json");
					Tokenizers.back().BondCleaner(Cleaner);
				}
				else if (SessionBert)
					throw std::exception("Bert Must Have a Tokenizer");
			}
		}
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
			throw std::exception("You must have a duration predictor");
		}

		logger.log(L"[Info] Vits Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		destory();
		throw std::exception(_exception.what());
	}

	if (sessionEmb)
	{
		if(EncoderG) EncoderInputNames.emplace_back("g");
		SdpInputNames.emplace_back("g");
		DpInputNames.emplace_back("g");
		FlowInputNames.emplace_back("g");
		DecInputNames.emplace_back("g");
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

std::vector<std::vector<int16_t>> Vits::Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const
{
	std::vector<std::vector<int16_t>> PCM;
	PCM.reserve(_Input.size());
	std::vector<std::vector<int16_t>> _Audio(1);
	logger.log("[Inference] Vits Inference Begin");
	size_t proc = 0;
	_callback(proc, _Input.size());
	for(const auto& Seq : _Input)
	{
		_callback(proc++, _Input.size());
		if(Seq.Seq.empty())
			continue;

		if (!_Audio[0].empty())
		{
			if (Seq.RestTime < 0.f)
			{
				_Audio[0].insert(_Audio[0].end(), size_t(_samplingRate), 0);
				PCM.emplace_back(std::move(_Audio[0]));
				_Audio[0] = std::vector<int16_t>();
			}
			else
				_Audio[0].insert(_Audio[0].end(), size_t(Seq.RestTime * float(_samplingRate)), 0);
		}

		std::mt19937 gen(static_cast<unsigned int>(Seq.Seed));
		std::normal_distribution FloatRandFn(0.f, 1.f);
		std::uniform_int_distribution IntRandFn(0, RAND_MAX);

		std::vector<int64_t> TextSeq;
		TextSeq.reserve(Seq.Seq.size() * 4 + 4);
		for (const auto& it : Seq.Seq)
		{
			if (AddBlank)
				TextSeq.push_back(0);
			if (Symbols.find(it) != Symbols.end())
				TextSeq.push_back(Symbols.at(it));
			else
				TextSeq.push_back(int64_t(size_t(IntRandFn(gen)) % Symbols.size()));
		}
		if (AddBlank)
			TextSeq.push_back(0);
		int64_t TextSeqLength[] = { (int64_t)TextSeq.size() };
		std::vector<Ort::Value> EncoderOutputs;
		std::vector<Ort::Value> EncoderInputs;
		const int64_t TextSeqShape[2] = { 1,TextSeqLength[0] };
		constexpr int64_t LengthShape[1] = { 1 };
		EncoderInputs.push_back(Ort::Value::CreateTensor(
			*memory_info, TextSeq.data(), TextSeqLength[0], TextSeqShape, 2));
		if (UseLength)
			EncoderInputs.push_back(Ort::Value::CreateTensor(
				*memory_info, TextSeqLength, 1, LengthShape, 1));
		std::vector<float> emoVec;
		constexpr int64_t EmotionShape[1] = { 1024 };
		if(Emotion)
		{
			emoVec = GetEmotionVector(Seq.EmotionPrompt);
			EncoderInputs.push_back(Ort::Value::CreateTensor(
				*memory_info, emoVec.data(), 1024, EmotionShape, 1));
		}
		std::vector ToneIn(TextSeq.size(), 0i64);
		if(UseTone)
		{
			if (ToneIn.size() == Seq.Tones.size())
				ToneIn = Seq.Tones;
			else if (AddBlank && ToneIn.size() == Seq.Tones.size() * 2 + 1)
				for (size_t i = 1; i < ToneIn.size(); i += 2)
					ToneIn[i] = Seq.Tones[i / 2];
			else if (ToneIn.size() * 2 + 1 == Seq.Tones.size())
				for (size_t i = 1; i < Seq.Tones.size(); i += 2)
					ToneIn[i / 2] = Seq.Tones[i];
			EncoderInputs.push_back(Ort::Value::CreateTensor(
				*memory_info, ToneIn.data(), TextSeqLength[0], TextSeqShape, 2));
		}
		std::vector LanguageIn(TextSeq.size(), Seq.TotLang);
		if(UseLanguage)
		{
			if (LanguageIn.size() == Seq.Tones.size())
				LanguageIn = Seq.Tones;
			else if (AddBlank && LanguageIn.size() == Seq.Tones.size() * 2 + 1)
				for (size_t i = 1; i < LanguageIn.size(); i += 2)
					LanguageIn[i] = Seq.Tones[i / 2];
			else if (LanguageIn.size() * 2 + 1 == Seq.Tones.size())
				for (size_t i = 1; i < Seq.Tones.size(); i += 2)
					LanguageIn[i / 2] = Seq.Tones[i];
			EncoderInputs.push_back(Ort::Value::CreateTensor(
				*memory_info, LanguageIn.data(), TextSeqLength[0], TextSeqShape, 2));
		}
		std::vector BertVecs(sessionBert.size(), std::vector(1024 * TextSeqLength[0], 0.f));
		int64_t BertShape[2] = { TextSeqLength[0],1024 };
		if(UseBert)
		{
			for (size_t IndexOfBert = 0; IndexOfBert < sessionBert.size(); ++IndexOfBert)
			{
				auto& BertData = BertVecs[IndexOfBert];
				if (sessionBert[IndexOfBert] && (IndexOfBert == size_t(Seq.TotLang) ||
					(IndexOfBert != size_t(Seq.TotLang) && sessionBert.size() == 1)))
				{
					auto input_ids = Tokenizers[IndexOfBert](TextNormalize(Seq.SeqStr, Seq.TotLang));
					std::vector<int64_t> attention_mask(input_ids.size(), 1), token_type_ids(input_ids.size(), 0);
					int64_t AttentionShape[2] = { 1, (int64_t)input_ids.size() };
					std::vector<Ort::Value> AttentionInput, AttentionOutput;
					AttentionInput.emplace_back(Ort::Value::CreateTensor(
						*memory_info, input_ids.data(), input_ids.size(), AttentionShape, 2));
					AttentionInput.emplace_back(Ort::Value::CreateTensor(
						*memory_info, attention_mask.data(), attention_mask.size(), AttentionShape, 2));
					AttentionInput.emplace_back(Ort::Value::CreateTensor(
						*memory_info, token_type_ids.data(), token_type_ids.size(), AttentionShape, 2));
					try
					{
						AttentionOutput = sessionBert[IndexOfBert]->Run(Ort::RunOptions{ nullptr },
							BertInputNames.data(),
							AttentionInput.data(),
							3,
							BertOutputNames.data(),
							1);
					}
					catch (Ort::Exception& e)
					{
						throw std::exception((std::string("Locate: Bert\n") + e.what()).c_str());
					}
					const auto AligmentMartix = GetAligments(BertShape[0], AttentionOutput[0].GetTensorTypeAndShapeInfo().GetShape()[0]);
					const auto AttnData = AttentionOutput[0].GetTensorData<float>();
					for (int64_t IndexOfSrcVector = 0; IndexOfSrcVector < TextSeqLength[0]; ++IndexOfSrcVector)
						memcpy(BertData.data() + IndexOfSrcVector * 1024, AttnData + AligmentMartix[IndexOfSrcVector] * 1024, 1024 * sizeof(float));
				}
				EncoderInputs.emplace_back(Ort::Value::CreateTensor(
					*memory_info, BertData.data(), BertData.size(), BertShape, 2));
			}
		}

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
						throw std::exception((std::string("Locate: emb\n") + e.what()).c_str());
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
				int64_t Character[1] = { Seq.SpeakerId };
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
					throw std::exception((std::string("Locate: emb\n") + e.what()).c_str());
				}
				const auto GOutCount = EmbiddingOutput[0].GetTensorTypeAndShapeInfo().GetElementCount();
				GEmbidding = std::vector(EmbiddingOutput[0].GetTensorData<float>(), EmbiddingOutput[0].GetTensorData<float>() + GOutCount);
				GOutShape = EmbiddingOutput[0].GetTensorTypeAndShapeInfo().GetShape();
				GOutShape.emplace_back(1);
			}
			if (EncoderG)
				EncoderInputs.push_back(Ort::Value::CreateTensor(*memory_info, GEmbidding.data(), GEmbidding.size(), GOutShape.data(), 3));
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
			throw std::exception((std::string("Locate: enc_p\n") + e.what()).c_str());
		}

		std::vector<float>
			m_p(EncoderOutputs[1].GetTensorData<float>(), EncoderOutputs[1].GetTensorData<float>() + EncoderOutputs[1].GetTensorTypeAndShapeInfo().GetElementCount()),
			logs_p(EncoderOutputs[2].GetTensorData<float>(), EncoderOutputs[2].GetTensorData<float>() + EncoderOutputs[2].GetTensorTypeAndShapeInfo().GetElementCount()),
			x_mask(EncoderOutputs[3].GetTensorData<float>(), EncoderOutputs[3].GetTensorData<float>() + EncoderOutputs[3].GetTensorTypeAndShapeInfo().GetElementCount());

		const auto xshape = EncoderOutputs[0].GetTensorTypeAndShapeInfo().GetShape();

		std::vector w_ceil(TextSeqLength[0], 1.f);
		bool enable_dp = false;
		if (Seq.Durations.size() == w_ceil.size() || Seq.Durations.size() == w_ceil.size() / 2)
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
		if(sessionSdp)
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
				throw std::exception((std::string("Locate: dp\n") + e.what()).c_str());
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
				throw std::exception((std::string("Locate: dp\n") + e.what()).c_str());
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
		if(enable_dp)
		{
			if (Seq.Durations.size() == TextSeq.size())
				for (size_t i = 0; i < w_ceil.size(); ++i)
					w_ceil[i] = float(Seq.Durations[i]);
			else if (AddBlank && Seq.Durations.size() == TextSeq.size() / 2ull)
				for (size_t i = 0; i < Seq.Durations.size(); ++i)
					w_ceil[1 + i * 2] = float(Seq.Durations[i]);
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
			throw std::exception((std::string("Locate: dec & flow\n") + e.what()).c_str());
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
			throw std::exception((std::string("Locate: dec & flow\n") + e.what()).c_str());
		}
		const auto shapeOut = FlowDecOutputs[0].GetTensorTypeAndShapeInfo().GetShape();
		const auto outData = FlowDecOutputs[0].GetTensorData<float>();
		for (int bbb = 0; bbb < shapeOut[2]; bbb++)
			_Audio[0].emplace_back(static_cast<int16_t>(outData[bbb] * 32768.0f));
	}
	if (!_Audio[0].empty())
	{
		_Audio[0].insert(_Audio[0].end(), size_t(_samplingRate), 0);
		PCM.emplace_back(std::move(_Audio[0]));
	}
	_callback(proc++, _Input.size());
	logger.log("[Inference] Vits Inference Fin");
	return PCM;
}

MoeVoiceStudioCoreEnd