#include "../header/DiffSinger.hpp"
#include "../../InferTools/inferTools.hpp"
#include "../../../Lib/World/src/world/matlabfunctions.h"
#include <deque>
#include <random>

INFERCLASSHEADER
DiffusionSinger::~DiffusionSinger()
{
	logger.log(L"[Info] unloading DiffSinger Models");
	if (!diffSinger)
	{
		delete nsfHifigan;
		delete encoder;
		delete denoise;
		delete pred;
		delete after;
		nsfHifigan = nullptr;
		encoder = nullptr;
		denoise = nullptr;
		pred = nullptr;
		after = nullptr;
	}
	else
	{
		delete diffSinger;
		delete nsfHifigan;
		diffSinger = nullptr;
		nsfHifigan = nullptr;
	}
	logger.log(L"[Info] DiffSinger Models unloaded");
}

DiffusionSinger::DiffusionSinger(const MJson& _config, const callback& _cb, const callback_params& _mr, Device _dev)
{
	_modelType = modelType::diffSinger;

	ChangeDevice(_dev);

	//Check Folder
	if (_config["Folder"].IsNull())
		throw std::exception("[Error] Missing field \"folder\" (Model Folder)");
	if (!_config["Folder"].IsString())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Must Be String");
	const auto _folder = to_wide_string(_config["Folder"].GetString());
	if (_folder.empty())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Can Not Be Empty");
	const std::wstring _path = GetCurrentFolder() + L"\\Models\\" + _folder + L"\\" + _folder;

	if (_config["Hifigan"].IsNull())
		throw std::exception("[Error] Missing field \"Hifigan\" (Hifigan Folder)");
	if (!_config["Hifigan"].IsString())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Folder) Must Be String");
	const std::wstring HifiganPath = to_wide_string(_config["Hifigan"].GetString());
	if (HifiganPath.empty())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Folder) Can Not Be Empty");

	//LoadModels
	try
	{
		logger.log(L"[Info] loading DiffSinger Models");
		nsfHifigan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan\\" + HifiganPath + L".onnx").c_str(), *session_options);
		if (_waccess((_path + L"_DiffSinger.onnx").c_str(), 0) != -1)
			diffSinger = new Ort::Session(*env, (_path + L"_DiffSinger.onnx").c_str(), *session_options);
		else
		{
			encoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
			denoise = new Ort::Session(*env, (_path + L"_denoise.onnx").c_str(), *session_options);
			pred = new Ort::Session(*env, (_path + L"_pred.onnx").c_str(), *session_options);
			after = new Ort::Session(*env, (_path + L"_after.onnx").c_str(), *session_options);
		}
		logger.log(L"[Info] DiffSinger Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		throw std::exception(_exception.what());
	}

	//Check SamplingRate
	if (_config["Rate"].IsNull())
		throw std::exception("[Error] Missing field \"Rate\" (SamplingRate)");
	if (_config["Rate"].IsInt() || _config["Rate"].IsInt64())
		_samplingRate = _config["Rate"].GetInt();
	else
		throw std::exception("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64");

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	if (_config["MelBins"].IsNull())
		throw std::exception("[Error] Missing field \"MelBins\" (MelBins)");
	if (_config["MelBins"].IsInt() || _config["MelBins"].IsInt64())
		melBins = _config["MelBins"].GetInt();
	else
		throw std::exception("[Error] Field \"MelBins\" (MelBins) Must Be Int/Int64");

	if (!(_config["Hop"].IsInt() || _config["Hop"].IsInt64()))
		throw std::exception("[Error] Hop Must Be Int");
	hop = _config["Hop"].GetInt();

	if (hop < 1)
		throw std::exception("[Error] Hop Must > 0");

	if (!(_config["HiddenSize"].IsInt() || _config["HiddenSize"].IsInt64()))
		logger.log(L"[Warn] Missing Field \"HiddenSize\", Use Default Value (256)");
	else
		Hidden_Size = _config["HiddenSize"].GetInt();

	std::wstring dicFolder;

	if (_config["DicFolder"].IsString())
		dicFolder = to_wide_string(_config["DicFolder"].GetString());

	if(!dicFolder.empty())
		PhonesPair = GetPhonesPairMap(dicFolder);
	else
		PhonesPair = GetPhonesPairMap(_path + L".json");
	Phones = GetPhones(PhonesPair);

	_callback = _cb;
	_get_init_params = _mr;
}

std::map<std::wstring, std::vector<std::wstring>> DiffusionSinger::GetPhonesPairMap(const std::wstring& path)
{
	std::string phoneInfo, phoneInfoAll;
	std::ifstream phonefile(path.c_str());
	if (!phonefile.is_open())
		throw std::exception("phone file not found");
	while (std::getline(phonefile, phoneInfo))
		phoneInfoAll += phoneInfo;
	phonefile.close();
	MJson PhoneJson;
	PhoneJson.Parse(phoneInfoAll);
	if (PhoneJson.HasParseError())
		throw std::exception("json file error");
	std::map<std::wstring, std::vector<std::wstring>> TmpOut;
	for (const auto& itr : PhoneJson.GetMemberArray())
	{
		std::wstring Key = to_wide_string(itr.first);
		const auto Value = itr.second.GetArray();
		TmpOut[Key] = std::vector<std::wstring>();
		for (const auto& it : Value)
			TmpOut[Key].push_back(to_wide_string(it.GetString()));
	}
	return TmpOut;
}

std::map<std::wstring, int64_t> DiffusionSinger::GetPhones(const std::map<std::wstring, std::vector<std::wstring>>& PhonesPair)
{
	std::map<std::wstring, int64_t> tmpMap;

	for (const auto& it : PhonesPair)
		for (const auto& its : it.second)
			tmpMap[its] = 0;
	tmpMap[L"SP"] = 0;
	tmpMap[L"AP"] = 0;
	int64_t pos = 3;
	for (auto& it : tmpMap)
		it.second = pos++;
	tmpMap[L"PAD"] = 0;
	tmpMap[L"EOS"] = 1;
	tmpMap[L"UNK"] = 2;
	return tmpMap;
}

std::vector<int16_t> DiffusionSinger::Inference(std::wstring& _inputLens) const
{
	const auto params = _get_init_params();
	std::wstring RawPath;
	const int ret = InsertMessageToEmptyEditBox(RawPath);
	if (ret == -1)
		throw std::exception("TTS Does Not Support Automatic Completion");
	if (ret == -2)
		throw std::exception("Please Select Files");
	RawPath += L'\n';
	std::vector<std::wstring> _Lens = CutLens(RawPath);
	int64_t charEmb = params.chara;
	std::mt19937 gen(int(params.seed));
	std::normal_distribution<float> normal(0, 1);
	float noise_scale = params.noise_scale;
	int64 speedup = params.pndm;
	int64 step = params.step;
	if (step > 1000 || step < 30)
		step = 1000;
	if (speedup > step || speedup < 2)
		speedup = 2;
	auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;
	std::vector<int64> diffusionSteps;
	for (int64 itt = step - speedup; itt >= 0; itt -= speedup)
		diffusionSteps.push_back(itt);
	const auto tran = static_cast<int>(params.keys);
	const auto inputs = preprocessDiffSinger(_Lens);
	size_t fileIndex = 0;
	for(const auto& DSFile : inputs)
	{
		size_t proc = 0;
		std::vector<int16_t> _wavData;
		logger.log(L"[Inferring] Inferring \"" + _Lens[fileIndex] + L'\"');
		if (diffSinger)
		{
			_callback(0, DSFile.inputLens.size());
			for (size_t i = 0; i < DSFile.inputLens.size(); ++i)
			{
				const long long TokenShape[2] = { 1, static_cast<long long>(DSFile.inputLens[i].size()) };
				const long long F0Shape[2] = { 1, static_cast<long long>(DSFile.f0[i].size()) };
				//long long charData[1] = { chara };
				long long speed_up[1] = { speedup };
				constexpr long long speedShape[1] = { 1 };
				auto token = DSFile.inputLens[i];
				auto duration = DSFile.durations[i];
				auto f0 = DSFile.f0[i];
				for (size_t ita = 0; ita < f0.size(); ++ita)
					f0[ita] *= static_cast<float>(pow(2.0, static_cast<double>(tran) / 12.0));
				std::vector<MTensor> inputTensors;
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, token.data(), TokenShape[1], TokenShape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, duration.data(), TokenShape[1], TokenShape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, f0.data(), F0Shape[1], F0Shape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, speed_up, 1, speedShape, 1));

				std::vector<Ort::Value> DiffusionOut;
				try {
					DiffusionOut = diffSinger->Run(Ort::RunOptions{ nullptr },
						SingerInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						SingerOutput.data(),
						SingerOutput.size());
				}
				catch (Ort::Exception& e1)
				{
					throw std::exception((std::string("Locate: Diffusion\n") + e1.what()).c_str());
				}
				std::vector<Ort::Value> finaOut;
				std::vector<Ort::Value> ganTensors;
				ganTensors.emplace_back(std::move(DiffusionOut[0]));
				ganTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, f0.data(), F0Shape[1], F0Shape, 2));
				try
				{
					finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
						SingerHifiganInput.data(),
						ganTensors.data(),
						ganTensors.size(),
						SingerHifiganOutput.data(),
						SingerHifiganOutput.size());
				}
				catch (Ort::Exception& e3)
				{
					throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
				}
				const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<float> out(finaOut[0].GetTensorData<float>(), finaOut[0].GetTensorData<float>() + shapeOut[1]);
				std::vector<int16_t> TempVecWav(shapeOut[1], 0);
				for (int bbb = 0; bbb < shapeOut[1]; bbb++) {
					TempVecWav[bbb] = static_cast<int16_t>(out[bbb] * 32768.0f);
				}
				const auto silent_length = static_cast<int64_t>(round(DSFile.offset[i] * _samplingRate)) - static_cast<int64_t>(_wavData.size());
				if (silent_length > 0)
				{
					std::vector<int16_t> iwav(silent_length, 0);
					_wavData.insert(_wavData.end(), iwav.data(), iwav.data() + iwav.size());
					_wavData.insert(_wavData.end(), TempVecWav.data(), TempVecWav.data() + shapeOut[1]);
				}
				else
				{
					const int64_t idx = silent_length + static_cast<int64_t>(_wavData.size());
					const auto rsize = static_cast<int64_t>(_wavData.size());
					const int64_t fade_len = static_cast<int64_t>(_wavData.size()) - idx;
					const auto fade = linspace(fade_len);
					int64_t ids = idx;
					while (ids != rsize)
					{
						_wavData[ids] = static_cast<short>((1.0 - fade[ids - idx]) * static_cast<double>(_wavData[ids]) + fade[ids - idx] * static_cast<double>(TempVecWav[ids - idx]));
						++ids;
					}
					_wavData.insert(_wavData.end(), TempVecWav.data() + fade_len, TempVecWav.data() + shapeOut[1]);
				}
				_callback(++proc, DSFile.inputLens.size());
			}
		}
		else
		{
			const auto maxProc = DSFile.inputLens.size() * RealDiffSteps;
			_callback(0, maxProc);
			for (size_t i = 0; i < DSFile.inputLens.size(); ++i)
			{
				const long long TokenShape[2] = { 1, static_cast<long long>(DSFile.inputLens[i].size()) };
				const long long F0Shape[2] = { 1, static_cast<long long>(DSFile.f0[i].size()) };
				long long charData[1] = { charEmb };
				constexpr long long charShape[1] = { 1 };
				auto token = DSFile.inputLens[i];
				auto duration = DSFile.durations[i];
				auto f0 = DSFile.f0[i];
				for (size_t ita = 0; ita < f0.size(); ++ita)
					f0[ita] *= static_cast<float>(pow(2.0, static_cast<double>(tran) / 12.0));
				std::vector<MTensor> inputTensors;
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, token.data(), TokenShape[1], TokenShape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, duration.data(), TokenShape[1], TokenShape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, f0.data(), F0Shape[1], F0Shape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, charData, 1, charShape, 1));

				std::vector<Ort::Value> encTensors;
				try {
					encTensors = encoder->Run(Ort::RunOptions{ nullptr },
						SingerEncoderInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						SingerEncoderOutput.data(),
						SingerEncoderOutput.size());
				}
				catch (Ort::Exception& e1)
				{
					throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
				}

				std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
				for (auto& it : initial_noise)
					it = normal(gen);
				long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };

				//Diffusion
				std::vector<Ort::Value> DenoiseIn;
				std::vector<Ort::Value> DenoiseOut;
				std::vector<Ort::Value> PredIn;
				std::vector<Ort::Value> PredOut;
				std::vector<Ort::Value> DiffOut;
				std::vector<Ort::Value> finaOut;
				std::deque<std::vector<float>> noiseList;
				int64 timeshape[1] = { 1 };
				for (const auto& t : diffusionSteps)
				{
					int64 time[1] = { t };
					int64 time_prev[1] = { t - speedup > 0 ? t - speedup : 0 };
					if (noiseList.empty())
					{
						DenoiseIn.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4)); // noise  DenoiseIn[0]
						DenoiseIn.emplace_back(Ort::Value::CreateTensor(*memory_info, time,
							1, timeshape, 1)); // time  DenoiseIn[1]
						DenoiseIn.emplace_back(std::move(encTensors[0])); // condition  DenoiseIn[2]
						try
						{
							DenoiseOut = denoise->Run(Ort::RunOptions{ nullptr },
								denoiseInput.data(),
								DenoiseIn.data(),
								DenoiseIn.size(),
								denoiseOutput.data(),
								denoiseOutput.size());
						}
						catch (Ort::Exception& e1)
						{
							throw std::exception((std::string("Locate: denoise\n") + e1.what()).c_str());
						}

						noiseList.emplace_back(std::vector<float>(DenoiseOut[0].GetTensorData<float>(),
							DenoiseOut[0].GetTensorData<float>() +
							DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount())); // NoiseListExpand

						PredIn.emplace_back(std::move(DenoiseIn[0])); // noise  PredIn[0]
						PredIn.emplace_back(std::move(DenoiseOut[0])); // noise_pred  PredIn[1]
						PredIn.emplace_back(std::move(DenoiseIn[1])); // time  PredIn[2]
						PredIn.emplace_back(Ort::Value::CreateTensor(*memory_info, time_prev,
							1, timeshape, 1)); // time_prev  PredIn[3]
						try
						{
							PredOut = pred->Run(Ort::RunOptions{ nullptr },
								predInput.data(),
								PredIn.data(),
								PredIn.size(),
								predOutput.data(),
								predOutput.size());
						}
						catch (Ort::Exception& e1)
						{
							throw std::exception((std::string("Locate: pred\n") + e1.what()).c_str());
						}
						DenoiseIn[0] = std::move(PredOut[0]); // x_pred
						DenoiseIn[1] = std::move(PredIn[3]); // time_prev
						//DenoiseIn[2] = std::move(DenoiseIn[2]); // condition
						try
						{
							DenoiseOut = denoise->Run(Ort::RunOptions{ nullptr },
								denoiseInput.data(),
								DenoiseIn.data(),
								DenoiseIn.size(),
								denoiseOutput.data(),
								denoiseOutput.size());
						}
						catch (Ort::Exception& e1)
						{
							throw std::exception((std::string("Locate: denoise\n") + e1.what()).c_str());
						}
						auto noise_pred_prev = DenoiseOut[0].GetTensorMutableData<float>();
						for (const auto it : noiseList[0])
							(*(noise_pred_prev++) += it) /= 2.0f;
						//PredIn[0] = std::move(PredIn[0]); // noise
						PredIn[1] = std::move(DenoiseOut[0]); // noise_pred_prime
						//PredIn[2] = std::move(PredIn[2]); // time
						PredIn[3] = std::move(DenoiseIn[1]); // time_prev
					}
					else
					{
						DenoiseIn[0] = std::move(PredOut[0]); // x
						DenoiseIn[1] = Ort::Value::CreateTensor(*memory_info, time, 1,
							timeshape, 1); // time
						//DenoiseIn[2] = std::move(DenoiseIn[2]); // condition
						try
						{
							DenoiseOut = denoise->Run(Ort::RunOptions{ nullptr },
								denoiseInput.data(),
								DenoiseIn.data(),
								DenoiseIn.size(),
								denoiseOutput.data(),
								denoiseOutput.size());
						}
						catch (Ort::Exception& e1)
						{
							throw std::exception((std::string("Locate: denoise\n") + e1.what()).c_str());
						}
						if (noiseList.size() < 4)
							noiseList.emplace_back(std::vector<float>(DenoiseOut[0].GetTensorData<float>(), DenoiseOut[0].GetTensorData<float>() + DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount()));
						else
						{
							noiseList.pop_front();
							noiseList.emplace_back(std::vector<float>(DenoiseOut[0].GetTensorData<float>(), DenoiseOut[0].GetTensorData<float>() + DenoiseOut[0].GetTensorTypeAndShapeInfo().GetElementCount()));
						}
						auto noise_pred_prime = DenoiseOut[0].GetTensorMutableData<float>();
						if (noiseList.size() == 2)
							for (size_t it = 0; it < noiseList[0].size(); ++it)
								((*(noise_pred_prime++) *= 3.0f) -= noiseList[0][it]) /= 2.0f;
						if (noiseList.size() == 3)
							for (size_t it = 0; it < noiseList[0].size(); ++it)
								(((*(noise_pred_prime++) *= 23.0f) -= noiseList[1][it] * 16.0f) += noiseList[0][it] * 5.0f) /= 12.0f;
						if (noiseList.size() == 4)
							for (size_t it = 0; it < noiseList[0].size(); ++it)
								((((*(noise_pred_prime++) *= 55.0f) -= noiseList[2][it] * 59.0f) += noiseList[1][it] * 37.0f) -= noiseList[0][it] * 9.0f) /= 24.0f;
						PredIn[0] = std::move(DenoiseIn[0]); // x
						PredIn[1] = std::move(DenoiseOut[0]); // noise_pred_prime
						PredIn[2] = std::move(DenoiseIn[1]); // time
						PredIn[3] = Ort::Value::CreateTensor(*memory_info, time_prev, 1, timeshape, 1);
					}
					try
					{
						PredOut = pred->Run(Ort::RunOptions{ nullptr },
							predInput.data(),
							PredIn.data(),
							PredIn.size(),
							predOutput.data(),
							predOutput.size());
					}
					catch (Ort::Exception& e1)
					{
						throw std::exception((std::string("Locate: pred\n") + e1.what()).c_str());
					}
					_callback(++proc, maxProc);
				}
				try
				{
					DiffOut = after->Run(Ort::RunOptions{ nullptr },
						afterInput.data(),
						PredOut.data(),
						PredOut.size(),
						afterOutput.data(),
						afterOutput.size());
				}
				catch (Ort::Exception& e1)
				{
					throw std::exception((std::string("Locate: pred\n") + e1.what()).c_str());
				}
				std::vector<Ort::Value> ganTensors;
				ganTensors.emplace_back(std::move(DiffOut[0]));
				ganTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, f0.data(), F0Shape[1], F0Shape, 2));
				try
				{
					finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
						SingerHifiganInput.data(),
						ganTensors.data(),
						ganTensors.size(),
						SingerHifiganOutput.data(),
						SingerHifiganOutput.size());
				}
				catch (Ort::Exception& e3)
				{
					throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
				}
				const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<float> out(finaOut[0].GetTensorData<float>(), finaOut[0].GetTensorData<float>() + shapeOut[1]);
				std::vector<int16_t> TempVecWav(shapeOut[1], 0);
				for (int bbb = 0; bbb < shapeOut[1]; bbb++) {
					TempVecWav[bbb] = static_cast<int16_t>(out[bbb] * 32768.0f);
				}
				const auto silent_length = static_cast<int64_t>(round(DSFile.offset[i] * _samplingRate)) - static_cast<int64_t>(_wavData.size());
				if (silent_length > 0)
				{
					std::vector<int16_t> iwav(silent_length, 0);
					_wavData.insert(_wavData.end(), iwav.data(), iwav.data() + iwav.size());
					_wavData.insert(_wavData.end(), TempVecWav.data(), TempVecWav.data() + shapeOut[1]);
				}
				else
				{
					const int64_t idx = silent_length + static_cast<int64_t>(_wavData.size());
					const auto rsize = static_cast<int64_t>(_wavData.size());
					int64_t fade_len = static_cast<int64_t>(_wavData.size()) - idx;
					const auto fade = linspace(fade_len);
					int64_t ids = idx;
					while (ids != rsize)
					{
						_wavData[ids] = static_cast<short>((1.0 - fade[ids - idx]) * static_cast<double>(_wavData[ids]) + fade[ids - idx] * static_cast<double>(TempVecWav[ids - idx]));
						++ids;
					}
					_wavData.insert(_wavData.end(), TempVecWav.data() + fade_len, TempVecWav.data() + shapeOut[1]);
				}
			}
		}
		logger.log(L"[Inferring] \"" + _Lens[fileIndex] + L"\" Finished");
		if (inputs.size() == 1)
		{
			logger.log(L"[Info] Finished, Send To FrontEnd");
			return _wavData;
		}
		std::wstring outPutPath = GetCurrentFolder() + L"\\OutPuts\\" + _Lens[fileIndex].substr(_Lens[fileIndex].rfind(L'\\') + 1, _Lens[fileIndex].rfind(L'.')) + L'-' + std::to_wstring(uint64_t(_Lens[fileIndex].data())) + L".wav";
		logger.log(L"[Inferring] Write To \"" + outPutPath + L'\"');
		Wav(_samplingRate, long(_wavData.size()) * 2, _wavData.data()).Writef(outPutPath);
		++fileIndex;
	}
	logger.log(L"[Info] Finished");
	return {};
}

std::vector<DiffusionSinger::DiffSingerInput> DiffusionSinger::preprocessDiffSinger(const std::vector<std::wstring>& Jsonpath) const
{
	std::vector<DiffSingerInput> OutS;
	OutS.reserve(Jsonpath.size() + 1);
	for (const auto& itPath : Jsonpath)
	{
		std::string phoneInfo, phoneInfoAll;
		std::ifstream phonefile(itPath.c_str());
		if (!phonefile.is_open())
			throw std::exception("phone file not found");
		while (std::getline(phonefile, phoneInfo))
			phoneInfoAll += phoneInfo;
		phonefile.close();
		MJson PhoneJson;
		PhoneJson.Parse(phoneInfoAll);
		if (PhoneJson.HasParseError())
			throw std::exception("json file error");

		DiffSingerInput tmpOut;
		try
		{
			std::wsmatch match_results;
			std::smatch amatch_results;
			std::smatch pitch_results;
			for (auto& it : PhoneJson.GetArray())
			{
				if (it["input_type"].Empty() || it["offset"].IsNull())
					throw std::exception("mis input_type or offset");
				std::string input_type = it["input_type"].GetString();
				size_t reqLen = 0;
				const double frame_length = static_cast<double>(hop) / static_cast<double>(_samplingRate);


				// Tokens Preprocess
				std::wstring rtext;
				std::string phone;
				std::vector<int64_t> Tokens;
				if (input_type == "text")
				{
					throw std::exception("TODO");
					/*
					if (it["text"].Empty())
						throw std::exception("error");
					auto text = std::regex_replace(to_wide_string(it["text"].GetString()), std::wregex(L"SP"), L",");
					text = std::regex_replace(text, std::wregex(L"AP"), L"!");
					text = pluginApi.functionAPI(text);
					text = std::regex_replace(text, std::wregex(L"!"), L"AP");
					text = std::regex_replace(text, std::wregex(L","), L"SP");
					while (std::regex_search(text, match_results, tokenReg))
					{
						if (PhonesPair.find(match_results[0].str()) != PhonesPair.end())
							for (const auto& its : PhonesPair[match_results[0]])
								Tokens.push_back(Phones[its]);
						else
							Tokens.push_back(Phones[match_results[0].str()]);
						text = match_results.suffix();
					}
					 */
				}
				else
				{
					if (it["ph_seq"].Empty())
						throw std::exception("Missing ph_seq");
					if (it["ph_dur"].Empty())
						throw std::exception("Missing ph_dur");
					if (!it["tran_method"].Empty())
					{
						auto pttxt = to_wide_string(it["ph_seq"].GetString()) + L"<<<<DURATION>>>>" + to_wide_string(it["ph_dur"].GetString());
						if (std::string(it["tran_method"].GetString()) == "ChineseToJapanese")
							pttxt = tranTokens::ChineseToJapanese(pttxt);
						else if (std::string(it["tran_method"].GetString()) == "JapaneseToChinese")
							pttxt = tranTokens::JapaneseToChinese(pttxt);
						rtext = pttxt.substr(0, pttxt.find(L"<<<<DURATION>>>>") - 1);
						phone = to_byte_string(pttxt.substr(pttxt.find(L"<<<<DURATION>>>>") + 16));
					}
					std::wstring text = rtext;
					if (text.empty())
						text = to_wide_string(it["ph_seq"].GetString());
					while (std::regex_search(text, match_results, tokenReg))
					{
						if (PhonesPair.find(match_results[0].str()) != PhonesPair.end())
							for (const auto& its : PhonesPair.at(match_results[0]))
								Tokens.push_back(Phones.at(its));
						else if (Phones.find(match_results[0].str()) != Phones.end())
							Tokens.push_back(Phones.at(match_results[0].str()));
						else
							throw std::exception(("Unsupported Ph, In Len:" + std::string(it["ph_seq"].GetString())).c_str());
						text = match_results.suffix();
					}
				}
				tmpOut.offset.push_back(it["offset"].GetDouble());


				// Duration Preprocess
				if (!it["ph_dur"].Empty())
				{
					std::vector<double> ph_dur;
					ph_dur.reserve(Tokens.size());
					std::string phstr = phone;
					if (phstr.empty())
						phstr = it["ph_dur"].GetString();
					while (std::regex_search(phstr, amatch_results, numReg))
					{
						ph_dur.push_back(atof(amatch_results[0].str().c_str()));
						phstr = amatch_results.suffix();
					}
					if (ph_dur.size() != Tokens.size())
						throw std::exception("size mismatch ph_dur & Tokens");
					std::vector<int64_t> duration(ph_dur.size(), 0);
					for (size_t i = 1; i < ph_dur.size(); ++i)
						ph_dur[i] += ph_dur[i - 1];
					for (size_t i = 0; i < ph_dur.size(); ++i)
						ph_dur[i] = round((ph_dur[i] / frame_length) + 0.5);
					for (size_t i = ph_dur.size() - 1; i; --i)
						reqLen += (duration[i] = static_cast<int64_t>(ph_dur[i] - ph_dur[i - 1]));
					duration[0] = static_cast<int64_t>(ph_dur[0]);
					reqLen += duration[0];

					tmpOut.durations.push_back(std::move(duration));
				}
				else
					throw std::exception("Durations Could Not Be Empty");


				// note Preprocess
				if (MidiVer)
				{
					if (!it["note_seq"].Empty())
					{
						std::vector<int64_t> note_seq;
						note_seq.reserve(Tokens.size());
						std::string notestr = it["note_seq"].GetString();
						while (std::regex_search(notestr, amatch_results, pitchReg))
						{
							auto pitchString = amatch_results[0].str();
							int64_t pitch = atoll(pitchString.c_str());
							if (!pitch)
							{
								if (std::regex_search(pitchString, pitch_results, noteReg))
									pitch = 12 + midiPitch.at(pitch_results[1].str()) + 12 * atoll(pitch_results[2].str().c_str());
								else
									pitch = 0;
							}
							note_seq.push_back(pitch);
							notestr = amatch_results.suffix();
						}
						if (note_seq.size() != Tokens.size())
							throw std::exception("size mismatch note_seq & Tokens");
						tmpOut.pitchs.push_back(std::move(note_seq));
					}
					if (!it["note_dur_seq"].Empty())
					{
						std::vector<int64_t> note_duration(Tokens.size(), 0);
						std::vector<double> note_dur_seq;
						std::string notestr = it["note_dur_seq"].GetString();
						while (std::regex_search(notestr, amatch_results, numReg))
						{
							note_dur_seq.push_back(atof(amatch_results[0].str().c_str()));
							notestr = amatch_results.suffix();
						}
						if (note_dur_seq.size() != Tokens.size())
							throw std::exception("size mismatch note_dur_seq & Tokens");
						for (size_t i = 1; i < note_dur_seq.size(); ++i)
							note_dur_seq[i] += note_dur_seq[i - 1];
						for (size_t i = 0; i < note_dur_seq.size(); ++i)
							note_dur_seq[i] = round((note_dur_seq[i] / frame_length) + 0.5);
						for (size_t i = note_dur_seq.size() - 1; i; --i)
							note_duration[i] = static_cast<int64_t>(note_dur_seq[i] - note_dur_seq[i - 1]);
						note_duration[0] = static_cast<int64_t>(note_dur_seq[0]);
						tmpOut.pitch_durations.push_back(std::move(note_duration));
					}
				}
				else
				{
					//f0 Preprocess
					if (!it["f0_seq"].Empty())
					{
						if (it["f0_timestep"].IsNull())
							throw std::exception("mis f0_timestep");
						std::vector<double> f0_seq;
						std::string f0str = it["f0_seq"].GetString();
						while (std::regex_search(f0str, amatch_results, numReg))
						{
							f0_seq.push_back(atof(amatch_results[0].str().c_str()));
							f0str = amatch_results.suffix();
						}
						double f0_timestep = atof(it["f0_timestep"].GetString().c_str());
						const double t_max = static_cast<double>(f0_seq.size() - 1) * f0_timestep;
						const auto x0 = arange(0.0, static_cast<double>(f0_seq.size()), 1.0, (1.0 / f0_timestep));
						auto xi = arange(0.0, t_max, frame_length);
						while (xi.size() < reqLen)
							xi.push_back(*(xi.end() - 1) + f0_timestep);
						while (xi.size() > reqLen)
							xi.pop_back();
						std::vector<double> yi(xi.size(), 0.0);
						interp1(x0.data(), f0_seq.data(), static_cast<int>(x0.size()), xi.data(), static_cast<int>(xi.size()), yi.data());
						std::vector<float> f0(xi.size(), 0.0);
						for (size_t i = 0; i < yi.size(); ++i)
							f0[i] = static_cast<float>(yi[i]);
						tmpOut.f0.push_back(std::move(f0));
					}
					else
						throw std::exception("F0 Could Not Be Empty");
				}

				tmpOut.inputLens.push_back(std::move(Tokens));
			}
		}
		catch (std::exception& e)
		{
			throw std::exception(e.what());
		}
		OutS.push_back(std::move(tmpOut));
	}
	return OutS;
}
INFERCLASSEND