#include "../header/DiffSvc.hpp"
#include <random>
#include "../../AvCodec/AvCodeResample.h"
#include <regex>
#include "../../InferTools/Sampler/MoeVSSamplerManager.hpp"
#include "../../InferTools/F0Extractor/F0ExtractorManager.hpp"

MoeVoiceStudioCoreHeader
void DiffusionSvc::Destory()
{
	//AudioEncoder
	delete hubert;
	hubert = nullptr;

	//VoCoder
	delete nsfHifigan;
	nsfHifigan = nullptr;

	//DiffusionModel
	delete encoder;      //Encoder
	encoder = nullptr;
	delete denoise;      //WaveNet
	denoise = nullptr;
	delete pred;         //PndmNoisePredictor
	pred = nullptr;
	delete after;        //AfterProcess
	after = nullptr;
	delete alpha;        //AlphasCumpord
	alpha = nullptr;
	delete naive;        //NaiveShallowDiffusion
	naive = nullptr;

	//SingleDiffusionModel
	delete diffSvc;
	diffSvc = nullptr;
}

DiffusionSvc::~DiffusionSvc()
{
	logger.log(L"[Info] unloading DiffSvc Models");
	Destory();
	logger.log(L"[Info] DiffSvc Models unloaded");
}

DiffusionSvc::DiffusionSvc(const MJson& _Config, const ProgressCallback& _ProgressCallback,
	ExecutionProviders ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_):
	SingingVoiceConversion(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioDiffSingingVoiceConversion");

	//Check Folder
	if (_Config["Folder"].IsNull())
		throw std::exception("[Error] Missing field \"folder\" (Model Folder)");
	if (!_Config["Folder"].IsString())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Must Be String");
	const auto _folder = to_wide_string(_Config["Folder"].GetString());
	if (_folder.empty())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Can Not Be Empty");
	const std::wstring _path = GetCurrentFolder() + L"/Models/" + _folder + L"/" + _folder;
	const auto cluster_folder = GetCurrentFolder() + L"/Models/" + _folder;
	if (_Config["Hubert"].IsNull())
		throw std::exception("[Error] Missing field \"Hubert\" (Hubert Folder)");
	if (!_Config["Hubert"].IsString())
		throw std::exception("[Error] Field \"Hubert\" (Hubert Folder) Must Be String");
	const std::wstring HuPath = to_wide_string(_Config["Hubert"].GetString());
	if (HuPath.empty())
		throw std::exception("[Error] Field \"Hubert\" (Hubert Folder) Can Not Be Empty");

	if (_Config["Hifigan"].IsNull())
		throw std::exception("[Error] Missing field \"Hifigan\" (Hifigan Folder)");
	if (!_Config["Hifigan"].IsString())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Folder) Must Be String");
	const std::wstring HifiganPath = to_wide_string(_Config["Hifigan"].GetString());
	if (HifiganPath.empty())
		throw std::exception("[Error] Field \"Hifigan\" (Hifigan Folder) Can Not Be Empty");

	//Check SamplingRate
	if (_Config["Rate"].IsNull())
		throw std::exception("[Error] Missing field \"Rate\" (SamplingRate)");
	if (_Config["Rate"].IsInt() || _Config["Rate"].IsInt64())
		_samplingRate = _Config["Rate"].GetInt();
	else
		throw std::exception("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64");

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	if (_Config["MelBins"].IsNull())
		throw std::exception("[Error] Missing field \"MelBins\" (MelBins)");
	if (_Config["MelBins"].IsInt() || _Config["MelBins"].IsInt64())
		melBins = _Config["MelBins"].GetInt();
	else
		throw std::exception("[Error] Field \"MelBins\" (MelBins) Must Be Int/Int64");

	if (!(_Config["Hop"].IsInt() || _Config["Hop"].IsInt64()))
		throw std::exception("[Error] Hop Must Be Int");
	HopSize = _Config["Hop"].GetInt();

	if (HopSize < 1)
		throw std::exception("[Error] Hop Must > 0");

	if (!(_Config["HiddenSize"].IsInt() || _Config["HiddenSize"].IsInt64()))
		logger.log(L"[Warn] Missing Field \"HiddenSize\", Use Default Value (256)");
	else
		HiddenUnitKDims = _Config["HiddenSize"].GetInt();

	if (_Config["Characters"].IsArray())
		SpeakerCount = (int64_t)_Config["Characters"].Size();

	if (_Config["Volume"].IsBool())
		EnableVolume = _Config["Volume"].GetBool();
	else
		logger.log(L"[Warn] Missing Field \"Volume\", Use Default Value (False)");

	if (!_Config["CharaMix"].IsBool())
		logger.log(L"[Warn] Missing Field \"CharaMix\", Use Default Value (False)");
	else
		EnableCharaMix = _Config["CharaMix"].GetBool();

	if (!_Config["Diffusion"].IsBool())
		logger.log(L"[Warn] Missing Field \"Diffusion\", Use Default Value (False)");
	else if (_Config["Diffusion"].GetBool())
		DiffSvcVersion = L"DiffusionSvc";

	if (_Config["Pndm"].IsInt())
		Pndms = _Config["Pndm"].GetInt();

	_callback = _ProgressCallback;

	if (_Config["Cluster"].IsString())
	{
		const auto clus = to_wide_string(_Config["Cluster"].GetString());
		if (!(_Config["KMeansLength"].IsInt() || _Config["KMeansLength"].IsInt64()))
			logger.log(L"[Warn] Missing Field \"KMeansLength\", Use Default Value (10000)");
		else
			ClusterCenterSize = _Config["KMeansLength"].GetInt();
		try
		{
			Cluster = MoeVoiceStudioCluster::GetMoeVSCluster(clus, cluster_folder, HiddenUnitKDims, ClusterCenterSize);
			EnableCluster = true;
		}
		catch (std::exception& e)
		{
			logger.error(e.what());
			EnableCluster = false;
		}
	}

	//LoadModels
	try
	{
		logger.log(L"[Info] loading DiffSvc Models");
		hubert = new Ort::Session(*env, (GetCurrentFolder() + L"/hubert/" + HuPath + L".onnx").c_str(), *session_options);
		nsfHifigan = new Ort::Session(*env, (GetCurrentFolder() + L"/hifigan/" + HifiganPath + L".onnx").c_str(), *session_options);
		if (_waccess((_path + L"_encoder.onnx").c_str(), 0) != -1)
		{
			encoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
			denoise = new Ort::Session(*env, (_path + L"_denoise.onnx").c_str(), *session_options);
			pred = new Ort::Session(*env, (_path + L"_pred.onnx").c_str(), *session_options);
			after = new Ort::Session(*env, (_path + L"_after.onnx").c_str(), *session_options);
			if(_waccess((_path + L"_alpha.onnx").c_str(), 0) != -1)
				alpha = new Ort::Session(*env, (_path + L"_alpha.onnx").c_str(), *session_options);
		}
		else
			diffSvc = new Ort::Session(*env, (_path + L"_DiffSvc.onnx").c_str(), *session_options);

		if(_waccess((_path + L"_naive.onnx").c_str(), 0) != -1)
			naive = new Ort::Session(*env, (_path + L"_naive.onnx").c_str(), *session_options);

		logger.log(L"[Info] DiffSvc Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		Destory();
		throw std::exception(_exception.what());
	}

	if (_Config["TensorExtractor"].IsString())
		DiffSvcVersion = to_wide_string(_Config["TensorExtractor"].GetString());

	if (_Config["MaxStep"].IsInt())
		MaxStep = _Config["MaxStep"].GetInt();

	MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Others _others_param;
	_others_param.Memory = *memory_info;

	try
	{
		_TensorExtractor = GetTensorExtractor(DiffSvcVersion, 48000, _samplingRate, HopSize, EnableCharaMix, EnableVolume, HiddenUnitKDims, SpeakerCount, _others_param);
	}
	catch (std::exception& e)
	{
		Destory();
		throw std::exception(e.what());
	}
}

std::vector<int16_t> DiffusionSvc::SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Start!");
	std::vector<int16_t> _data;
	size_t total_audio_size = 0;
	for (const auto& data_size : _Slice.OrgLen)
		total_audio_size += data_size;
	_data.reserve(size_t(double(total_audio_size) * 1.5));

	std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);

	auto speedup = (int64_t)_InferParams.Pndm;
	auto step = (int64_t)_InferParams.Step;

	if (step > MaxStep) step = MaxStep;
	if (speedup >= step) speedup = step / 5;
	if (speedup == 0) speedup = 1;
	const auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;
	if (diffSvc)
		_callback(0, _Slice.F0.size());
	else
		_callback(0, _Slice.F0.size() * RealDiffSteps);
	size_t process = 0;
	for (size_t slice = 0; slice < _Slice.F0.size(); ++slice)
	{
		if (_Slice.IsNotMute[slice])
		{
			const auto InferDurTime = clock();
			auto RawWav = InferTools::InterpResample(_Slice.Audio[slice], 48000, 16000, 32768.0f);
			const auto src_audio_length = RawWav.size();
			bool NeedPadding = false;
			if (_cur_execution_provider == ExecutionProviders::CUDA && !diffSvc)
			{
				NeedPadding = RawWav.size() % 16000;
				const size_t WavPaddedSize = RawWav.size() / 16000 + 1;
				if (NeedPadding)
					RawWav.resize(WavPaddedSize * 16000, 0.f);
			}

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
			const auto HubertSize = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetElementCount();
			const auto HubertOutPutData = HubertOutPuts[0].GetTensorMutableData<float>();
			auto HubertOutPutShape = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetShape();
			HubertInputTensors.clear();
			if (HubertOutPutShape[2] != HiddenUnitKDims)
				throw std::exception("HiddenUnitKDims UnMatch");

			std::vector srcHiddenUnits(HubertOutPutData, HubertOutPutData + HubertSize);

			const auto max_cluster_size = int64_t((size_t)HubertOutPutShape[1] * src_audio_length / RawWav.size());
			if (EnableCluster && _InferParams.ClusterRate > 0.001f)
			{
				const auto pts = Cluster->find(srcHiddenUnits.data(), long(_InferParams.SpeakerId), max_cluster_size);
				for (int64_t indexs = 0; indexs < max_cluster_size * HiddenUnitKDims; ++indexs)
					srcHiddenUnits[indexs] = srcHiddenUnits[indexs] * (1.f - _InferParams.ClusterRate) + pts[indexs] * _InferParams.ClusterRate;
			}
			std::vector<Ort::Value> finaOut;
			std::vector<Ort::Value> DiffOut;
			if(diffSvc)
			{
				const auto HubertLen = int64_t(HubertSize) / HiddenUnitKDims;
				const int64_t F0Shape[] = { 1, int64_t(_Slice.Audio[slice].size() * _samplingRate / 48000 / HopSize) };
				const int64_t HiddenUnitShape[] = { 1, HubertLen, HiddenUnitKDims };
				constexpr int64_t CharaEmbShape[] = { 1 };
				int64_t speedData[] = { Pndms };
				auto srcF0Data = InferTools::InterpFunc(_Slice.F0[slice], long(_Slice.F0[slice].size()), long(F0Shape[1]));
				for (auto& it : srcF0Data)
					it *= (float)pow(2.0, static_cast<double>(_InferParams.Keys) / 12.0);
				auto InterpedF0 = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetInterpedF0log(srcF0Data, true);
				auto alignment = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetAligments(F0Shape[1], HubertLen);
				std::vector<Ort::Value> TensorsInp;

				int64_t Chara[] = { _InferParams.SpeakerId };

				TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, srcHiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
				TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
				TensorsInp.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Chara, 1, CharaEmbShape, 1));
				TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));

				std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
				long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };
				if(!naive)
				{
					for (auto& it : initial_noise)
						it = normal(gen) * _InferParams.NoiseScale;
					TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
				}
				else
					MoeVSNotImplementedError;
				
				TensorsInp.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, speedData, 1, CharaEmbShape, 1));
				try
				{
					DiffOut = diffSvc->Run(Ort::RunOptions{ nullptr },
						DiffInput.data(),
						TensorsInp.data(),
						TensorsInp.size(),
						DiffOutput.data(),
						DiffOutput.size());
				}
				catch (Ort::Exception& e2)
				{
					throw std::exception((std::string("Locate: Diff\n") + e2.what()).c_str());
				}
				try
				{
					finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
						nsfInput.data(),
						DiffOut.data(),
						DiffOut.size(),
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
				MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::InferParams _Inference_Params;
				_Inference_Params.AudioSize = _Slice.Audio[slice].size();
				_Inference_Params.Chara = _InferParams.SpeakerId;
				_Inference_Params.NoiseScale = _InferParams.NoiseScale;
				_Inference_Params.DDSPNoiseScale = _InferParams.DDSPNoiseScale;
				_Inference_Params.Seed = int(_InferParams.Seed);
				_Inference_Params.upKeys = _InferParams.Keys;

				MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Inputs InputTensors;

				if (_cur_execution_provider == ExecutionProviders::CUDA && NeedPadding)
				{
					auto CUDAF0 = _Slice.F0[slice];
					auto CUDAVolume = _Slice.Volume[slice];
					auto CUDASpeaker = _Slice.Speaker[slice];
					const auto src_src_audio_length = _Slice.Audio[slice].size();
					const size_t WavPaddedSize = ((src_src_audio_length / 48000) + 1) * 48000;
					const size_t AudioPadSize = WavPaddedSize - src_src_audio_length;
					const size_t PaddedF0Size = CUDAF0.size() + (CUDAF0.size() * AudioPadSize / src_src_audio_length);

					if (!CUDAF0.empty()) CUDAF0.resize(PaddedF0Size, 0.f);
					if (!CUDAVolume.empty()) CUDAVolume.resize(PaddedF0Size, 0.f);
					for (auto iSpeaker : CUDASpeaker)
					{
						if (!iSpeaker.empty())
							iSpeaker.resize(PaddedF0Size, 0.f);
					}
					_Inference_Params.AudioSize = WavPaddedSize;
					InputTensors = _TensorExtractor->Extract(srcHiddenUnits, CUDAF0, CUDAVolume, CUDASpeaker, _Inference_Params);
				}
				else
					InputTensors = _TensorExtractor->Extract(srcHiddenUnits, _Slice.F0[slice], _Slice.Volume[slice], _Slice.Speaker[slice], _Inference_Params);

				std::vector<Ort::Value> EncoderOut;
				try {
					EncoderOut = encoder->Run(Ort::RunOptions{ nullptr },
						InputTensors.InputNames,
						InputTensors.Tensor.data(),
						min(InputTensors.Tensor.size(), encoder->GetInputCount()),
						InputTensors.OutputNames,
						InputTensors.OutputCount);
				}
				catch (Ort::Exception& e1)
				{
					throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
				}
				if (EncoderOut.size() == 1)
					EncoderOut.emplace_back(Ort::Value::CreateTensor(*memory_info, InputTensors.Data.F0.data(), InputTensors.Data.FrameShape[1], InputTensors.Data.FrameShape.data(), 2));

				std::vector<Ort::Value> DenoiseInTensors;
				DenoiseInTensors.emplace_back(std::move(EncoderOut[0]));

				std::vector<float> initial_noise(melBins * InputTensors.Data.FrameShape[1], 0.0);
				long long noise_shape[4] = { 1,1,melBins,InputTensors.Data.FrameShape[1] };
				if(!naive)
				{
					for (auto& it : initial_noise)
						it = normal(gen) * _InferParams.NoiseScale;
					DenoiseInTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
				}
				else
				{
					std::vector<Ort::Value> NaiveOut;
					try {
						NaiveOut = naive->Run(Ort::RunOptions{ nullptr },
							InputTensors.InputNames,
							InputTensors.Tensor.data(),
							min(InputTensors.Tensor.size(), naive->GetInputCount()),
							naiveOutput.data(),
							1);
					}
					catch (Ort::Exception& e1)
					{
						throw std::exception((std::string("Locate: naive\n") + e1.what()).c_str());
					}
					DenoiseInTensors.emplace_back(std::move(NaiveOut[0]));
				}

				auto PredOut = MoeVSSampler::GetMoeVSSampler((!alpha ? L"Pndm" : _InferParams.Sampler), alpha, denoise, pred, melBins, _callback, memory_info)->Sample(DenoiseInTensors, step, speedup, _InferParams.NoiseScale, _InferParams.Seed, process);

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

				DiffOut.emplace_back(std::move(EncoderOut[1]));
				try
				{
					finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
						nsfInput.data(),
						DiffOut.data(),
						DiffOut.size(),
						nsfOutput.data(),
						nsfOutput.size());
				}
				catch (Ort::Exception& e3)
				{
					throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
				}
			}
			const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
			const auto dstWavLen = (_Slice.OrgLen[slice] * int64_t(_samplingRate)) / 48000;
			std::vector<int16_t> TempVecWav(dstWavLen, 0);
			if (shapeOut[2] < dstWavLen)
				for (int64_t bbb = 0; bbb < shapeOut[2]; bbb++)
					TempVecWav[bbb] = static_cast<int16_t>(Clamp(finaOut[0].GetTensorData<float>()[bbb]) * 32766.f);
			else
				for (int64_t bbb = 0; bbb < dstWavLen; bbb++)
					TempVecWav[bbb] = static_cast<int16_t>(Clamp(finaOut[0].GetTensorData<float>()[bbb]) * 32766.f);
			_data.insert(_data.end(), TempVecWav.data(), TempVecWav.data() + (dstWavLen));
			logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Segment[" + std::to_wstring(slice) + L"] Finished! Segment Use Time: " + std::to_wstring(clock() - InferDurTime) + L"ms, Segment Duration: " + std::to_wstring((size_t)_Slice.OrgLen[slice] * 1000ull / 48000ull) + L"ms");
		}
		else
		{
			//Mute clips
			const auto len = _Slice.OrgLen[slice] * int64_t(_samplingRate) / 48000;
			const auto data = new int16_t[len];
			memset(data, 0, int64_t(len) * 2);
			_data.insert(_data.end(), data, data + len);
			delete[] data;
			if(!diffSvc)
			{
				process += RealDiffSteps;
				_callback(process, _Slice.F0.size() * RealDiffSteps);
			}
			logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Jump Empty Segment[" + std::to_wstring(slice) + L"]!");
		}
		if(diffSvc)
			_callback(++process, _Slice.F0.size());
	}
	logger.log(L"[Inferring] \"" + _Slice.Path + L"\" Finished");
	return _data;
}

std::vector<std::wstring> DiffusionSvc::Inference(std::wstring& _Paths,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
	const InferTools::SlicerSettings& _SlicerSettings) const
{
	std::vector<std::wstring> _Lens = GetOpenFileNameMoeVS();
	std::vector<std::wstring> AudioFolders;
	for (auto& path : _Lens)
	{
		path = std::regex_replace(path, std::wregex(L"\\\\"), L"/");
		auto PCMData = AudioPreprocess().codec(path, 48000);
		auto SlicePos = SliceAudio(PCMData, _SlicerSettings);
		auto Audio = GetAudioSlice(PCMData, SlicePos, _SlicerSettings);
		Audio.Path = path;
		PreProcessAudio(Audio);
		std::vector<int16_t> _data = SliceInference(Audio, _InferParams);

		std::wstring OutFolder = GetCurrentFolder() + L"/Outputs/" + path.substr(path.rfind(L'/') + 1, path.rfind(L'.') - path.rfind(L'/') - 1);
		OutFolder += L"-Params-(-NoiseScale=" +
			std::to_wstring(_InferParams.NoiseScale) +
			L"-Speaker=" +
			(EnableCharaMix ? std::wstring(L"SpeakerMix") : std::to_wstring(_InferParams.SpeakerId)) +
			L"-Seed=" +
			std::to_wstring(_InferParams.Seed) +
			L"-Sampler=" +
			_InferParams.Sampler +
			L"-F0Method=" +
			_InferParams.F0Method + L")";
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
		InferTools::Wav::WritePCMData(_samplingRate, 1, _data, OutFolder);
	}
	return AudioFolders;
}

std::vector<int16_t> DiffusionSvc::InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	if (diffSvc || DiffSvcVersion != L"DiffusionSvc")
		return PCMData;

	auto hubertin = InferTools::InterpResample<float>(PCMData, srcSr, 16000);
	const int64_t charEmb = _InferParams.SpeakerId;
	std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);

	const int64_t inputShape[3] = { 1i64,1i64,(int64_t)hubertin.size() };
	std::vector<Ort::Value> inputTensorshu;
	inputTensorshu.emplace_back(Ort::Value::CreateTensor(*memory_info, hubertin.data(), hubertin.size(), inputShape, 3));
	std::vector<Ort::Value> hubertOut;

	const auto RealDiffSteps = _InferParams.Step % _InferParams.Pndm ? _InferParams.Step / _InferParams.Pndm + 1 : _InferParams.Step / _InferParams.Pndm;
	_callback(0, RealDiffSteps);

	try {
		hubertOut = hubert->Run(Ort::RunOptions{ nullptr },
			hubertInput.data(),
			inputTensorshu.data(),
			inputTensorshu.size(),
			hubertOutput.data(),
			hubertOutput.size());
	}
	catch (Ort::Exception& e)
	{
		throw std::exception((std::string("Locate: hubert\n") + e.what()).c_str());
	}
	const auto HubertSize = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
	const auto HubertOutPutData = hubertOut[0].GetTensorMutableData<float>();
	const auto HubertOutPutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();
	inputTensorshu.clear();
	if (HubertOutPutShape[2] != HiddenUnitKDims)
		throw std::exception("HiddenUnitKDims UnMatch");

	std::vector HiddenUnits(HubertOutPutData, HubertOutPutData + HubertSize);

	if (EnableCluster && _InferParams.ClusterRate > 0.001f)
	{
		const auto clus_size = HubertOutPutShape[1];
		const auto pts = Cluster->find(HiddenUnits.data(), long(charEmb), clus_size);
		for (size_t indexs = 0; indexs < HiddenUnits.size(); ++indexs)
			HiddenUnits[indexs] = HiddenUnits[indexs] * (1.f - _InferParams.ClusterRate) + pts[indexs] * _InferParams.ClusterRate;
	}

	const auto HubertLen = int64_t(HubertSize) / HiddenUnitKDims;
	const int64_t F0Shape[] = { 1, int64_t(PCMData.size() / HopSize) };
	const int64_t HiddenUnitShape[] = { 1, HubertLen, HiddenUnitKDims };
	constexpr int64_t CharaEmbShape[] = { 1 };
	const int64_t CharaMixShape[] = { F0Shape[1], SpeakerCount };

	const auto F0Extractor = MoeVSF0Extractor::GetF0Extractor(_InferParams.F0Method, _samplingRate, HopSize);
	auto F0Data = F0Extractor->ExtractF0(PCMData, PCMData.size() / HopSize);
	for (auto& ifo : F0Data)
		ifo *= (float)pow(2.0, static_cast<double>(_InferParams.Keys) / 12.0);
	F0Data = _TensorExtractor->GetInterpedF0(InferTools::InterpFunc(F0Data, long(F0Data.size()), long(F0Shape[1])));
	std::vector<int64_t> Alignment = _TensorExtractor->GetAligments(F0Shape[1], HubertLen);
	int64_t CharaEmb[] = { charEmb };

	std::vector<Ort::Value> EncoderTensors;

	EncoderTensors.emplace_back(Ort::Value::CreateTensor(
		*memory_info,
		HiddenUnits.data(),
		HubertSize,
		HiddenUnitShape,
		3
	));

	EncoderTensors.emplace_back(Ort::Value::CreateTensor(
		*memory_info,
		Alignment.data(),
		F0Shape[1],
		F0Shape,
		2
	));

	EncoderTensors.emplace_back(Ort::Value::CreateTensor(
		*memory_info,
		F0Data.data(),
		F0Shape[1],
		F0Shape,
		2
	));

	std::vector<const char*> InputNamesEncoder;
	std::vector<float> Volume, SpkMap;

	if (EnableVolume)
	{
		InputNamesEncoder = { "hubert", "mel2ph", "f0", "volume", "spk_mix" };
		Volume = ExtractVolume(PCMData, HopSize);
		if (abs(int64_t(Volume.size()) - int64_t(F0Data.size())) > 3)
			Volume = InferTools::InterpFunc(ExtractVolume(PCMData, HopSize), long(Volume.size()), long(F0Shape[1]));
		else
			Volume.resize(F0Data.size(), 0.f);
		EncoderTensors.emplace_back(Ort::Value::CreateTensor(
			*memory_info,
			Volume.data(),
			F0Shape[1],
			F0Shape,
			2
		));
	}
	else
		InputNamesEncoder = { "hubert", "mel2ph", "f0", "spk_mix" };

	if (EnableCharaMix)
	{
		SpkMap = _TensorExtractor->GetCurrectSpkMixData(std::vector<std::vector<float>>(), F0Shape[1], charEmb);
		EncoderTensors.emplace_back(Ort::Value::CreateTensor(
			*memory_info,
			SpkMap.data(),
			SpkMap.size(),
			CharaMixShape,
			2
		));
	}
	else
	{
		EncoderTensors.emplace_back(Ort::Value::CreateTensor(
			*memory_info,
			CharaEmb,
			1,
			CharaEmbShape,
			1
		));
	}

	const std::vector OutputNamesEncoder = { "mel_pred" };

	std::vector<Ort::Value> EncoderOut;
	try {
		EncoderOut = encoder->Run(Ort::RunOptions{ nullptr },
			InputNamesEncoder.data(),
			EncoderTensors.data(),
			min(EncoderTensors.size(), encoder->GetInputCount()),
			OutputNamesEncoder.data(),
			1);
	}
	catch (Ort::Exception& e1)
	{
		throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
	}
	EncoderOut.emplace_back(Ort::Value::CreateTensor(*memory_info, F0Data.data(), F0Shape[1], F0Shape, 2));

	std::vector<Ort::Value> DenoiseInTensors;
	DenoiseInTensors.emplace_back(std::move(EncoderOut[0]));

	std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
	long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };
	if (!naive)
	{
		for (auto& it : initial_noise)
			it = normal(gen) * _InferParams.NoiseScale;
		DenoiseInTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
	}
	else
	{
		std::vector<Ort::Value> NaiveOut;
		try {
			NaiveOut = naive->Run(Ort::RunOptions{ nullptr },
				InputNamesEncoder.data(),
				EncoderTensors.data(),
				min(EncoderTensors.size(), naive->GetInputCount()),
				naiveOutput.data(),
				1);
		}
		catch (Ort::Exception& e1)
		{
			throw std::exception((std::string("Locate: naive\n") + e1.what()).c_str());
		}
		DenoiseInTensors.emplace_back(std::move(NaiveOut[0]));
	}

	size_t process = 0;

	auto PredOut = MoeVSSampler::GetMoeVSSampler((!alpha ? L"Pndm" : _InferParams.Sampler), alpha, denoise, pred, melBins, _callback, memory_info)->Sample(DenoiseInTensors, (int64_t)_InferParams.Step, (int64_t)_InferParams.Pndm, _InferParams.NoiseScale, _InferParams.Seed, process);

	std::vector<Ort::Value> DiffOut, finaOut;

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

	DiffOut.emplace_back(std::move(EncoderOut[1]));
	try
	{
		finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
			nsfInput.data(),
			DiffOut.data(),
			DiffOut.size(),
			nsfOutput.data(),
			nsfOutput.size());
	}
	catch (Ort::Exception& e3)
	{
		throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
	}

	const auto dstWavLen = finaOut[0].GetTensorTypeAndShapeInfo().GetShape()[2];
	std::vector<int16_t> TempVecWav(dstWavLen, 0);
	for (int64_t bbb = 0; bbb < dstWavLen; bbb++)
		TempVecWav[bbb] = static_cast<int16_t>(Clamp(finaOut[0].GetTensorData<float>()[bbb]) * 32766.0f);
	return TempVecWav;
}

std::vector<int16_t> DiffusionSvc::ShallowDiffusionInference(
	std::vector<float>& _16KAudioHubert,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
	Ort::Value&& _Mel,
	const std::vector<float>& _SrcF0,
	const std::vector<float>& _SrcVolume,
	const std::vector<std::vector<float>>& _SrcSpeakerMap
) const
{
	if (diffSvc || DiffSvcVersion != L"DiffusionSvc")
		throw std::exception("ShallowDiffusion Only Support DiffusionSvc Model");
	std::vector<const char*> InputNamesEncoder;
	const int64_t _Mel_Size = _Mel.GetTensorTypeAndShapeInfo().GetShape()[3];

	std::vector<Ort::Value> HubertInputTensors, HubertOutputTensors;
	const int64_t HubertInputShape[3] = { 1i64,1i64,(int64_t)_16KAudioHubert.size() };
	HubertInputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, _16KAudioHubert.data(), _16KAudioHubert.size(), HubertInputShape, 3));
	try {
		HubertOutputTensors = hubert->Run(Ort::RunOptions{ nullptr },
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

	const auto HubertLength = HubertOutputTensors[0].GetTensorTypeAndShapeInfo().GetShape()[1];
	const int64_t FrameShape[] = { 1, _Mel_Size };
	const int64_t CharaMixShape[] = { _Mel_Size, SpeakerCount };
	constexpr int64_t OneShape[] = { 1 };
	int64_t CharaEmb[] = { _InferParams.SpeakerId };

	auto Alignment = _TensorExtractor->GetAligments(_Mel_Size, HubertLength);
	Alignment.resize(FrameShape[1]);
	auto F0Data = InferTools::InterpFunc(_SrcF0, long(_SrcF0.size()), long(FrameShape[1]));
	std::vector<float> Volume, SpkMap;

	std::vector<Ort::Value> EncoderTensors;
	EncoderTensors.emplace_back(std::move(HubertOutputTensors[0]));
	EncoderTensors.emplace_back(Ort::Value::CreateTensor(
		*memory_info,
		Alignment.data(),
		FrameShape[1],
		FrameShape,
		2
	));

	EncoderTensors.emplace_back(Ort::Value::CreateTensor(
		*memory_info,
		F0Data.data(),
		FrameShape[1],
		FrameShape,
		2
	));

	if (EnableVolume)
	{
		InputNamesEncoder = { "hubert", "mel2ph", "f0", "volume", "spk_mix" };
		Volume = InferTools::InterpFunc(_SrcVolume, long(_SrcVolume.size()), long(FrameShape[1]));
		EncoderTensors.emplace_back(Ort::Value::CreateTensor(
			*memory_info,
			Volume.data(),
			FrameShape[1],
			FrameShape,
			2
		));
	}
	else
		InputNamesEncoder = { "hubert", "mel2ph", "f0", "spk_mix" };

	if (EnableCharaMix)
	{
		SpkMap = _TensorExtractor->GetCurrectSpkMixData(_SrcSpeakerMap, FrameShape[1], CharaEmb[0]);
		EncoderTensors.emplace_back(Ort::Value::CreateTensor(
			*memory_info,
			SpkMap.data(),
			SpkMap.size(),
			CharaMixShape,
			2
		));
	}
	else
	{
		EncoderTensors.emplace_back(Ort::Value::CreateTensor(
			*memory_info,
			CharaEmb,
			1,
			OneShape,
			1
		));
	}

	const std::vector OutputNamesEncoder = { "mel_pred" };

	std::vector<Ort::Value> EncoderOut;
	try {
		EncoderOut = encoder->Run(Ort::RunOptions{ nullptr },
			InputNamesEncoder.data(),
			EncoderTensors.data(),
			min(EncoderTensors.size(), encoder->GetInputCount()),
			OutputNamesEncoder.data(),
			1);
	}
	catch (Ort::Exception& e1)
	{
		throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
	}

	std::vector<Ort::Value> DenoiseInTensors;
	DenoiseInTensors.emplace_back(std::move(EncoderOut[0]));
	DenoiseInTensors.emplace_back(std::move(_Mel));

	size_t process = 0;
	auto PredOut = MoeVSSampler::GetMoeVSSampler((!alpha ? L"Pndm" : _InferParams.Sampler), alpha, denoise, pred, melBins, [](size_t, size_t) {}, memory_info)->Sample(DenoiseInTensors, (int64_t)_InferParams.Step, (int64_t)_InferParams.Pndm, _InferParams.NoiseScale, _InferParams.Seed, process);

	std::vector<Ort::Value> DiffOut, finaOut;
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

	DiffOut.emplace_back(std::move(EncoderTensors[2]));
	try
	{
		finaOut = nsfHifigan->Run(Ort::RunOptions{ nullptr },
			nsfInput.data(),
			DiffOut.data(),
			DiffOut.size(),
			nsfOutput.data(),
			nsfOutput.size());
	}
	catch (Ort::Exception& e3)
	{
		throw std::exception((std::string("Locate: Nsf\n") + e3.what()).c_str());
	}

	const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
	const auto dstWavLen = int64_t(_16KAudioHubert.size() * int64_t(_samplingRate)) / 16000;
	std::vector<float> TempVecWav(dstWavLen, 0);
	memcpy(TempVecWav.data(), finaOut[0].GetTensorData<float>(), min(dstWavLen, shapeOut[2]) * sizeof(float));
	auto OutTmpData = InferTools::InterpResample(TempVecWav, _samplingRate, (long)_InferParams.SrcSamplingRate, 1.f);
	std::vector<int16_t> OutData;
	OutData.reserve(OutTmpData.size());
	for (const auto& dat : OutTmpData)
		OutData.emplace_back(int16_t(Clamp(dat) * 32766.f));
	return OutData;
}

MoeVoiceStudioCoreEnd
