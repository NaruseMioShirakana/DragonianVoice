#include "../header/GPT-SoVits.hpp"
#include "../../InferTools/inferTools.hpp"
#include <random>
#include <set>

MoeVoiceStudioCoreHeader
	GptSoVits::~GptSoVits()
{
	logger.log(L"[Info] unloading GptSoVits Models");
	destory();
	logger.log(L"[Info] GptSoVits Models unloaded");
}

GptSoVits::GptSoVits(const MJson& _Config, const ProgressCallback& _ProgressCallback,
	const DurationCallback& _DurationCallback,
	ExecutionProviders ExecutionProvider_,
	unsigned DeviceID_, unsigned ThreadCount_) :
	TextToSpeech(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	//Check Folder
	if (_Config["Folder"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"folder\" (Model Folder)");
	if (!_Config["Folder"].IsString())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Must Be String");
	const auto _folder = to_wide_string(_Config["Folder"].GetString());
	if (_folder.empty())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Can Not Be Empty");
	const std::wstring _path = GetCurrentFolder() + L"/Models/" + _folder + L"/" + _folder;

	std::map<std::string, std::wstring> _PathDict;

	_PathDict["SSL"] = GetCurrentFolder() + L"/Bert/" + L"SSLModel.onnx";
	_PathDict["Vits"] = _path + L"_vits.onnx";
	_PathDict["Encoder"] = _path + L"_t2s_encoder.onnx";
	_PathDict["FDecoder"] = _path + L"_t2s_fsdec.onnx";
	_PathDict["Decoder"] = _path + L"_t2s_sdec.onnx";

	if (_Config.HasMember("Dict") && _Config["Dict"].IsString() && !_Config["Dict"].Empty())
		_PathDict["Dict"] = GetCurrentFolder() + L"/Dict/" + to_wide_string(_Config["Dict"].GetString()) + L".json";

	if (_Config.HasMember("BertPath") && _Config["BertPath"].IsString() && !_Config["BertPath"].Empty())
		_PathDict["Bert"] = GetCurrentFolder() + L"/Bert/" + to_wide_string(_Config["BertPath"].GetString());
	else
		LibDLVoiceCodecThrow("[Error] Missing field \"BertPath\" (Bert Folder)");

	load(_PathDict, _Config, _ProgressCallback, _DurationCallback);
}

void GptSoVits::load(const std::map<std::string, std::wstring>& _PathDict,
	const MJson& _Config, const ProgressCallback& _ProgressCallback,
	const DurationCallback& _DurationCallback)
{
	Cleaner = MoeVSG2P::GetDefCleaner();
	if (_PathDict.find("Dict") != _PathDict.end() && (_waccess(_PathDict.at("Dict").c_str(), 0) != -1))
		Cleaner->loadDict(_PathDict.at("Dict"));
	else
		Cleaner->loadDict(L"");

	if (_Config.HasMember("LanguageMap") && !_Config["LanguageMap"].IsNull())
	{
		for (const auto& CMember : _Config["LanguageMap"].GetMemberArray())
		{
			LanguageSymbol2ID[CMember.first] = CMember.second.GetInt();
			LanguageID2Symbol[CMember.second.GetInt()] = CMember.first;
		}
	}
	else
		logger.log("[Warn] Field \"LanguageMap\" Is Missing, Use Default Value");

	//Check SamplingRate
	if (_Config["Rate"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Rate\" (SamplingRate)");
	if (_Config["Rate"].IsInt() || _Config["Rate"].IsInt64())
		_samplingRate = _Config["Rate"].GetInt();
	else
		LibDLVoiceCodecThrow("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64");

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	//Check Symbol
	if (!_Config.HasMember("Symbol") || _Config["Symbol"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Symbol\" (PhSymbol)");
	if (_Config.HasMember("AddBlank") && !_Config["AddBlank"].IsNull())
		AddBlank = _Config["AddBlank"].GetBool();
	else
		logger.log(L"[Warn] Field \"AddBlank\" Is Missing, Use Default Value");

	if (_Config.HasMember("NumLayers") && _Config["NumLayers"].IsInt())
		NumLayers = _Config["NumLayers"].GetInt();
	else
		logger.log(L"[Warn] Field \"NumLayers\" Is Missing, Use Default Value");

	if (_Config.HasMember("EmbeddingDim") && _Config["EmbeddingDim"].IsInt())
		EmbeddingDim = _Config["EmbeddingDim"].GetInt();
	else
		logger.log(L"[Warn] Field \"EmbeddingDim\" Is Missing, Use Default Value");

	if (_Config.HasMember("EOSId") && _Config["EOSId"].IsInt())
		EOSId = _Config["EOSId"].GetInt();
	else
		logger.log(L"[Warn] Field \"EOSId\" Is Missing, Use Default Value");

	//Load Symbol
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

	if (_Config.HasMember("Characters") && _Config["Characters"].IsArray())
	{
		SpeakerCount = (int64_t)_Config["Characters"].Size();
		int64_t SpkIdx = 0;
		for (const auto& iterator : _Config["Characters"].GetArray())
			SpeakerName2ID[to_wide_string(iterator.GetString())] = SpkIdx++;
	}

	try
	{
		sessionBert = new Ort::Session(*env, (_PathDict.at("Bert") + L"/model.onnx").c_str(), *session_options);
	}
	catch (Ort::Exception& e)
	{
		LibDLVoiceCodecThrow(e.what());
	}
	if (_waccess((_PathDict.at("Bert") + L"/Tokenizer.json").c_str(), 0) != -1)
	{
		Tokenizers.insert({ L"0" ,_PathDict.at("Bert") + L"/Tokenizer.json" });
		Tokenizers[L"0"].BondCleaner(Cleaner);
	}
	else if (sessionBert)
	{
		delete sessionBert;
		sessionBert = nullptr;
		LibDLVoiceCodecThrow("Bert Must Have a Tokenizer");
	}

	_callback = _ProgressCallback;
	CustomDurationCallback = _DurationCallback;

	//LoadModels
	try
	{
		logger.log(L"[Info] loading GptSoVits Models");
		sessionVits = new Ort::Session(*env, _PathDict.at("Vits").c_str(), *session_options);
		sessionSSL = new Ort::Session(*env, _PathDict.at("SSL").c_str(), *session_options);
		sessionEncoder = new Ort::Session(*env, _PathDict.at("Encoder").c_str(), *session_options);
		sessionDecoder = new Ort::Session(*env, _PathDict.at("Decoder").c_str(), *session_options);
		sessionFDecoder = new Ort::Session(*env, _PathDict.at("FDecoder").c_str(), *session_options);
		logger.log(L"[Info] GptSoVits Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		destory();
		LibDLVoiceCodecThrow(_exception.what());
	}

	SpeakerCount = 0;
}

GptSoVits::GptSoVits(const std::map<std::string, std::wstring>& _PathDict,
	const MJson& _Config, const ProgressCallback& _ProgressCallback,
	const DurationCallback& _DurationCallback,
	ExecutionProviders ExecutionProvider_,
	unsigned DeviceID_, unsigned ThreadCount_) :
	TextToSpeech(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	load(_PathDict, _Config, _ProgressCallback, _DurationCallback);
}

std::wregex NCH_Regex(L"[^\\u4e00-\\u9fa5 !@#$%^&*()_+\\-=`~,./;'\\[\\]<>?:\"{}|\\\\。？！，、；：“”‘’『』「」（）〔〕【】─…·—～《》〈〉]+");
std::set ZH_Vowel{L'a', L'e', L'i', L'o', L'u', L'A', L'E', L'I', L'O', L'U'};
std::set Byte_Symbol{L',', L'.', L'?', L'/', L'\"', L'\'', L';', L':', L'!', L' ', L'…' };
std::unordered_map<std::wstring, int64_t> ToneRepMap{
	{ L"[acc_6]", 6 }, { L"[acc_5]", 5 }, { L"[acc_4]", 4 }, { L"[acc_3]", 3 }, { L"[acc_2]", 2 }, { L"[acc_1]", 1 },
	{ L"[acc_-4]", -4 }, { L"[acc_-3]", -3 }, { L"[acc_-2]", -2 }, { L"[acc_-1]", -1 },
};

std::tuple<std::vector<float>, std::vector<int64_t>> GptSoVits::GetBertPhs(const MoeVSProjectSpace::MoeVSTTSSeq& Seq, const MoeVSG2P::Tokenizer& Tokenizer) const
{
	std::vector<std::wstring> Phonemes;
	std::vector<float> OutBert;

	if (sessionBert && Seq.LanguageSymbol == "ZH")
	{
		std::vector<size_t> Bert2Ph;
		std::vector<std::wstring> BertSeq;
		std::vector<int64_t> GlobalTonesSeq;
		if (Seq.SlicedTokens.empty())
		{
			std::wstring TextSeq = MoeVSG2P::NormalizeText(Seq.TextSeq, "ZH");
			if (std::regex_search(TextSeq, NCH_Regex))
				LibDLVoiceCodecThrow("Chinese Sentence ShouldNot Have NonChinese Symbols");
			Phonemes = MoeVSG2P::GetDefCleaner()->DictReplace(MoeVSG2P::GetDefCleaner()->G2p(TextSeq, Seq.PlaceHolderSymbol, L"wt", "ZH"), Seq.PlaceHolderSymbol);

			std::vector<int64_t> LocalTonesSeq;
			std::vector<std::wstring> LocalPhonemeSeq;
			for (const auto& it : Phonemes)
			{
				if (ToneRepMap.find(it) == ToneRepMap.end())
				{
					LocalPhonemeSeq.emplace_back(it);
					LocalTonesSeq.emplace_back(0);
				}
				else if (!LocalTonesSeq.empty())
					*(LocalTonesSeq.end() - 1) = ToneRepMap.at(it);
			}
			if (LocalTonesSeq.size() != LocalPhonemeSeq.size())
				LocalTonesSeq.resize(LocalPhonemeSeq.size(), 0);
			Phonemes = std::move(LocalPhonemeSeq);
			GlobalTonesSeq = std::move(LocalTonesSeq);

			const size_t BertSize = TextSeq.length() + 2, PhoneSize = Phonemes.size();

			Bert2Ph.resize(PhoneSize, 0);
			size_t startFrame = 1;
			for (size_t iph = 0; iph < Phonemes.size(); ++iph)
			{
				if (startFrame == BertSize)
					LibDLVoiceCodecThrow("AligError");
				Bert2Ph[iph] = startFrame;
				if (!Phonemes[iph].empty() && ZH_Vowel.find(Phonemes[iph][0]) != ZH_Vowel.end())
					++startFrame;
				else if (Byte_Symbol.find(Phonemes[iph][0]) != Byte_Symbol.end())
					++startFrame;
				else if (Phonemes[iph] == L"[UNK]")
					++startFrame;
			}

			BertSeq.resize(BertSize);
			BertSeq[0] = L"[CLS]";
			for (size_t iph = 0; iph < Phonemes.size(); ++iph)
				if (BertSeq[Bert2Ph[iph]].empty())
					BertSeq[Bert2Ph[iph]] = TextSeq[Bert2Ph[iph] - 1];
			BertSeq[BertSeq.size() - 1] = L"[SEP]";
		}
		else
		{
			size_t startFrame = 1;
			for(const auto& i : Seq.SlicedTokens)
			{
				BertSeq.emplace_back(i.Text);
				if (i.Text == L"[CLS]")
					continue;
				if (i.Text == L"[SEP]")
					break;
				if(i.Phonemes.empty())
				{
					if(i.Text.empty())
						continue;
					auto TmpPhonemes = MoeVSG2P::GetDefCleaner()->DictReplace(MoeVSG2P::GetDefCleaner()->G2p(i.Text, Seq.PlaceHolderSymbol, L"wt", "ZH"), Seq.PlaceHolderSymbol);

					std::vector<int64_t> LocalTonesSeq;
					std::vector<std::wstring> LocalPhonemeSeq;
					for (const auto& it : TmpPhonemes)
					{
						if (ToneRepMap.find(it) == ToneRepMap.end())
						{
							LocalPhonemeSeq.emplace_back(it);
							LocalTonesSeq.emplace_back(0);
						}
						else if (!LocalTonesSeq.empty())
							*(LocalTonesSeq.end() - 1) = ToneRepMap.at(it);
					}
					if (LocalTonesSeq.size() != LocalPhonemeSeq.size())
						LocalTonesSeq.resize(LocalPhonemeSeq.size(), 0);
					for (auto& ph : LocalPhonemeSeq)
					{
						Phonemes.emplace_back(std::move(ph));
						Bert2Ph.emplace_back(startFrame);
					}
					for (const auto& tones : LocalTonesSeq)
						GlobalTonesSeq.emplace_back(tones);
				}
				else
				{
					for (const auto& ph : i.Phonemes)
					{
						Phonemes.emplace_back(ph);
						Bert2Ph.emplace_back(startFrame);
					}
					auto LocalTonesSeq = i.Tones;
					LocalTonesSeq.resize(i.Phonemes.size(), 0);
					for (const auto& tones : LocalTonesSeq)
						GlobalTonesSeq.emplace_back(tones);
				}
				++startFrame;
			}
		}

		for (size_t i = 0; i < Phonemes.size(); ++i)
		{
			if (GlobalTonesSeq[i] == 5)
				continue;
			auto NewPh = Phonemes[i] + std::to_wstring(GlobalTonesSeq[i]);
			if (GlobalTonesSeq[i] != 0 && Symbols.find(NewPh) != Symbols.end())
				Phonemes[i] = std::move(NewPh);
		}

		//PromptSeq To Bert Features
		auto input_ids = Tokenizer(BertSeq);
		std::vector<int64_t> attention_mask(input_ids.size(), 1), token_type_ids(input_ids.size(), 0);
		int64_t AttentionShape[2] = { 1, (int64_t)input_ids.size() };
		std::vector<Ort::Value> AttentionInput, AttentionOutput;
		AttentionInput.emplace_back(Ort::Value::CreateTensor(
			*memory_info, input_ids.data(), input_ids.size(), AttentionShape, 2));
		if (sessionBert->GetInputCount() == 3)
			AttentionInput.emplace_back(Ort::Value::CreateTensor(*memory_info, attention_mask.data(), attention_mask.size(), AttentionShape, 2));
		AttentionInput.emplace_back(Ort::Value::CreateTensor(
			*memory_info, token_type_ids.data(), token_type_ids.size(), AttentionShape, 2));
		logger.log("[Inference] Infer Bert");
		try
		{
			if (sessionBert->GetInputCount() == 3)
			{
				AttentionOutput = sessionBert->Run(Ort::RunOptions{ nullptr },
					BertInputNames.data(),
					AttentionInput.data(),
					3,
					BertOutputNames.data(),
					1);
			}
			else if (sessionBert->GetInputCount() == 2)
			{
				AttentionOutput = sessionBert->Run(Ort::RunOptions{ nullptr },
					BertInputNames2.data(),
					AttentionInput.data(),
					2,
					BertOutputNames.data(),
					1);
			}
			else
			{
				AttentionOutput = sessionBert->Run(Ort::RunOptions{ nullptr },
					BertInputNames3.data(),
					AttentionInput.data(),
					1,
					BertOutputNames.data(),
					1);
			}
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow((std::string("Locate: Bert\n") + e.what()))
		}
		std::vector<size_t> BertAligSeq;
		if (AddBlank)
			BertAligSeq.emplace_back(0);
		for(auto i : Bert2Ph)
		{
			BertAligSeq.emplace_back(i);
			if (AddBlank)
				BertAligSeq.emplace_back(i);
		}
		const auto AttnData = AttentionOutput[0].GetTensorData<float>();
		for (size_t IndexOfSrcVector = 0; IndexOfSrcVector < BertAligSeq.size(); ++IndexOfSrcVector)
			OutBert.insert(OutBert.end(), AttnData + BertAligSeq[IndexOfSrcVector] * 1024, AttnData + (BertAligSeq[IndexOfSrcVector] + 1) * 1024);
	}
	else
	{
		if (Seq.SlicedTokens.empty())
		{
			if (Seq.TextSeq.empty())
				return {};
			Phonemes = MoeVSG2P::GetDefCleaner()->DictReplace(MoeVSG2P::GetDefCleaner()->G2p(Seq.TextSeq, Seq.PlaceHolderSymbol, Seq.AdditionalInfo, Seq.LanguageSymbol), Seq.PlaceHolderSymbol);
		}
		else
		{
			for (const auto& i : Seq.SlicedTokens)
			{
				if (i.Text == L"[CLS]")
					continue;
				if (i.Text == L"[SEP]")
					break;
				auto TmpPhonemes = i.Phonemes;
				if (TmpPhonemes.empty())
				{
					if (i.Text.empty())
						continue;
					TmpPhonemes = MoeVSG2P::GetDefCleaner()->DictReplace(MoeVSG2P::GetDefCleaner()->G2p(i.Text, Seq.PlaceHolderSymbol, Seq.AdditionalInfo, Seq.LanguageSymbol), Seq.PlaceHolderSymbol);
				}
				for (auto& ph : TmpPhonemes)
					Phonemes.emplace_back(ph);
			}
		}
	}

	//Phoneme To InputSeq
	std::vector<int64_t> OutPhonemes;
	if (AddBlank)
		OutPhonemes.emplace_back(0);
	for (const auto& i : Phonemes)
	{
		if (Symbols.find(i) != Symbols.end())
			OutPhonemes.emplace_back(Symbols.at(i));
		else
			OutPhonemes.emplace_back(0);
		if(AddBlank)
			OutPhonemes.emplace_back(0);
	}

	if (OutBert.empty())
		OutBert = std::vector(OutPhonemes.size() * 1024, 0.f);
	//OutBert = std::vector(OutPhonemes.size() * 1024, 0.f);

	return { std::move(OutBert), std::move(OutPhonemes) };
}

std::vector<std::vector<int16_t>> GptSoVits::Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const
{
	if (_Input.size() < 3 || _Input[0].TextSeq.empty() || _Input[1].TextSeq.empty())
		LibDLVoiceCodecThrow("Input[0] Should Be Reference Text, Input[1] Should Be Reference Path, Input[2:n] Should Be TTS Text");

	const auto RefAudio = InferTools::Wav(_Input[1].TextSeq.c_str());
	if (RefAudio.getHeader().bitsPerSample != 16 || RefAudio.getHeader().NumOfChan != 1)
		LibDLVoiceCodecThrow("RefAudio Should Be signed-int16 and mono");

	std::vector<std::vector<int16_t>> PCM;
	PCM.reserve(_Input.size());
	std::vector<std::vector<int16_t>> _Audio(1);
	logger.log("[Inference] GptSoVits Inference Begin");
	size_t proc = 0;
	const size_t max_proc = _Input.size() + 2;
	_callback(proc, max_proc);

	const auto& Tokenizer = GetTokenizer(L"0");

	const auto RefSize = RefAudio.getHeader().Subchunk2Size / 2;
	const auto RefData = (const int16_t*)RefAudio.getData();

	std::vector<float> RefAudioSrc(RefSize);
	for (size_t i = 0; i < RefSize; ++i)
		RefAudioSrc[i] = float(*(RefData + i)) / 32768.f;
	auto RefAudio16K = InferTools::InterpResample(RefAudioSrc, (int)RefAudio.getHeader().SamplesPerSec, 16000, 1.f);
	auto RefAudioSr = InferTools::InterpResample(RefAudioSrc, (int)RefAudio.getHeader().SamplesPerSec, _samplingRate, 1.f);

	//Reference SSL
	logger.log("[Inference] Infer SSL");
	const int64_t SSLShape[] = { 1, (int64_t)RefAudio16K.size() };
	std::vector<Ort::Value> SSLInput, SSLOutPut;
	SSLInput.emplace_back(Ort::Value::CreateTensor(*memory_info, RefAudio16K.data(), RefAudio16K.size(),
		SSLShape, 2));
	try
	{
		SSLOutPut = sessionSSL->Run(Ort::RunOptions{ nullptr },
			SSLInputNames.data(),
			SSLInput.data(),
			SSLInputNames.size(),
			SSLOutputNames.data(),
			SSLOutputNames.size());
	}
	catch (Ort::Exception& e)
	{
		LibDLVoiceCodecThrow((std::string("Locate: SSL\n") + e.what()))
	}
	_callback(++proc, max_proc);

	logger.log("[Inference] Infer Ref Data");
	auto RefDat = GetBertPhs(_Input[0], Tokenizer);
	std::vector<float> RefBert = std::move(std::get<0>(RefDat));
	std::vector<int64_t> RefSeq = std::move(std::get<1>(RefDat));
	int64_t RefSeqShape[] = { 1, (int64_t)RefSeq.size() };
	int64_t RefBertShape[] = { RefSeqShape[1], 1024 };
	int64_t TempData[] = { 1 };
	_callback(++proc, max_proc);

	std::vector<Ort::Value> EncoderInpTensor;
	EncoderInpTensor.emplace_back(Ort::Value::CreateTensor(*memory_info, RefSeq.data(), RefSeq.size(),
		RefSeqShape, 2));
	EncoderInpTensor.emplace_back(Ort::Value::CreateTensor(*memory_info, TempData, 1,
		TempData, 1));
	EncoderInpTensor.emplace_back(Ort::Value::CreateTensor(*memory_info, RefBert.data(), RefBert.size(),
		RefBertShape, 2));
	EncoderInpTensor.emplace_back(Ort::Value::CreateTensor(*memory_info, TempData, 1,
		TempData, 1));
	EncoderInpTensor.emplace_back(std::move(SSLOutPut[0]));

	for (size_t i = 2; i < _Input.size(); ++i)
	{
		const auto& InferSeq = _Input[i];
		auto SeqAndBert = GetBertPhs(InferSeq, Tokenizer);
		std::vector<float> InputBert = std::move(std::get<0>(SeqAndBert));
		std::vector<int64_t> InputSeq = std::move(std::get<1>(SeqAndBert));
		int64_t SeqShape[] = { 1, (int64_t)InputSeq.size() };
		int64_t BertShape[] = { SeqShape[1], 1024 };
		EncoderInpTensor[1] = Ort::Value::CreateTensor(*memory_info, InputSeq.data(), InputSeq.size(), SeqShape, 2);
		EncoderInpTensor[3] = Ort::Value::CreateTensor(*memory_info, InputBert.data(), InputBert.size(), BertShape, 2);
		std::vector<Ort::Value> EncoderOutput, FDecoderOutput, DecoderOutput;
		try
		{
			EncoderOutput = sessionEncoder->Run(Ort::RunOptions{ nullptr },
				EncoderInputNames.data(),
				EncoderInpTensor.data(),
				EncoderInputNames.size(),
				EncoderOutputNames.data(),
				EncoderOutputNames.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow((std::string("Locate: Encoder\n") + e.what()))
		}

		try
		{
			FDecoderOutput = sessionFDecoder->Run(Ort::RunOptions{ nullptr },
				FDecoderInputNames.data(),
				EncoderOutput.data(),
				FDecoderInputNames.size(),
				FDecoderOutputNames.data(),
				FDecoderOutputNames.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow((std::string("Locate: FDecoder\n") + e.what()))
		}

		int64_t idx = 1;
		for (; idx < InferSeq.MaxDecodeStep; ++idx)
		{
			try
			{
				DecoderOutput = sessionDecoder->Run(Ort::RunOptions{ nullptr },
					DecoderInputNames.data(),
					FDecoderOutput.data(),
					DecoderInputNames.size(),
					DecoderOutputNames.data(),
					DecoderOutputNames.size());
			}
			catch (Ort::Exception& e)
			{
				LibDLVoiceCodecThrow((std::string("Locate: Decoder\n") + e.what()))
			}

			const auto Logit = DecoderOutput[4].GetTensorData<float>();
			int64_t MaxIdx = 0;
			for (int64_t midx = 0; midx < EOSId + 1; ++midx)
				if (Logit[midx] > Logit[MaxIdx])
					MaxIdx = midx;
			if (MaxIdx == EOSId)
				break;
			if (*DecoderOutput[5].GetTensorData<int32_t>() == EOSId)
				break;

			FDecoderOutput[0] = std::move(DecoderOutput[0]);
			FDecoderOutput[1] = std::move(DecoderOutput[1]);
			FDecoderOutput[2] = std::move(DecoderOutput[2]);
			FDecoderOutput[3] = std::move(DecoderOutput[3]);
		}

		auto PredSemanticPtr = DecoderOutput[0].GetTensorData<int64_t>();
		auto PredSemanticShape = DecoderOutput[0].GetTensorTypeAndShapeInfo().GetShape();
		PredSemanticShape.insert(PredSemanticShape.begin(), 1);
		std::vector PredSemantic(PredSemanticPtr + max((PredSemanticShape[2] - idx - 1), 0), PredSemanticPtr + PredSemanticShape[2]);
		if (PredSemantic[PredSemantic.size() - 1] == EOSId)
			PredSemantic[PredSemantic.size() - 1] = 0;
		PredSemanticShape[2] = int64_t(PredSemantic.size());
		logger.log(L"Size Of PredSemantic: " + std::to_wstring(PredSemantic.size()));

		std::vector<Ort::Value> VitsTensors;
		VitsTensors.emplace_back(std::move(EncoderInpTensor[1]));
		VitsTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, PredSemantic.data(), PredSemantic.size(), PredSemanticShape.data(), PredSemanticShape.size()));
		int64_t AudioShape[] = { 1, (int64_t)RefAudioSr.size() };
		VitsTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RefAudioSr.data(), RefAudioSr.size(), AudioShape, 2));

		try
		{
			VitsTensors = sessionVits->Run(Ort::RunOptions{ nullptr },
				VitsInputNames.data(),
				VitsTensors.data(),
				VitsInputNames.size(),
				VitsOutputNames.data(),
				VitsOutputNames.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow((std::string("Locate: SoVits\n") + e.what()))
		}

		if (!_Audio[0].empty())
		{
			if (InferSeq.RestTime < 0.f)
			{
				_Audio[0].insert(_Audio[0].end(), size_t(_samplingRate), 0);
				PCM.emplace_back(std::move(_Audio[0]));
				_Audio[0] = std::vector<int16_t>();
			}
			else
				_Audio[0].insert(_Audio[0].end(), size_t(InferSeq.RestTime * float(_samplingRate)), 0);
		}
		const auto shapeOut = VitsTensors[0].GetTensorTypeAndShapeInfo().GetShape();
		const auto outData = VitsTensors[0].GetTensorData<float>();
		for (int bbb = 0; bbb < shapeOut[0]; bbb++)
			_Audio[0].emplace_back(static_cast<int16_t>(outData[bbb] * 32768.0f));

		_callback(++proc, max_proc);
	}
	if (!_Audio[0].empty())
	{
		_Audio[0].insert(_Audio[0].end(), size_t(_samplingRate), 0);
		PCM.emplace_back(std::move(_Audio[0]));
	}

	return PCM;
}

MoeVoiceStudioCoreEnd