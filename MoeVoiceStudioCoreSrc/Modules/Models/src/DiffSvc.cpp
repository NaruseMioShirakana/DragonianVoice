#include "../header/DiffSvc.hpp"

#include <deque>

#include "../../AvCodec/AvCodeResample.h"
#include <random>

INFERCLASSHEADER
DiffusionSvc::~DiffusionSvc()
{
	logger.log(L"[Info] unloading DiffSvc Models");
	if(V2)
	{
		delete hubert;
		delete nsfHifigan;
		delete encoder;
		delete denoise;
		delete pred;
		delete after;
		hubert = nullptr;
		nsfHifigan = nullptr;
		encoder = nullptr;
		denoise = nullptr;
		pred = nullptr;
		after = nullptr;
	}
	else
	{
		delete hubert;
		delete diffSvc;
		delete nsfHifigan;
		hubert = nullptr;
		diffSvc = nullptr;
		nsfHifigan = nullptr;
	}
	logger.log(L"[Info] DiffSvc Models unloaded");
}

DiffusionSvc::DiffusionSvc(const MJson& _config, const callback& _cb, const callback_params& _mr, Device _dev)
{
	_modelType = modelType::diffSvc;

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

	if (_config["Hubert"].IsNull())
		throw std::exception("[Error] Missing field \"Hubert\" (Hubert Folder)");
	if (!_config["Hubert"].IsString())
		throw std::exception("[Error] Field \"Hubert\" (Hubert Folder) Must Be String");
	const std::wstring HuPath = to_wide_string(_config["Hubert"].GetString());
	if (HuPath.empty())
		throw std::exception("[Error] Field \"Hubert\" (Hubert Folder) Can Not Be Empty");

	if (_config["Hifigan"].IsNull())
		throw std::exception("[Error] Missing field \"Hifigan\" (Hifigan Folder)");
	if (!_config["Hifigan"].IsString())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Folder) Must Be String");
	const std::wstring HifiganPath = to_wide_string(_config["Hifigan"].GetString());
	if (HifiganPath.empty())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Folder) Can Not Be Empty");

	if (_waccess((_path + L"_encoder.onnx").c_str(), 0) != -1)
		V2 = true;

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

	if(!V2)
	{
		if (_config["Pndm"].IsNull())
			throw std::exception("[Error] Missing field \"Pndm\" (Pndm)");
		pndm = _config["Pndm"].GetInt();
	}

	if (!(_config["Hop"].IsInt() || _config["Hop"].IsInt64()))
		throw std::exception("[Error] Hop Must Be Int");
	hop = _config["Hop"].GetInt();

	if (hop < 1)
		throw std::exception("[Error] Hop Must > 0");

	if (!(_config["HiddenSize"].IsInt() || _config["HiddenSize"].IsInt64()))
		logger.log(L"[Warn] Missing Field \"HiddenSize\", Use Default Value (256)");
	else
		Hidden_Size = _config["HiddenSize"].GetInt();

	if (_config["Characters"].IsArray())
		n_speaker = _config["Characters"].Size();

	if (_config["Volume"].IsBool())
		VolumeB = _config["Volume"].GetBool();
	else
		logger.log(L"[Warn] Missing Field \"Volume\", Use Default Value (False)");

	if (!_config["CharaMix"].IsBool())
		logger.log(L"[Warn] Missing Field \"CharaMix\", Use Default Value (False)");
	else
		CharaMix = _config["CharaMix"].GetBool();

	if (!_config["Diffusion"].IsBool())
		logger.log(L"[Warn] Missing Field \"Diffusion\", Use Default Value (False)");
	else
		ddsp = _config["Diffusion"].GetBool();

	//LoadModels
	try
	{
		logger.log(L"[Info] loading DiffSvc Models");
		hubert = new Ort::Session(*env, (GetCurrentFolder() + L"\\hubert\\" + HuPath + L".onnx").c_str(), *session_options);
		nsfHifigan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan\\" + HifiganPath + L".onnx").c_str(), *session_options);
		if (_waccess((_path + L"_encoder.onnx").c_str(), 0) != -1)
		{
			encoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
			denoise = new Ort::Session(*env, (_path + L"_denoise.onnx").c_str(), *session_options);
			pred = new Ort::Session(*env, (_path + L"_pred.onnx").c_str(), *session_options);
			after = new Ort::Session(*env, (_path + L"_after.onnx").c_str(), *session_options);
			V2 = true;
		}
		else
			diffSvc = new Ort::Session(*env, (_path + L"_DiffSvc.onnx").c_str(), *session_options);
		logger.log(L"[Info] DiffSvc Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		delete hubert;
		delete nsfHifigan;
		delete encoder;
		delete denoise;
		delete pred;
		delete after;
		delete diffSvc;
		hubert = nullptr;
		nsfHifigan = nullptr;
		encoder = nullptr;
		denoise = nullptr;
		pred = nullptr;
		after = nullptr;
		diffSvc = nullptr;
		throw std::exception(_exception.what());
	}

	_callback = _cb;
	_get_init_params = _mr;
}

std::vector<int16_t> DiffusionSvc::InferWithF0AndHiddenUnit(std::vector<MoeVSProject::Params>& Inputs) const
{
	// Hidden_Unit -> Shape -> [audio, slice]
	const auto params = _get_init_params();
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

	const auto total_audio_count = Inputs.size();
	for (auto& inp_audio : Inputs)
	{
		if (!inp_audio.paths.empty())
			logger.log(L"[Inferring] Inferring \"" + inp_audio.paths + L'\"');
		size_t proc = 0;
		const auto Total_Slice_Count = inp_audio.Hidden_Unit.size();
		if (V2)
			_callback(proc, Total_Slice_Count * RealDiffSteps);
		else
			_callback(proc, Total_Slice_Count);
		logger.log(L"[Inferring] Inferring \"" + inp_audio.paths + L"\" Svc");

		std::vector<int16_t> _data;
		size_t total_audio_size = 0;
		for (const auto& data_size : inp_audio.OrgLen)
			total_audio_size += data_size;
		_data.reserve(size_t(double(total_audio_size) * 1.5));
		for (size_t slice = 0; slice < Total_Slice_Count; ++slice)
		{
			if (inp_audio.symbolb[slice])
			{
				const auto HubertSize = inp_audio.Hidden_Unit[slice].size();
				const auto HubertLen = int64_t(HubertSize) / Hidden_Size;
				const int64_t F0Shape[] = { 1, int64_t(inp_audio.F0[slice].size()) };
				const int64_t HiddenUnitShape[] = { 1, HubertLen, Hidden_Size };
				constexpr int64_t CharaEmbShape[] = { 1 };
				int64_t CharaMixShape[] = { F0Shape[1],n_speaker };

				std::vector<float> InterpedF0;
				std::vector<int64_t> alignment;
				int64_t Chara[] = { charEmb };

				std::vector<Ort::Value> finaOut;

				if(V2)
				{
					InterpedF0 = GetInterpedF0log(inp_audio.F0[slice]);
					alignment = GetAligments(F0Shape[1], HubertLen);
					std::vector<Ort::Value> inputTensors;
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, inp_audio.Hidden_Unit[slice].data(), HubertSize, HiddenUnitShape, 3));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
					if(!ddsp && !CharaMix)
						inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Chara, 1, CharaEmbShape, 1));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));
					std::vector<Ort::Value> encTensors;
					if(ddsp)
					{
						if(VolumeB)
							inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, inp_audio.Volume[slice].data(), F0Shape[1], F0Shape, 2));
						std::vector<float> chara_mix;
						if(CharaMix)
						{
							CharaMixShape[0] = F0Shape[1];
							if (!inp_audio.Speaker[slice].empty())
								chara_mix = inp_audio.Speaker[slice];
							else
							{
								std::vector<float> charaMap(n_speaker, 0.f);
								charaMap[params.chara] = 1.f;
								//std::vector<float>(n_speaker * CharaMixShape[0], 1.f / float(n_speaker));
								chara_mix.reserve((n_speaker + 1) * F0Shape[1]);
								for (int64_t index = 0; index < F0Shape[1]; ++index)
									chara_mix.insert(chara_mix.end(), charaMap.begin(), charaMap.end());
							}
							inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, chara_mix.data(), chara_mix.size(), CharaMixShape, 2));
						}
						try {
							encTensors = encoder->Run(Ort::RunOptions{ nullptr },
								encoderInputddsp.data(),
								inputTensors.data(),
								inputTensors.size(),
								encoderOutputDDSP.data(),
								encoderOutputDDSP.size());
						}
						catch (Ort::Exception& e1)
						{
							throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
						}
						encTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));
					}
					else
					{
						auto SvcInputNames = encoderInput;
						std::vector<float> chara_mix;
						if (CharaMix)
						{
							SvcInputNames = encoderInputSpkMix;
							CharaMixShape[0] = F0Shape[1];
							if (!inp_audio.Speaker[slice].empty())
								chara_mix = inp_audio.Speaker[slice];
							else
							{
								std::vector<float> charaMap(n_speaker, 0.f);
								charaMap[params.chara] = 1.f;
								//std::vector<float>(n_speaker * CharaMixShape[0], 1.f / float(n_speaker));
								chara_mix.reserve((n_speaker + 1) * F0Shape[1]);
								for (int64_t index = 0; index < F0Shape[1]; ++index)
									chara_mix.insert(chara_mix.end(), charaMap.begin(), charaMap.end());
							}
							inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, chara_mix.data(), chara_mix.size(), CharaMixShape, 2));
						}
						if (VolumeB)
						{
							inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, inp_audio.Volume[slice].data(), F0Shape[1], F0Shape, 2));
							SvcInputNames.emplace_back("volume");
						}
						try {
							encTensors = encoder->Run(Ort::RunOptions{ nullptr },
								SvcInputNames.data(),
								inputTensors.data(),
								inputTensors.size(),
								encoderOutput.data(),
								encoderOutput.size());
						}
						catch (Ort::Exception& e1)
						{
							throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
						}
					}

					//Diffusion
					std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
					for (auto& it : initial_noise)
						it = normal(gen) * noise_scale;
					long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };

					std::vector<Ort::Value> DenoiseIn;
					std::vector<Ort::Value> DenoiseOut;
					std::vector<Ort::Value> PredIn;
					std::vector<Ort::Value> PredOut;
					std::vector<Ort::Value> DiffOut;
					std::deque<std::vector<float>> noiseList;
					int64 timeshape[1] = { 1 }; // PLMS SAMPLING
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
						_callback(++proc, Total_Slice_Count* RealDiffSteps);
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
					ganTensors.emplace_back(std::move(encTensors[1]));
					try
					{
						finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
							nsfInput.data(),
							ganTensors.data(),
							ganTensors.size(),
							nsfOutput.data(),
							nsfOutput.size());
					}
					catch (Ort::Exception& e3)
					{
						throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
					}
				}
				else
				{
					int64_t speedData[] = { pndm };
					InterpedF0 = GetInterpedF0log(inp_audio.F0[slice]);
					alignment = GetAligments(F0Shape[1], HubertLen);
					std::vector<Ort::Value> inputTensors;
					std::vector<float> initial_noise(melBins* F0Shape[1], 0.0);
					for (auto& it : initial_noise)
						it = normal(gen) * noise_scale;
					long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, inp_audio.Hidden_Unit[slice].data(), HubertSize, HiddenUnitShape, 3));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
					inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Chara, 1, CharaEmbShape, 1));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
					inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, speedData, 1, CharaEmbShape, 1));

					std::vector<Ort::Value> DiffOut;
					try
					{
						DiffOut = diffSvc->Run(Ort::RunOptions{ nullptr },
							DiffInput.data(),
							inputTensors.data(),
							inputTensors.size(),
							DiffOutput.data(),
							DiffOutput.size());
					}
					catch (Ort::Exception& e2)
					{
						throw std::exception((std::string("Locate: Diff\n") + e2.what()).c_str());
					}
					inputTensors.clear();
					inputTensors.push_back(std::move(DiffOut[0]));
					inputTensors.push_back(std::move(DiffOut[1]));
					try
					{
						finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
							nsfInput.data(),
							inputTensors.data(),
							inputTensors.size(),
							nsfOutput.data(),
							nsfOutput.size());
					}
					catch (Ort::Exception& e3)
					{
						throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
					}
				}
				const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
				const auto dstWavLen = inp_audio.OrgLen[slice];
				std::vector<int16_t> TempVecWav(dstWavLen, 0);
				if (shapeOut[2] < dstWavLen)
				{
					for (int64_t bbb = 0; bbb < shapeOut[2]; bbb++)
						TempVecWav[bbb] = static_cast<int16_t>(finaOut[0].GetTensorData<float>()[bbb] * 32768.0f);
				}
				else
				{
					for (int64_t bbb = 0; bbb < dstWavLen; bbb++)
						TempVecWav[bbb] = static_cast<int16_t>(finaOut[0].GetTensorData<float>()[bbb] * 32768.0f);
				}
				_data.insert(_data.end(), TempVecWav.data(), TempVecWav.data() + (dstWavLen));
			}
			else
			{
				const auto len = inp_audio.OrgLen[slice];
				const auto data = new int16_t[len];
				memset(data, 0, int64_t(len) * 2);
				_data.insert(_data.end(), data, data + len);
				delete[] data;
				if(V2)
				{
					proc += RealDiffSteps;
					_callback(proc, Total_Slice_Count* RealDiffSteps);
				}
			}
			if(!V2)
				_callback(++proc, Total_Slice_Count);
		}
		logger.log(L"[Inferring] \"" + inp_audio.paths + L"\" Finished");
		if (total_audio_count == 1)
		{
			logger.log(L"[Info] Finished, Send To FrontEnd");
			return _data;
		}
		std::wstring outPutPath = GetCurrentFolder() + L"\\OutPuts\\" + inp_audio.paths.substr(inp_audio.paths.rfind(L'\\') + 1, inp_audio.paths.rfind(L'.')) + L'-' + std::to_wstring(uint64_t(inp_audio.paths.data())) + L".wav";
		logger.log(L"[Inferring] Write To \"" + outPutPath + L'\"');
		Wav(_samplingRate, long(_data.size()) * 2, _data.data()).Writef(outPutPath);
	}
	logger.log(L"[Info] Finished");
	return {};
}

std::vector<int16_t> DiffusionSvc::Inference(std::wstring& _inputLens) const
{
	auto SvcParams = GetSvcParam(_inputLens);
	return InferWithF0AndHiddenUnit(SvcParams);
}

std::vector<int16_t> DiffusionSvc::InferCurAudio(MoeVSProject::Params& input_audio_infer)
{
	const auto params = _get_init_params();
	int64_t charEmb = params.chara;
	std::mt19937 gen(int(params.seed));
	std::normal_distribution<float> normal(0, 1);
	float noise_scale = params.noise_scale;
	const int64 speedup = params.pndm;
	const int64 step = params.step;
	auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;
	std::vector<int64> diffusionSteps;
	for (int64 itt = step - speedup; itt >= 0; itt -= speedup)
		diffusionSteps.push_back(itt);

	if (!input_audio_infer.paths.empty())
		logger.log(L"[Inferring] Inferring \"" + input_audio_infer.paths + L'\"');
	size_t proc = 0;
	const auto Total_Slice_Count = input_audio_infer.F0.size();
	if (V2)
		_callback(proc, Total_Slice_Count * RealDiffSteps);
	else
		_callback(proc, Total_Slice_Count);
	logger.log(L"[Inferring] Inferring \"" + input_audio_infer.paths + L"\" Svc");

	std::vector<int16_t> _data;
	size_t total_audio_size = 0;
	for (const auto& data_size : input_audio_infer.OrgLen)
		total_audio_size += data_size;
	_data.reserve(size_t(double(total_audio_size) * 1.5));
	for (size_t slice = 0; slice < Total_Slice_Count; ++slice)
	{
		if (input_audio_infer.symbolb[slice])
		{
			auto RawWav = InterpResample(input_audio_infer.OrgAudio[slice], 48000, 16000, 32768.0f);
			const int64_t HubertInputShape[3] = { 1i64,1i64,(int64_t)RawWav.size() };
			std::vector<Ort::Value> HubertInputTensors, HubertOutPuts;
			HubertInputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RawWav.data(), RawWav.size(), HubertInputShape, 3));
			try {
				HubertOutPuts = hubert->Run(Ort::RunOptions{ nullptr },
					hubertInput.data(),
					HubertInputTensors.data(),
					HubertInputTensors.size(),
					hubertOutput.data(),
					hubertOutput.size());
			}
			catch (Ort::Exception& e)
			{
				throw std::exception((std::string("Locate: hubert\n") + e.what()).c_str());
			}
			auto HubertSize = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetElementCount();
			auto HubertOutPutData = HubertOutPuts[0].GetTensorMutableData<float>();
			auto HubertOutPutShape = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetShape();

			if (HubertOutPutShape[2] != Hidden_Size)
				throw std::exception("Hidden Size UnMatch");

			std::vector srcHiddenUnits(HubertOutPutData, HubertOutPutData + HubertSize);

			if (KMenas_Stat && params.kmeans_rate > 0.001f)
			{
				for (int64_t indexs = 0; indexs < HubertOutPutShape[1]; ++indexs)
				{
					const auto curbeg = srcHiddenUnits.data() + indexs * HubertOutPutShape[2];
					const auto curend = srcHiddenUnits.data() + (indexs + 1) * HubertOutPutShape[2];
					const auto hu = kmeans_->find({ curbeg ,curend }, long(params.chara));
					for (int64_t ind = 0; ind < HubertOutPutShape[2]; ++ind)
						*(curbeg + ind) = *(curbeg + ind) * (1.f - params.kmeans_rate) + hu[ind] * params.kmeans_rate;
				}
			}

			const auto HubertLen = int64_t(HubertSize) / Hidden_Size;
			const int64_t F0Shape[] = { 1, int64_t(input_audio_infer.OrgAudio[slice].size() * _samplingRate / 48000 / hop) };
			const int64_t HiddenUnitShape[] = { 1, HubertLen, Hidden_Size };
			constexpr int64_t CharaEmbShape[] = { 1 };
			int64_t CharaMixShape[] = { F0Shape[1],n_speaker };

			std::vector<float> InterpedF0;
			std::vector<int64_t> alignment;
			int64_t Chara[] = { charEmb };

			std::vector<Ort::Value> finaOut;
			auto& srcF0Data = input_audio_infer.F0[slice];
			for (auto& ifo : srcF0Data)
				ifo *= (float)pow(2.0, static_cast<double>(params.keys) / 12.0);

			std::vector<float> VolumeData;

			if (V2)
			{
				InterpedF0 = GetInterpedF0log(InterpFunc(srcF0Data, long(srcF0Data.size()), long(F0Shape[1])));
				alignment = GetAligments(F0Shape[1], HubertLen);
				std::vector<Ort::Value> inputTensors;
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, srcHiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
				if (!ddsp && !CharaMix)
					inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Chara, 1, CharaEmbShape, 1));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));
				std::vector<Ort::Value> encTensors;
				if (ddsp)
				{
					if (VolumeB)
					{
						VolumeData = InterpFunc(input_audio_infer.Volume[slice], long(input_audio_infer.Volume[slice].size()), long(F0Shape[1]));
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, VolumeData.data(), F0Shape[1], F0Shape, 2));
					}
					std::vector<float> chara_mix;
					if (CharaMix)
					{
						if (n_speaker > 1)
							chara_mix = GetCurrectSpkMixData(input_audio_infer.Speaker[slice], input_audio_infer.F0[slice].size(), F0Shape[1]);
						CharaMixShape[0] = F0Shape[1];
						if (chara_mix.empty())
						{
							std::vector<float> charaMap(n_speaker, 0.f);
							charaMap[params.chara] = 1.f;
							//std::vector<float>(n_speaker * CharaMixShape[0], 1.f / float(n_speaker));
							chara_mix.reserve((n_speaker + 1) * F0Shape[1]);
							for (int64_t index = 0; index < F0Shape[1]; ++index)
								chara_mix.insert(chara_mix.end(), charaMap.begin(), charaMap.end());
						}
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, chara_mix.data(), chara_mix.size(), CharaMixShape, 2));
					}
					try {
						encTensors = encoder->Run(Ort::RunOptions{ nullptr },
							encoderInputddsp.data(),
							inputTensors.data(),
							inputTensors.size(),
							encoderOutputDDSP.data(),
							encoderOutputDDSP.size());
					}
					catch (Ort::Exception& e1)
					{
						throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
					}
					encTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));
				}
				else
				{
					auto SvcInputNames = encoderInput;
					std::vector<float> chara_mix;
					if (CharaMix)
					{
						SvcInputNames = encoderInputSpkMix;
						if (n_speaker > 1)
							chara_mix = GetCurrectSpkMixData(input_audio_infer.Speaker[slice], input_audio_infer.F0[slice].size(), F0Shape[1]);
						CharaMixShape[0] = F0Shape[1];
						if (chara_mix.empty())
						{
							std::vector<float> charaMap(n_speaker, 0.f);
							charaMap[params.chara] = 1.f;
							//std::vector<float>(n_speaker * CharaMixShape[0], 1.f / float(n_speaker));
							chara_mix.reserve((n_speaker + 1) * F0Shape[1]);
							for (int64_t index = 0; index < F0Shape[1]; ++index)
								chara_mix.insert(chara_mix.end(), charaMap.begin(), charaMap.end());
						}
						inputTensors[2] = (Ort::Value::CreateTensor(*memory_info, chara_mix.data(), chara_mix.size(), CharaMixShape, 2));
					}
					if (VolumeB)
					{
						SvcInputNames.emplace_back("volume");
						VolumeData = InterpFunc(input_audio_infer.Volume[slice], long(input_audio_infer.Volume[slice].size()), long(F0Shape[1]));
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, VolumeData.data(), F0Shape[1], F0Shape, 2));
					}
					try {
						encTensors = encoder->Run(Ort::RunOptions{ nullptr },
							SvcInputNames.data(),
							inputTensors.data(),
							inputTensors.size(),
							encoderOutput.data(),
							encoderOutput.size());
					}
					catch (Ort::Exception& e1)
					{
						throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
					}
				}

				//Diffusion
				std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
				for (auto& it : initial_noise)
					it = normal(gen) * noise_scale;
				long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };

				std::vector<Ort::Value> DenoiseIn;
				std::vector<Ort::Value> DenoiseOut;
				std::vector<Ort::Value> PredIn;
				std::vector<Ort::Value> PredOut;
				std::vector<Ort::Value> DiffOut;
				std::deque<std::vector<float>> noiseList;
				int64 timeshape[1] = { 1 }; // PLMS SAMPLING
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
					_callback(++proc, Total_Slice_Count * RealDiffSteps);
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
				ganTensors.emplace_back(std::move(encTensors[1]));
				try
				{
					finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
						nsfInput.data(),
						ganTensors.data(),
						ganTensors.size(),
						nsfOutput.data(),
						nsfOutput.size());
				}
				catch (Ort::Exception& e3)
				{
					throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
				}
			}
			else
			{
				int64_t speedData[] = { pndm };
				InterpedF0 = GetInterpedF0log(InterpFunc(srcF0Data, long(srcF0Data.size()), long(F0Shape[1])));
				alignment = GetAligments(F0Shape[1], HubertLen);
				std::vector<Ort::Value> inputTensors;
				std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
				for (auto& it : initial_noise)
					it = normal(gen) * noise_scale;
				long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, srcHiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Chara, 1, CharaEmbShape, 1));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
				inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, speedData, 1, CharaEmbShape, 1));

				std::vector<Ort::Value> DiffOut;
				try
				{
					DiffOut = diffSvc->Run(Ort::RunOptions{ nullptr },
						DiffInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						DiffOutput.data(),
						DiffOutput.size());
				}
				catch (Ort::Exception& e2)
				{
					throw std::exception((std::string("Locate: Diff\n") + e2.what()).c_str());
				}
				inputTensors.clear();
				inputTensors.push_back(std::move(DiffOut[0]));
				inputTensors.push_back(std::move(DiffOut[1]));
				try
				{
					finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
						nsfInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						nsfOutput.data(),
						nsfOutput.size());
				}
				catch (Ort::Exception& e3)
				{
					throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
				}
			}
			const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
			const auto dstWavLen = (input_audio_infer.OrgLen[slice] * int64_t(_samplingRate)) / 48000;
			std::vector<int16_t> TempVecWav(dstWavLen, 0);
			if (shapeOut[2] < dstWavLen)
			{
				for (int64_t bbb = 0; bbb < shapeOut[2]; bbb++)
					TempVecWav[bbb] = static_cast<int16_t>(finaOut[0].GetTensorData<float>()[bbb] * 32768.0f);
			}
			else
			{
				for (int64_t bbb = 0; bbb < dstWavLen; bbb++)
					TempVecWav[bbb] = static_cast<int16_t>(finaOut[0].GetTensorData<float>()[bbb] * 32768.0f);
			}
			_data.insert(_data.end(), TempVecWav.data(), TempVecWav.data() + (dstWavLen));
		}
		else
		{
			const auto len = (input_audio_infer.OrgLen[slice] * int64_t(_samplingRate)) / 48000;
			const auto data = new int16_t[len];
			memset(data, 0, int64_t(len) * 2);
			_data.insert(_data.end(), data, data + len);
			delete[] data;
			if (V2)
			{
				proc += RealDiffSteps;
				_callback(proc, Total_Slice_Count * RealDiffSteps);
			}
		}
		if (!V2)
			_callback(++proc, Total_Slice_Count);
	}
	logger.log(L"[Inferring] \"" + input_audio_infer.paths + L"\" Finished");
	logger.log(L"[Info] Finished, Send To FrontEnd");
	return _data;
}

INFERCLASSEND
