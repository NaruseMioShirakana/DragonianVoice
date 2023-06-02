#include "../header/Tacotron.hpp"
INFERCLASSHEADER
Tacotron2::~Tacotron2()
{
	logger.log(L"[Info] unloading Tacotron Models");
	delete sessionEncoder;
	delete sessionDecoderIter;
	delete sessionPostNet;
	delete sessionGan;
	sessionEncoder = nullptr;
	sessionDecoderIter = nullptr;
	sessionPostNet = nullptr;
	sessionGan = nullptr;
	logger.log(L"[Info] Tacotron Models unloaded");
}

Tacotron2::Tacotron2(const rapidjson::Document& _config, const callback& _cb, const callback_params& _mr, const DurationCallback& _dcbb, Device _dev)
{
	_modelType = modelType::Taco;

	ChangeDevice(_dev);

	//Check Folder
	if (_config["Folder"].IsNull())
		throw std::exception("[Error] Missing field \"folder\" (Model Folder)");
	if (!_config["Folder"].IsString())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Must Be String");
	const auto _folder = to_wide_string(_config["Folder"].GetString());
	if(_folder.empty())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Can Not Be Empty");
	const std::wstring _path = GetCurrentFolder() + L"\\Models\\" + _folder + L"\\" + _folder;

	//Check Hifigan
	if (!_config.HasMember("Hifigan") || _config["Hifigan"].IsNull())
		throw std::exception("[Error] Missing field \"Hifigan\" (Hifigan Model Name)");
	if (!_config["Hifigan"].IsString())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Model Name) Must Be String");
	const auto _hfolder = to_wide_string(_config["Hifigan"].GetString());
	if (_hfolder.empty())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Model Name) Can Not Be Empty");
	const std::wstring _HifiganPath = GetCurrentFolder() + L"\\hifigan\\" + _hfolder + L".onnx";

	//Check SamplingRate
	if (_config["Rate"].IsNull())
		throw std::exception("[Error] Missing field \"Rate\" (SamplingRate)");
	if (_config["Rate"].IsInt() || _config["Rate"].IsInt64())
		_samplingRate = _config["Rate"].GetInt();
	else
		throw std::exception("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64");

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	add_blank = false;
	//Check Symbol
	if (!_config.HasMember("Symbol") || _config["Symbol"].IsNull())
		throw std::exception("[Error] Missing field \"Symbol\" (PhSymbol)");
	if (_config.HasMember("AddBlank") && _config["AddBlank"].IsBool())
		add_blank = _config["AddBlank"].GetBool();
	else
		logger.log(L"[Warn] Field \"AddBlank\" Is Missing, Use Default Value");

	//Load Symbol
	int64_t iter = 0;
	if (_config["Symbol"].IsArray())
	{
		logger.log(L"[Info] Use Phs");
		if(_config["Symbol"].Empty())
			throw std::exception("[Error] Field \"Symbol\" (PhSymbol) Can Not Be Empty");
		for (const auto& it : _config["Symbol"].GetArray())
			_Phs.insert({ to_wide_string(it.GetString()), iter++ });
		use_ph = true;
	}
	else
	{
		if(!_config["Symbol"].IsString())
			throw std::exception("[Error] Field \"Symbol\" (PhSymbol) Must Be Array<String> or String");
		logger.log(L"[Info] Use Symbols");
		const std::wstring SymbolsStr = to_wide_string(_config["Symbol"].GetString());
		if (SymbolsStr.empty())
			throw std::exception("[Error] Field \"Symbol\" (PhSymbol) Can Not Be Empty");
		for (auto it : SymbolsStr)
			_Symbols.insert(std::pair<wchar_t, int64_t>(it, iter++));
		use_ph = false;
	}

	if(_config.HasMember("Cleaner") && _config["Cleaner"].IsString())
	{
		const auto Cleaner = to_wide_string(_config["Cleaner"].GetString());
		if (!Cleaner.empty())
			switch (_plugin.Load(Cleaner))
			{
			case (-1):
				{
					logger.log(L"[Error] Plugin File Does Not Exist");
					_plugin.unLoad();
					break;
				}
			case (1):
				{
					logger.log(L"[Error] Plugin Has Some Error");
					_plugin.unLoad();
					break;
				}
			default:
				{
					logger.log(L"[Info] Plugin Loaded");
					break;
				}
			}
		else
			logger.log(L"[Info] Disable Plugin");
	}
	else
		logger.log(L"[Info] Disable Plugin");

	_callback = _cb;
	_get_init_params = _mr;

	//LoadModels
	try
	{
		logger.log(L"[Info] loading Tacotron Models");
		sessionEncoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
		sessionDecoderIter = new Ort::Session(*env, (_path + L"_decoder_iter.onnx").c_str(), *session_options);
		sessionPostNet = new Ort::Session(*env, (_path + L"_postnet.onnx").c_str(), *session_options);
		logger.log(L"[Info] Tacotron Models loaded");

		logger.log(L"[Info] loading Hifigan Model");
		sessionGan = new Ort::Session(*env, _HifiganPath.c_str(), *session_options);
		logger.log(L"[Info] Hifigan Model loaded");
	}
	catch (Ort::Exception& _exception)
	{
		delete sessionEncoder;
		delete sessionDecoderIter;
		delete sessionPostNet;
		delete sessionGan;
		throw std::exception(_exception.what());
	}
}

std::vector<int16_t> Tacotron2::Inference(std::wstring& _inputLens) const
{
	if (_inputLens.length() == 0)
	{
		logger.log(L"[Warn] Empty Input Box");
		int ret = InsertMessageToEmptyEditBox(_inputLens);
		if(ret == -1)
			throw std::exception("TTS Does Not Support Automatic Completion");
		if(ret == -2)
			throw std::exception("Please Select Files");
	}
	_inputLens += L'\n';
	std::vector<std::wstring> _Lens = CutLens(_inputLens);
	auto _configs = GetParam(_Lens);
	size_t proc = 0;
	_callback(proc, _Lens.size());
	std::vector<int16_t> _wavData;
	_wavData.reserve(441000);
	logger.log(L"[Info] Inferring");
	for(const auto& _input : _Lens)
	{
		logger.log(L"[Inferring] Inferring \"" + _input + L'\"');
		if (_input.empty())
		{
			logger.log(L"[Inferring] Skip Empty Len");
			continue;
		}
		int64_t maxDecoderSteps = _configs[proc].maxDecoderSteps;
		float gateThreshold = _configs[proc].gateThreshold;
		//preprocess phs
		std::vector<int64_t> text;
		text.reserve(_input.length() * 4 + 4);
		if (!use_ph)
		{
			for (auto it : _input)
			{
				if (add_blank)
					text.push_back(0);
				text.push_back(_Symbols.at(it));
			}
			if (add_blank)
				text.push_back(0);
		}
		else
		{
			std::vector<std::wstring> textVec;
			textVec.reserve((_input.length() + 2) * 2);
			std::wstring _inputStrW = _input + L'|';
			while (!_inputStrW.empty())
			{
				const auto this_ph = _inputStrW.substr(0, _inputStrW.find(L'_'));
				if (add_blank)
					text.push_back(0);
				text.push_back(_Phs.at(this_ph));
				const auto idx = _inputStrW.find(L'|');
				if (idx != std::wstring::npos)
					_inputStrW = _inputStrW.substr(idx + 1);
			}
			if (add_blank)
				text.push_back(0);
		}
		int64 textLength[1] = { static_cast<int64>(_input.length()) };

		std::vector<MTensor> inputTensors;
		const int64 inputShape1[2] = { 1,textLength[0] };
		constexpr int64 inputShape2[1] = { 1 };
		inputTensors.push_back(MTensor::CreateTensor<int64>(
			*memory_info, text.data(), textLength[0], inputShape1, 2));
		inputTensors.push_back(MTensor::CreateTensor<int64>(
			*memory_info, textLength, 1, inputShape2, 1));
		logger.log(L"[Inferring] Inferring \"" + _input + L"\" Encoder");
		std::vector<MTensor> outputTensors = sessionEncoder->Run(Ort::RunOptions{ nullptr },
			inputNodeNamesSessionEncoder.data(),
			inputTensors.data(),
			inputTensors.size(),
			outputNodeNamesSessionEncoder.data(),
			outputNodeNamesSessionEncoder.size());
		std::vector<int64> shape1 = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
		std::vector<int64> shape2 = outputTensors[1].GetTensorTypeAndShapeInfo().GetShape();
		const std::vector<int64> attention_rnn_dim{ shape1[0],1024i64 };
		const std::vector<int64> decoder_rnn_dim{ shape1[0],1024i64 };
		const std::vector<int64> encoder_embedding_dim{ shape1[0],512i64 };
		const std::vector<int64> n_mel_channels{ shape1[0],80i64 };
		std::vector<int64> seqLen{ shape1[0],shape1[1] };
		std::vector<float> zero1(n_mel_channels[0] * n_mel_channels[1], 0.0);
		std::vector<float> zero2(attention_rnn_dim[0] * attention_rnn_dim[1], 0.0);
		std::vector<float> zero3(attention_rnn_dim[0] * attention_rnn_dim[1], 0.0);
		std::vector<float> zero4(decoder_rnn_dim[0] * decoder_rnn_dim[1], 0.0);
		std::vector<float> zero5(decoder_rnn_dim[0] * decoder_rnn_dim[1], 0.0);
		std::vector<float> zero6(seqLen[0] * seqLen[1], 0.0);
		std::vector<float> zero7(seqLen[0] * seqLen[1], 0.0);
		std::vector<float> zero8(encoder_embedding_dim[0] * encoder_embedding_dim[1], 0.0);
		inputTensors.clear();
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero1.data(), zero1.size(), n_mel_channels.data(), n_mel_channels.size()));
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero2.data(), zero2.size(), attention_rnn_dim.data(), attention_rnn_dim.size()));
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero3.data(), zero3.size(), attention_rnn_dim.data(), attention_rnn_dim.size()));
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero4.data(), zero4.size(), decoder_rnn_dim.data(), decoder_rnn_dim.size()));
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero5.data(), zero5.size(), decoder_rnn_dim.data(), decoder_rnn_dim.size()));
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero6.data(), zero6.size(), seqLen.data(), seqLen.size()));
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero7.data(), zero7.size(), seqLen.data(), seqLen.size()));
		inputTensors.push_back(MTensor::CreateTensor<float>(
			*memory_info, zero8.data(), zero8.size(), encoder_embedding_dim.data(), encoder_embedding_dim.size()));
		inputTensors.push_back(std::move(outputTensors[0]));
		inputTensors.push_back(std::move(outputTensors[1]));
		std::vector<char> BooleanTensor(seqLen[0] * seqLen[1], 0);
		//auto BooleanVec = new bool[seqLen[0] * seqLen[1]];
		//memset(BooleanVec, 0, seqLen[0] * seqLen[1] * sizeof(bool));
		inputTensors.push_back(MTensor::CreateTensor<bool>(
			*memory_info, reinterpret_cast<bool*>(BooleanTensor.data()), seqLen[0] * seqLen[1], seqLen.data(), seqLen.size()));
		int32_t notFinished = 1;
		int32_t melLengths = 0;
		std::vector<float> melGateAlig;
		std::vector<int64> melGateAligShape;
		bool firstIter = true;
		logger.log(L"[Inferring] Inferring \"" + _input + L"\" Decoder");
		while (true) {
			try
			{
				outputTensors = sessionDecoderIter->Run(Ort::RunOptions{ nullptr },
					inputNodeNamesSessionDecoderIter.data(),
					inputTensors.data(),
					inputTensors.size(),
					outputNodeNamesSessionDecoderIter.data(),
					outputNodeNamesSessionDecoderIter.size());
			}
			catch (Ort::Exception& e)
			{
				throw std::exception(e.what());
			}
			if (firstIter) {
				melGateAlig = { outputTensors[0].GetTensorMutableData<float>() ,outputTensors[0].GetTensorMutableData<float>() + outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount() };
				melGateAligShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
				melGateAligShape.push_back(1);
				firstIter = false;
			}
			else {
				cat(melGateAlig, melGateAligShape, outputTensors[0]);
			}
			auto sigs = outputTensors[1].GetTensorData<float>()[0];
			sigs = 1.0F / (1.0F + exp(0.0F - sigs));
			auto decInt = sigs <= gateThreshold;
			notFinished = decInt * notFinished;
			melLengths += notFinished;
			if (!notFinished) {
				break;
			}
			if (melGateAligShape[2] >= maxDecoderSteps) {
				logger.log(L"[Info] reach max decode steps");
				break;
			}
			inputTensors[0] = std::move(outputTensors[0]);
			inputTensors[1] = std::move(outputTensors[2]);
			inputTensors[2] = std::move(outputTensors[3]);
			inputTensors[3] = std::move(outputTensors[4]);
			inputTensors[4] = std::move(outputTensors[5]);
			inputTensors[5] = std::move(outputTensors[6]);
			inputTensors[6] = std::move(outputTensors[7]);
			inputTensors[7] = std::move(outputTensors[8]);
		}
		std::vector<MTensor> melInput;
		melInput.push_back(MTensor::CreateTensor<float>(
			*memory_info, melGateAlig.data(), melGateAligShape[0] * melGateAligShape[1] * melGateAligShape[2], melGateAligShape.data(), melGateAligShape.size()));
		logger.log(L"[Inferring] Inferring \"" + _input + L"\" PostNet");
		outputTensors = sessionPostNet->Run(Ort::RunOptions{ nullptr },
			inputNodeNamesSessionPostNet.data(),
			melInput.data(),
			melInput.size(),
			outputNodeNamesSessionPostNet.data(),
			outputNodeNamesSessionPostNet.size());
		logger.log(L"[Inferring] Inferring \"" + _input + L"\" VoCoder");
		const std::vector<MTensor> wavOuts = sessionGan->Run(Ort::RunOptions{ nullptr },
			ganIn.data(),
			outputTensors.data(),
			outputTensors.size(),
			ganOut.data(),
			ganOut.size());
		const std::vector<int64> wavOutsSharp = wavOuts[0].GetTensorTypeAndShapeInfo().GetShape();
		const auto outData = wavOuts[0].GetTensorData<float>();
		for (int i = 0; i < wavOutsSharp[2]; i++)
			_wavData.emplace_back(static_cast<int16_t>(outData[i] * 32768.0f));
		_callback(++proc, _Lens.size());
		logger.log(L"[Inferring] \"" + _input + L"\" Finished");
		BooleanTensor.clear();
	}
	logger.log(L"[Info] Finished");
	return _wavData;
}

std::vector<int16_t> Tacotron2::Inference(const MoeVSProject::TTSParams& _input) const
{
	std::vector<int16_t> _wavData;
	_wavData.reserve(441000);
	logger.log(L"[Inferring] Inferring \"" + _input.phs + L'\"');
	if (_input.phs.empty())
	{
		logger.log(L"[Inferring] Skip Empty Len");
		return {};
	}
	int64_t maxDecoderSteps = _input.decode_step;
	auto gateThreshold = float(_input.gate);
	//preprocess phs
	std::vector<int64_t> text;
	text.reserve(_input.phs.length() * 4 + 4);
	if (!use_ph)
	{
		for (auto it : _input.phs)
		{
			if (add_blank)
				text.push_back(0);
			text.push_back(_Symbols.at(it));
		}
		if (add_blank)
			text.push_back(0);
	}
	else
	{
		std::vector<std::wstring> textVec;
		textVec.reserve((_input.phs.length() + 2) * 2);
		std::wstring _inputStrW = _input.phs + L'|';
		while (!_inputStrW.empty())
		{
			const auto this_ph = _inputStrW.substr(0, _inputStrW.find(L'_'));
			if (add_blank)
				text.push_back(0);
			text.push_back(_Phs.at(this_ph));
			const auto idx = _inputStrW.find(L'|');
			if (idx != std::wstring::npos)
				_inputStrW = _inputStrW.substr(idx + 1);
		}
		if (add_blank)
			text.push_back(0);
	}
	int64 textLength[1] = { static_cast<int64>(_input.phs.length()) };

	std::vector<MTensor> inputTensors;
	const int64 inputShape1[2] = { 1,textLength[0] };
	constexpr int64 inputShape2[1] = { 1 };
	inputTensors.push_back(MTensor::CreateTensor<int64>(
		*memory_info, text.data(), textLength[0], inputShape1, 2));
	inputTensors.push_back(MTensor::CreateTensor<int64>(
		*memory_info, textLength, 1, inputShape2, 1));
	logger.log(L"[Inferring] Inferring \"" + _input.phs + L"\" Encoder");
	std::vector<MTensor> outputTensors = sessionEncoder->Run(Ort::RunOptions{ nullptr },
		inputNodeNamesSessionEncoder.data(),
		inputTensors.data(),
		inputTensors.size(),
		outputNodeNamesSessionEncoder.data(),
		outputNodeNamesSessionEncoder.size());
	std::vector<int64> shape1 = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
	std::vector<int64> shape2 = outputTensors[1].GetTensorTypeAndShapeInfo().GetShape();
	const std::vector<int64> attention_rnn_dim{ shape1[0],1024i64 };
	const std::vector<int64> decoder_rnn_dim{ shape1[0],1024i64 };
	const std::vector<int64> encoder_embedding_dim{ shape1[0],512i64 };
	const std::vector<int64> n_mel_channels{ shape1[0],80i64 };
	std::vector<int64> seqLen{ shape1[0],shape1[1] };
	std::vector<float> zero1(n_mel_channels[0] * n_mel_channels[1], 0.0);
	std::vector<float> zero2(attention_rnn_dim[0] * attention_rnn_dim[1], 0.0);
	std::vector<float> zero3(attention_rnn_dim[0] * attention_rnn_dim[1], 0.0);
	std::vector<float> zero4(decoder_rnn_dim[0] * decoder_rnn_dim[1], 0.0);
	std::vector<float> zero5(decoder_rnn_dim[0] * decoder_rnn_dim[1], 0.0);
	std::vector<float> zero6(seqLen[0] * seqLen[1], 0.0);
	std::vector<float> zero7(seqLen[0] * seqLen[1], 0.0);
	std::vector<float> zero8(encoder_embedding_dim[0] * encoder_embedding_dim[1], 0.0);
	inputTensors.clear();
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero1.data(), zero1.size(), n_mel_channels.data(), n_mel_channels.size()));
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero2.data(), zero2.size(), attention_rnn_dim.data(), attention_rnn_dim.size()));
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero3.data(), zero3.size(), attention_rnn_dim.data(), attention_rnn_dim.size()));
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero4.data(), zero4.size(), decoder_rnn_dim.data(), decoder_rnn_dim.size()));
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero5.data(), zero5.size(), decoder_rnn_dim.data(), decoder_rnn_dim.size()));
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero6.data(), zero6.size(), seqLen.data(), seqLen.size()));
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero7.data(), zero7.size(), seqLen.data(), seqLen.size()));
	inputTensors.push_back(MTensor::CreateTensor<float>(
		*memory_info, zero8.data(), zero8.size(), encoder_embedding_dim.data(), encoder_embedding_dim.size()));
	inputTensors.push_back(std::move(outputTensors[0]));
	inputTensors.push_back(std::move(outputTensors[1]));
	std::vector<char> BooleanTensor(seqLen[0] * seqLen[1], 0);
	inputTensors.push_back(MTensor::CreateTensor<bool>(
		*memory_info, reinterpret_cast<bool*>(BooleanTensor.data()), seqLen[0] * seqLen[1], seqLen.data(), seqLen.size()));
	int32_t notFinished = 1;
	int32_t melLengths = 0;
	std::vector<float> melGateAlig;
	std::vector<int64> melGateAligShape;
	bool firstIter = true;
	logger.log(L"[Inferring] Inferring \"" + _input.phs + L"\" Decoder");
	while (true) {
		try
		{
			outputTensors = sessionDecoderIter->Run(Ort::RunOptions{ nullptr },
				inputNodeNamesSessionDecoderIter.data(),
				inputTensors.data(),
				inputTensors.size(),
				outputNodeNamesSessionDecoderIter.data(),
				outputNodeNamesSessionDecoderIter.size());
		}
		catch (Ort::Exception& e)
		{
			throw std::exception(e.what());
		}
		if (firstIter) {
			melGateAlig = { outputTensors[0].GetTensorMutableData<float>() ,outputTensors[0].GetTensorMutableData<float>() + outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount() };
			melGateAligShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
			melGateAligShape.push_back(1);
			firstIter = false;
		}
		else {
			cat(melGateAlig, melGateAligShape, outputTensors[0]);
		}
		auto sigs = outputTensors[1].GetTensorData<float>()[0];
		sigs = 1.0F / (1.0F + exp(0.0F - sigs));
		auto decInt = sigs <= gateThreshold;
		notFinished = decInt * notFinished;
		melLengths += notFinished;
		if (!notFinished) {
			break;
		}
		if (melGateAligShape[2] >= maxDecoderSteps) {
			logger.log(L"[Info] reach max decode steps");
			break;
		}
		inputTensors[0] = std::move(outputTensors[0]);
		inputTensors[1] = std::move(outputTensors[2]);
		inputTensors[2] = std::move(outputTensors[3]);
		inputTensors[3] = std::move(outputTensors[4]);
		inputTensors[4] = std::move(outputTensors[5]);
		inputTensors[5] = std::move(outputTensors[6]);
		inputTensors[6] = std::move(outputTensors[7]);
		inputTensors[7] = std::move(outputTensors[8]);
	}
	std::vector<MTensor> melInput;
	melInput.push_back(MTensor::CreateTensor<float>(
		*memory_info, melGateAlig.data(), melGateAligShape[0] * melGateAligShape[1] * melGateAligShape[2], melGateAligShape.data(), melGateAligShape.size()));
	logger.log(L"[Inferring] Inferring \"" + _input.phs + L"\" PostNet");
	outputTensors = sessionPostNet->Run(Ort::RunOptions{ nullptr },
		inputNodeNamesSessionPostNet.data(),
		melInput.data(),
		melInput.size(),
		outputNodeNamesSessionPostNet.data(),
		outputNodeNamesSessionPostNet.size());
	logger.log(L"[Inferring] Inferring \"" + _input.phs + L"\" Vocoder");
	const std::vector<MTensor> wavOuts = sessionGan->Run(Ort::RunOptions{ nullptr },
		ganIn.data(),
		outputTensors.data(),
		outputTensors.size(),
		ganOut.data(),
		ganOut.size());
	const std::vector<int64> wavOutsSharp = wavOuts[0].GetTensorTypeAndShapeInfo().GetShape();
	const auto outData = wavOuts[0].GetTensorData<float>();
	for (int i = 0; i < wavOutsSharp[2]; i++)
		_wavData.emplace_back(static_cast<int16_t>(outData[i] * 32768.0f));
	logger.log(L"[Inferring] \"" + _input.phs + L"\" Finished");
	BooleanTensor.clear();
	return _wavData;
}
INFERCLASSEND