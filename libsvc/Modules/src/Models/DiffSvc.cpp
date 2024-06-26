﻿#include "../../header/Models/DiffSvc.hpp"
#include <random>
#include "../../header/InferTools/AvCodec/AvCodeResample.h"
#include <regex>
#include "../../header/InferTools/Sampler/MoeVSSamplerManager.hpp"
#include "../../header/InferTools/F0Extractor/F0ExtractorManager.hpp"
#include "../../header/Models/EnvManager.hpp"

MoeVoiceStudioCoreHeader

Ort::Session* GlobalVocoder = nullptr;

void LoadVocoderModel(const std::wstring& VocoderPath)
{
	GlobalVocoder = new Ort::Session(*moevsenv::GetGlobalMoeVSEnv().GetEnv(), VocoderPath.c_str(), *moevsenv::GetGlobalMoeVSEnv().GetSessionOptions());
}

void UnLoadVocoderModel()
{
	delete GlobalVocoder;
	GlobalVocoder = nullptr;
}

bool VocoderEnabled()
{
	return GlobalVocoder;
}

Ort::Session* GetCurrentVocoder()
{
	return GlobalVocoder;
}

void DiffusionSvc::Destory()
{
	//AudioEncoder
	delete hubert;
	hubert = nullptr;

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
		LibDLVoiceCodecThrow("[Error] Missing field \"folder\" (Model Folder)")
	if (!_Config["Folder"].IsString())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Must Be String")
	const auto _folder = to_wide_string(_Config["Folder"].GetString());
	if (_folder.empty())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Can Not Be Empty")
	const std::wstring _path = GetCurrentFolder() + L"/Models/" + _folder + L"/" + _folder;
	const auto cluster_folder = GetCurrentFolder() + L"/Models/" + _folder;
	if (_Config["Hubert"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Hubert\" (Hubert Folder)")
	if (!_Config["Hubert"].IsString())
		LibDLVoiceCodecThrow("[Error] Field \"Hubert\" (Hubert Folder) Must Be String")
	const std::wstring HuPath = to_wide_string(_Config["Hubert"].GetString());
	if (HuPath.empty())
		LibDLVoiceCodecThrow("[Error] Field \"Hubert\" (Hubert Folder) Can Not Be Empty")

	std::map<std::string, std::wstring> _PathDict;
	_PathDict["Cluster"] = cluster_folder;
	_PathDict["DiffHubert"] = GetCurrentFolder() + L"/hubert/" + HuPath + L".onnx";
	if(_waccess((_path + L"_encoder.onnx").c_str(), 0) != -1)
	{
		_PathDict["Encoder"] = _path + L"_encoder.onnx";
		_PathDict["DenoiseFn"] = _path + L"_denoise.onnx";
		_PathDict["NoisePredictor"] = _path + L"_pred.onnx";
		_PathDict["AfterProcess"] = _path + L"_after.onnx";
		if (_waccess((_path + L"_alpha.onnx").c_str(), 0) != -1)
			_PathDict["Alphas"] = _path + L"_alpha.onnx";
	}
	else
		_PathDict["DiffSvc"] = _path + L"_DiffSvc.onnx";
	
	if (_waccess((_path + L"_naive.onnx").c_str(), 0) != -1)
		_PathDict["Naive"] = _path + L"_naive.onnx";

	logger.log("[Model Loader] Prepeocess Complete!");
	load(_PathDict, _Config, _ProgressCallback);
}

DiffusionSvc::DiffusionSvc(const std::map<std::string, std::wstring>& _PathDict,
	const MJson& _Config, const ProgressCallback& _ProgressCallback,
	ExecutionProviders ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_) :
	SingingVoiceConversion(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioDiffSingingVoiceConversion");

	load(_PathDict, _Config, _ProgressCallback);
}

DiffusionSvc::DiffusionSvc(const Hparams& _Hps, const ProgressCallback& _ProgressCallback, ExecutionProviders ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_) : 
	SingingVoiceConversion(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioDiffSingingVoiceConversion");

	_samplingRate = max(_Hps.SamplingRate, 2000);
	melBins = max(_Hps.MelBins, 1);
	HopSize = max(_Hps.HopSize, 1);
	HiddenUnitKDims = max(_Hps.HiddenUnitKDims, 1);
	SpeakerCount = max(_Hps.SpeakerCount, 1);
	EnableVolume = _Hps.EnableVolume;
	EnableCharaMix = _Hps.EnableCharaMix;
	DiffSvcVersion = _Hps.TensorExtractor;
	Pndms = max(_Hps.Pndms, 1);
	SpecMax = _Hps.SpecMax;
	SpecMin = _Hps.SpecMin;
	MaxStep = max(_Hps.MaxStep, 1);

	_callback = _ProgressCallback;

	if (!_Hps.Cluster.Type.empty())
	{
		ClusterCenterSize = _Hps.Cluster.ClusterCenterSize;
		try
		{
			Cluster = MoeVoiceStudioCluster::GetMoeVSCluster(_Hps.Cluster.Type, _Hps.Cluster.Path, HiddenUnitKDims, ClusterCenterSize);
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
		hubert = new Ort::Session(*env, _Hps.HubertPath.c_str(), *session_options);
		if (!_Hps.DiffusionSvc.Encoder.empty())
		{
			encoder = new Ort::Session(*env, _Hps.DiffusionSvc.Encoder.c_str(), *session_options);
			denoise = new Ort::Session(*env, _Hps.DiffusionSvc.Denoise.c_str(), *session_options);
			pred = new Ort::Session(*env, _Hps.DiffusionSvc.Pred.c_str(), *session_options);
			after = new Ort::Session(*env, _Hps.DiffusionSvc.After.c_str(), *session_options);
			if (!_Hps.DiffusionSvc.Alpha.empty())
				alpha = new Ort::Session(*env, _Hps.DiffusionSvc.Alpha.c_str(), *session_options);
		}
		else
			diffSvc = new Ort::Session(*env, _Hps.DiffusionSvc.DiffSvc.c_str(), *session_options);

		if (!_Hps.DiffusionSvc.Naive.empty())
			naive = new Ort::Session(*env, _Hps.DiffusionSvc.Naive.c_str(), *session_options);

		logger.log(L"[Info] DiffSvc Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		Destory();
		LibDLVoiceCodecThrow(_exception.what())
	}

	MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Others _others_param;
	_others_param.Memory = *memory_info;

	try
	{
		_TensorExtractor = GetTensorExtractor(DiffSvcVersion, 48000, _samplingRate, HopSize, EnableCharaMix, EnableVolume, HiddenUnitKDims, SpeakerCount, _others_param);
	}
	catch (std::exception& e)
	{
		Destory();
		LibDLVoiceCodecThrow(e.what())
	}
}

void DiffusionSvc::load(const std::map<std::string, std::wstring>& _PathDict, const MJson& _Config, const ProgressCallback& _ProgressCallback)
{
	//Check SamplingRate
	if (_Config["Rate"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Rate\" (SamplingRate)")
	if (_Config["Rate"].IsInt() || _Config["Rate"].IsInt64())
		_samplingRate = _Config["Rate"].GetInt();
	else
		LibDLVoiceCodecThrow("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64")

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	if (_Config["MelBins"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"MelBins\" (MelBins)")
	if (_Config["MelBins"].IsInt() || _Config["MelBins"].IsInt64())
		melBins = _Config["MelBins"].GetInt();
	else
		LibDLVoiceCodecThrow("[Error] Field \"MelBins\" (MelBins) Must Be Int/Int64")

	if (!(_Config["Hop"].IsInt() || _Config["Hop"].IsInt64()))
		LibDLVoiceCodecThrow("[Error] Hop Must Be Int")
	HopSize = _Config["Hop"].GetInt();

	if (HopSize < 1)
		LibDLVoiceCodecThrow("[Error] Hop Must > 0")

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

	if (_Config.HasMember("SpecMax") && _Config["SpecMax"].IsDouble())
		SpecMax = _Config["SpecMax"].GetFloat();
	else
		logger.log(L"[Warn] Missing Field \"SpecMax\", Use Default Value (2)");

	if (_Config.HasMember("SpecMin") && _Config["SpecMin"].IsDouble())
		SpecMin = _Config["SpecMin"].GetFloat();
	else
		logger.log(L"[Warn] Missing Field \"SpecMin\", Use Default Value (-12)");

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
			Cluster = MoeVoiceStudioCluster::GetMoeVSCluster(clus, _PathDict.at("Cluster"), HiddenUnitKDims, ClusterCenterSize);
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
		hubert = new Ort::Session(*env, _PathDict.at("DiffHubert").c_str(), *session_options);
		if (_PathDict.find("Encoder")!= _PathDict.end() && _waccess(_PathDict.at("Encoder").c_str(), 0) != -1)
		{
			encoder = new Ort::Session(*env, _PathDict.at("Encoder").c_str(), *session_options);
			denoise = new Ort::Session(*env, _PathDict.at("DenoiseFn").c_str(), *session_options);
			pred = new Ort::Session(*env, _PathDict.at("NoisePredictor").c_str(), *session_options);
			after = new Ort::Session(*env, _PathDict.at("AfterProcess").c_str(), *session_options);
			if (_PathDict.find("Alphas") != _PathDict.end() && _waccess(_PathDict.at("Alphas").c_str(), 0) != -1)
				alpha = new Ort::Session(*env, _PathDict.at("Alphas").c_str(), *session_options);
		}
		else
			diffSvc = new Ort::Session(*env, _PathDict.at("DiffSvc").c_str(), *session_options);

		if (_PathDict.find("Naive") != _PathDict.end() && _waccess(_PathDict.at("Naive").c_str(), 0) != -1)
			naive = new Ort::Session(*env, _PathDict.at("Naive").c_str(), *session_options);

		logger.log(L"[Info] DiffSvc Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		Destory();
		LibDLVoiceCodecThrow(_exception.what())
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
		LibDLVoiceCodecThrow(e.what())
	}
}

std::vector<int16_t> DiffusionSvc::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	_TensorExtractor->SetSrcSamplingRates(_InferParams.SrcSamplingRate);
	logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Start!");
	std::vector<int16_t> _data;
	size_t total_audio_size = 0;
	for (const auto& data_size : _Slice.Slices)
		total_audio_size += data_size.OrgLen;
	_data.reserve(size_t(double(total_audio_size) * 1.5));

	auto speedup = (int64_t)_InferParams.Pndm;
	auto step = (int64_t)_InferParams.Step;
	if (step > MaxStep) step = MaxStep;
	if (speedup >= step) speedup = step / 5;
	if (speedup == 0) speedup = 1;
	const auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;

	if (diffSvc)
		_callback(0, _Slice.Slices.size());
	else
		_callback(0, _Slice.Slices.size() * RealDiffSteps);
	size_t process = 0;
	for (const auto& CurSlice : _Slice.Slices)
	{
		const auto InferDurTime = clock();
		const auto CurRtn = SliceInference(CurSlice, _InferParams, process);
		_data.insert(_data.end(), CurRtn.data(), CurRtn.data() + CurRtn.size());
		if (CurSlice.IsNotMute)
			logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Segment[" + std::to_wstring(process) + L"] Finished! Segment Use Time: " + std::to_wstring(clock() - InferDurTime) + L"ms, Segment Duration: " + std::to_wstring((size_t)CurSlice.OrgLen * 1000ull / _InferParams.SrcSamplingRate) + L"ms");
		else
		{
			if (!diffSvc)
			{
				process += RealDiffSteps;
				_callback(process, _Slice.Slices.size() * RealDiffSteps);
			}
			logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Jump Empty Segment[" + std::to_wstring(process) + L"]!");
		}
		if (diffSvc)
			_callback(++process, _Slice.Slices.size());
	}

	logger.log(L"[Inferring] \"" + _Slice.Path + L"\" Finished");
	return _data;
}

std::vector<int16_t> DiffusionSvc::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process) const
{
	_TensorExtractor->SetSrcSamplingRates(_InferParams.SrcSamplingRate);
	std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);
	auto speedup = (int64_t)_InferParams.Pndm;
	auto step = (int64_t)_InferParams.Step;
	if (step > MaxStep) step = MaxStep;
	if (speedup >= step) speedup = step / 5;
	if (speedup == 0) speedup = 1;

	if (_Slice.IsNotMute)
	{
		auto RawWav = InferTools::InterpResample(_Slice.Audio, (int)(_InferParams.SrcSamplingRate), 16000, 32768.0f);
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
			LibDLVoiceCodecThrow((std::string("Locate: hubert\n") + e.what()))
		}
		const auto HubertSize = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetElementCount();
		const auto HubertOutPutData = HubertOutPuts[0].GetTensorMutableData<float>();
		auto HubertOutPutShape = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetShape();
		HubertInputTensors.clear();
		if (HubertOutPutShape[2] != HiddenUnitKDims)
			LibDLVoiceCodecThrow("HiddenUnitKDims UnMatch")

		std::vector srcHiddenUnits(HubertOutPutData, HubertOutPutData + HubertSize);
		int64_t SpeakerIdx = _InferParams.SpeakerId;
		if (SpeakerIdx >= SpeakerCount)
			SpeakerIdx = SpeakerCount;
		if (SpeakerIdx < 0)
			SpeakerIdx = 0;

		const auto max_cluster_size = int64_t((size_t)HubertOutPutShape[1] * src_audio_length / RawWav.size());
		if (EnableCluster && _InferParams.ClusterRate > 0.001f)
		{
			const auto pts = Cluster->find(srcHiddenUnits.data(), long(SpeakerIdx), max_cluster_size);
			for (int64_t indexs = 0; indexs < max_cluster_size * HiddenUnitKDims; ++indexs)
				srcHiddenUnits[indexs] = srcHiddenUnits[indexs] * (1.f - _InferParams.ClusterRate) + pts[indexs] * _InferParams.ClusterRate;
		}
		std::vector<Ort::Value> finaOut;
		std::vector<Ort::Value> DiffOut;
		if (diffSvc)
		{
			const auto HubertLen = int64_t(HubertSize) / HiddenUnitKDims;
			const int64_t F0Shape[] = { 1, int64_t(_Slice.Audio.size() * _samplingRate / (int)(_InferParams.SrcSamplingRate) / HopSize) };
			const int64_t HiddenUnitShape[] = { 1, HubertLen, HiddenUnitKDims };
			constexpr int64_t CharaEmbShape[] = { 1 };
			int64_t speedData[] = { Pndms };
			auto srcF0Data = InferTools::InterpFunc(_Slice.F0, long(_Slice.F0.size()), long(F0Shape[1]));
			for (auto& it : srcF0Data)
				it *= (float)pow(2.0, static_cast<double>(_InferParams.Keys) / 12.0);
			auto InterpedF0 = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetInterpedF0log(srcF0Data, true);
			auto alignment = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetAligments(F0Shape[1], HubertLen);
			std::vector<Ort::Value> TensorsInp;

			int64_t Chara[] = { SpeakerIdx };

			TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, srcHiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
			TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
			TensorsInp.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Chara, 1, CharaEmbShape, 1));
			TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));

			std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
			long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };
			if (!naive)
			{
				for (auto& it : initial_noise)
					it = normal(gen) * _InferParams.NoiseScale;
				TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
			}
			else
				MoeVSNotImplementedError

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
				LibDLVoiceCodecThrow((std::string("Locate: Diff\n") + e2.what()))
			}
			Ort::Session* Vocoder = GlobalVocoder;
			if (_InferParams._VocoderModel)
				Vocoder = static_cast<Ort::Session*>(_InferParams._VocoderModel);
			try
			{
				finaOut = Vocoder->Run(Ort::RunOptions{ nullptr },
					nsfInput.data(),
					DiffOut.data(),
					Vocoder->GetInputCount(),
					nsfOutput.data(),
					nsfOutput.size());
			}
			catch (Ort::Exception& e3)
			{
				LibDLVoiceCodecThrow((std::string("Locate: Nsf\n") + e3.what()))
			}
		}
		else
		{
			MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::InferParams _Inference_Params;
			_Inference_Params.AudioSize = _Slice.Audio.size();
			_Inference_Params.Chara = SpeakerIdx;
			_Inference_Params.NoiseScale = _InferParams.NoiseScale;
			_Inference_Params.DDSPNoiseScale = _InferParams.DDSPNoiseScale;
			_Inference_Params.Seed = int(_InferParams.Seed);
			_Inference_Params.upKeys = _InferParams.Keys;

			MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Inputs InputTensors;

			if (_cur_execution_provider == ExecutionProviders::CUDA && NeedPadding)
			{
				auto CUDAF0 = _Slice.F0;
				auto CUDAVolume = _Slice.Volume;
				auto CUDASpeaker = _Slice.Speaker;
				const auto src_src_audio_length = _Slice.Audio.size();
				const size_t WavPaddedSize = ((src_src_audio_length / (int)(_InferParams.SrcSamplingRate)) + 1) * (int)(_InferParams.SrcSamplingRate);
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
				InputTensors = _TensorExtractor->Extract(srcHiddenUnits, _Slice.F0, _Slice.Volume, _Slice.Speaker, _Inference_Params);

			std::vector<Ort::Value> EncoderOut;
			try {
				EncoderOut = encoder->Run(Ort::RunOptions{ nullptr },
					InputTensors.InputNames,
					InputTensors.Tensor.data(),
					min(InputTensors.Tensor.size(), encoder->GetInputCount()),
					InputTensors.OutputNames,
					encoder->GetOutputCount());
			}
			catch (Ort::Exception& e1)
			{
				LibDLVoiceCodecThrow((std::string("Locate: encoder\n") + e1.what()))
			}
			if (EncoderOut.size() == 1)
				EncoderOut.emplace_back(Ort::Value::CreateTensor(*memory_info, InputTensors.Data.F0.data(), InputTensors.Data.FrameShape[1], InputTensors.Data.FrameShape.data(), 2));

			std::vector<Ort::Value> DenoiseInTensors;
			DenoiseInTensors.emplace_back(std::move(EncoderOut[0]));

			std::vector<float> initial_noise(melBins * InputTensors.Data.FrameShape[1], 0.0);
			long long noise_shape[4] = { 1,1,melBins,InputTensors.Data.FrameShape[1] };
			if (EncoderOut.size() == 3)
				DenoiseInTensors.emplace_back(std::move(EncoderOut[2]));
			else if (!naive)
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
					LibDLVoiceCodecThrow((std::string("Locate: naive\n") + e1.what()))
				}
				DenoiseInTensors.emplace_back(std::move(NaiveOut[0]));
			}

			auto PredOut = MoeVSSampler::GetMoeVSSampler((!alpha ? L"Pndm" : _InferParams.Sampler), alpha, denoise, pred, melBins, _callback, memory_info)->Sample(DenoiseInTensors, step, speedup, _InferParams.NoiseScale, _InferParams.Seed, _Process);

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
				LibDLVoiceCodecThrow((std::string("Locate: pred\n") + e1.what()))
			}
			DiffOut.emplace_back(std::move(EncoderOut[1]));
			Ort::Session* Vocoder = GlobalVocoder;
			if (_InferParams._VocoderModel)
				Vocoder = static_cast<Ort::Session*>(_InferParams._VocoderModel);
			try
			{
				finaOut = Vocoder->Run(Ort::RunOptions{ nullptr },
					nsfInput.data(),
					DiffOut.data(),
					Vocoder->GetInputCount(),
					nsfOutput.data(),
					nsfOutput.size());
			}
			catch (Ort::Exception& e3)
			{
				LibDLVoiceCodecThrow((std::string("Locate: Nsf\n") + e3.what()))
			}
		}

		auto DiffOutputAudioSize = finaOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
		std::vector<int16_t> DiffPCMOutput(DiffOutputAudioSize);
		{
			auto DiffOutputAudioData = finaOut[0].GetTensorData<float>();
			auto OutputAudioData = DiffPCMOutput.data();
			const auto OutputAudioEnd = OutputAudioData + DiffPCMOutput.size();
			while (OutputAudioData != OutputAudioEnd)
				*(OutputAudioData++) = (int16_t)(Clamp(*(DiffOutputAudioData++)) * 32766.f);
		}
		const auto dstWavLen = (_Slice.OrgLen * int64_t(_samplingRate)) / (int)(_InferParams.SrcSamplingRate);
		DiffPCMOutput.resize(dstWavLen);
		return DiffPCMOutput;
	}
	const auto len = size_t(_Slice.OrgLen * int64_t(_samplingRate) / (int)(_InferParams.SrcSamplingRate));
	return { len, 0i16, std::allocator<int16_t>() };
}

std::vector<std::wstring> DiffusionSvc::Inference(std::wstring& _Paths,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
	const InferTools::SlicerSettings& _SlicerSettings) const
{
	_TensorExtractor->SetSrcSamplingRates(_InferParams.SrcSamplingRate);
	std::vector<std::wstring> _Lens = GetOpenFileNameMoeVS();
	std::vector<std::wstring> AudioFolders;
	for (auto& path : _Lens)
	{
		path = std::regex_replace(path, std::wregex(L"\\\\"), L"/");
		auto PCMData = AudioPreprocess().codec(path, (int)(_InferParams.SrcSamplingRate));
		auto SlicePos = SliceAudio(PCMData, _SlicerSettings);
		auto Audio = GetAudioSlice(PCMData, SlicePos, _SlicerSettings);
		Audio.Path = path;
		PreProcessAudio(Audio);
		std::vector<int16_t> _data = SliceInference(Audio, _InferParams);

		std::wstring OutFolder = GetCurrentFolder() + L"/Outputs/" + path.substr(path.rfind(L'/') + 1, path.rfind(L'.') - path.rfind(L'/') - 1);
		int64_t SpeakerIdx = _InferParams.SpeakerId;
		if (SpeakerIdx >= SpeakerCount)
			SpeakerIdx = SpeakerCount;
		if (SpeakerIdx < 0)
			SpeakerIdx = 0;
		OutFolder += L"-Params-(-NoiseScale=" +
			std::to_wstring(_InferParams.NoiseScale) +
			L"-Speaker=" +
			(EnableCharaMix ? std::wstring(L"SpeakerMix") : std::to_wstring(SpeakerIdx)) +
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
	_TensorExtractor->SetSrcSamplingRates(_InferParams.SrcSamplingRate);
	if (diffSvc || DiffSvcVersion != L"DiffusionSvc")
		return PCMData;

	auto hubertin = InferTools::InterpResample<float>(PCMData, srcSr, 16000);
	int64_t SpeakerIdx = _InferParams.SpeakerId;
	if (SpeakerIdx >= SpeakerCount)
		SpeakerIdx = SpeakerCount;
	if (SpeakerIdx < 0)
		SpeakerIdx = 0;
	std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);

	const int64_t inputShape[3] = { 1i64,1i64,(int64_t)hubertin.size() };
	std::vector<Ort::Value> inputTensorshu;
	inputTensorshu.emplace_back(Ort::Value::CreateTensor(*memory_info, hubertin.data(), hubertin.size(), inputShape, 3));
	std::vector<Ort::Value> hubertOut;

	auto speedup = (int64_t)_InferParams.Pndm;
	auto step = (int64_t)_InferParams.Step;
	if (step > MaxStep) step = MaxStep;
	if (speedup >= step) speedup = step / 5;
	if (speedup == 0) speedup = 1;
	const auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;

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
		LibDLVoiceCodecThrow((std::string("Locate: hubert\n") + e.what()))
	}
	const auto HubertSize = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
	const auto HubertOutPutData = hubertOut[0].GetTensorMutableData<float>();
	const auto HubertOutPutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();
	inputTensorshu.clear();
	if (HubertOutPutShape[2] != HiddenUnitKDims)
		LibDLVoiceCodecThrow("HiddenUnitKDims UnMatch")

	std::vector HiddenUnits(HubertOutPutData, HubertOutPutData + HubertSize);

	if (EnableCluster && _InferParams.ClusterRate > 0.001f)
	{
		const auto clus_size = HubertOutPutShape[1];
		const auto pts = Cluster->find(HiddenUnits.data(), long(SpeakerIdx), clus_size);
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
	int64_t CharaEmb[] = { SpeakerIdx };

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
		SpkMap = _TensorExtractor->GetCurrectSpkMixData(std::vector<std::vector<float>>(), F0Shape[1], SpeakerIdx);
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

	const std::vector OutputNamesEncoder = { "mel_pred", "f0_pred", "init_noise" };

	std::vector<Ort::Value> EncoderOut;
	try {
		EncoderOut = encoder->Run(Ort::RunOptions{ nullptr },
			InputNamesEncoder.data(),
			EncoderTensors.data(),
			min(EncoderTensors.size(), encoder->GetInputCount()),
			OutputNamesEncoder.data(),
			encoder->GetOutputCount());
	}
	catch (Ort::Exception& e1)
	{
		LibDLVoiceCodecThrow((std::string("Locate: encoder\n") + e1.what()))
	}
	if (EncoderOut.size() == 1)
		EncoderOut.emplace_back(Ort::Value::CreateTensor(*memory_info, F0Data.data(), F0Shape[1], F0Shape, 2));

	std::vector<Ort::Value> DenoiseInTensors;
	DenoiseInTensors.emplace_back(std::move(EncoderOut[0]));

	std::vector<float> initial_noise(melBins * F0Shape[1], 0.0);
	long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };
	if (EncoderOut.size() == 3)
		DenoiseInTensors.emplace_back(std::move(EncoderOut[2]));
	else if (!naive)
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
			LibDLVoiceCodecThrow((std::string("Locate: naive\n") + e1.what()))
		}
		DenoiseInTensors.emplace_back(std::move(NaiveOut[0]));
	}

	size_t process = 0;

	auto PredOut = MoeVSSampler::GetMoeVSSampler((!alpha ? L"Pndm" : _InferParams.Sampler), alpha, denoise, pred, melBins, _callback, memory_info)->Sample(DenoiseInTensors, step, speedup, _InferParams.NoiseScale, _InferParams.Seed, process);

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
		LibDLVoiceCodecThrow((std::string("Locate: pred\n") + e1.what()))
	}
	DiffOut.emplace_back(std::move(EncoderOut[1]));
	Ort::Session* Vocoder = GlobalVocoder;
	if (_InferParams._VocoderModel)
		Vocoder = static_cast<Ort::Session*>(_InferParams._VocoderModel);
	try
	{
		finaOut = Vocoder->Run(Ort::RunOptions{ nullptr },
			nsfInput.data(),
			DiffOut.data(),
			Vocoder->GetInputCount(),
			nsfOutput.data(),
			nsfOutput.size());
	}
	catch (Ort::Exception& e3)
	{
		LibDLVoiceCodecThrow((std::string("Locate: Nsf\n") + e3.what()))
	}

	auto DiffOutputAudioSize = finaOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
	std::vector<int16_t> DiffPCMOutput(DiffOutputAudioSize);
	{
		auto DiffOutputAudioData = finaOut[0].GetTensorData<float>();
		auto OutputAudioData = DiffPCMOutput.data();
		const auto OutputAudioEnd = OutputAudioData + DiffPCMOutput.size();
		while (OutputAudioData != OutputAudioEnd)
			*(OutputAudioData++) = (int16_t)(Clamp(*(DiffOutputAudioData++)) * 32766.f);
	}
	return DiffPCMOutput;
}

void DiffusionSvc::NormMel(std::vector<float>& MelSpec) const
{
	for (auto& it : MelSpec)
		it = (it - SpecMin) / (SpecMax - SpecMin) * 2 - 1;
}

std::vector<int16_t> DiffusionSvc::ShallowDiffusionInference(
	std::vector<float>& _16KAudioHubert,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
	std::pair<std::vector<float>, int64_t>& _Mel,
	const std::vector<float>& _SrcF0,
	const std::vector<float>& _SrcVolume,
	const std::vector<std::vector<float>>& _SrcSpeakerMap,
	size_t& Process,
	int64_t SrcSize
) const
{
	_TensorExtractor->SetSrcSamplingRates(_InferParams.SrcSamplingRate);
	if (diffSvc || DiffSvcVersion != L"DiffusionSvc")
		LibDLVoiceCodecThrow("ShallowDiffusion Only Support DiffusionSvc Model")

	auto speedup = (int64_t)_InferParams.Pndm;
	auto step = (int64_t)_InferParams.Step;
	if (step > MaxStep) step = MaxStep;
	if (speedup >= step) speedup = step / 5;
	if (speedup == 0) speedup = 1;

	std::vector<const char*> InputNamesEncoder;
	const auto _Mel_Size = _Mel.second;

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
		LibDLVoiceCodecThrow((std::string("Locate: hubert\n") + e.what()))
	}

	int64_t SpeakerIdx = _InferParams.SpeakerId;
	if (SpeakerIdx >= SpeakerCount)
		SpeakerIdx = SpeakerCount;
	if (SpeakerIdx < 0)
		SpeakerIdx = 0;

	const auto HubertLength = HubertOutputTensors[0].GetTensorTypeAndShapeInfo().GetShape()[1];
	const int64_t FrameShape[] = { 1, _Mel_Size };
	const int64_t CharaMixShape[] = { _Mel_Size, SpeakerCount };
	constexpr int64_t OneShape[] = { 1 };
	int64_t CharaEmb[] = { SpeakerIdx };

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
		LibDLVoiceCodecThrow((std::string("Locate: encoder\n") + e1.what()))
	}

	std::vector<Ort::Value> DenoiseInTensors;
	DenoiseInTensors.emplace_back(std::move(EncoderOut[0]));

	long long noise_shape[4] = { 1,1,melBins,_Mel_Size };

	NormMel(_Mel.first);

	/*std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);
	std::vector<float> initial_noise(melBins * _Mel_Size, 0.0);
	for (auto& it : initial_noise)
		it = normal(gen) * _InferParams.NoiseScale;
	DenoiseInTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));*/

	DenoiseInTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, _Mel.first.data(), _Mel.first.size(), noise_shape, 4));

	auto PredOut = MoeVSSampler::GetMoeVSSampler((!alpha ? L"Pndm" : _InferParams.Sampler), alpha, denoise, pred, melBins, _callback, memory_info)->Sample(DenoiseInTensors, step, speedup, _InferParams.NoiseScale, _InferParams.Seed, Process);

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
		LibDLVoiceCodecThrow((std::string("Locate: pred\n") + e1.what()))
	}
	DiffOut.emplace_back(std::move(EncoderTensors[2]));
	Ort::Session* Vocoder = GlobalVocoder;
	if (_InferParams._VocoderModel)
		Vocoder = static_cast<Ort::Session*>(_InferParams._VocoderModel);
	try
	{
		finaOut = Vocoder->Run(Ort::RunOptions{ nullptr },
			nsfInput.data(),
			DiffOut.data(),
			Vocoder->GetInputCount(),
			nsfOutput.data(),
			nsfOutput.size());
	}
	catch (Ort::Exception& e3)
	{
		LibDLVoiceCodecThrow((std::string("Locate: Nsf\n") + e3.what()))
	}

	auto DiffOutputAudioSize = finaOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
	std::vector<int16_t> DiffPCMOutput(DiffOutputAudioSize);
	{
		auto DiffOutputAudioData = finaOut[0].GetTensorData<float>();
		auto OutputAudioData = DiffPCMOutput.data();
		const auto OutputAudioEnd = OutputAudioData + DiffPCMOutput.size();
		while (OutputAudioData != OutputAudioEnd)
			*(OutputAudioData++) = (int16_t)(Clamp(*(DiffOutputAudioData++)) * 32766.f);
	}
	const auto dstWavLen = (SrcSize * int64_t(_samplingRate)) / (int)(_InferParams.SrcSamplingRate);
	DiffPCMOutput.resize(dstWavLen);
	return DiffPCMOutput;
}

void StaticNormMel(std::vector<float>& MelSpec, float SpecMin = -12, float SpecMax = 2)
{
	for (auto& it : MelSpec)
		it = (it - SpecMin) / (SpecMax - SpecMin) * 2 - 1;
}

std::vector<int16_t> VocoderInfer(std::vector<float>& Mel, std::vector<float>& F0, int64_t MelBins, int64_t MelSize, const Ort::MemoryInfo* Mem, void* _VocoderModel)
{
	const int64_t MelShape[] = { 1i64,MelBins,MelSize };
	const int64_t FrameShape[] = { 1,MelSize };
	std::vector<Ort::Value> Tensors;
	Tensors.emplace_back(Ort::Value::CreateTensor(
		*Mem,
		Mel.data(),
		Mel.size(),
		MelShape,
		3)
	);
	Tensors.emplace_back(Ort::Value::CreateTensor(
		*Mem,
		F0.data(),
		FrameShape[1],
		FrameShape,
		2)
	);
	const std::vector nsfInput = { "c", "f0" };
	const std::vector nsfOutput = { "audio" };
	Ort::Session* Vocoder = GlobalVocoder;
	if (_VocoderModel)
		Vocoder = static_cast<Ort::Session*>(_VocoderModel);
	Tensors = Vocoder->Run(Ort::RunOptions{ nullptr },
		nsfInput.data(),
		Tensors.data(),
		Vocoder->GetInputCount(),
		nsfOutput.data(),
		nsfOutput.size());
	const auto AudioSize = Tensors[0].GetTensorTypeAndShapeInfo().GetShape()[2];
	std::vector<int16_t> Audio(AudioSize, 0);
	for (int64_t it = 0; it < AudioSize; it++)
		Audio[it] = static_cast<int16_t>(DiffusionSvc::Clamp(Tensors[0].GetTensorData<float>()[it]) * 32766.0f);
	return Audio;
}

MoeVoiceStudioCoreEnd
