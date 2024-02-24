#include "../../header/Models/VitsSvc.hpp"
#include "../../header/InferTools/AvCodec/AvCodeResample.h"
#include "../../header/InferTools/F0Extractor/F0ExtractorManager.hpp"
#include <random>
#include "../../header/Logger/MoeSSLogger.hpp"
#include "../../header/InferTools/TensorExtractor/TensorExtractorManager.hpp"
#include <regex>

MoeVoiceStudioCoreHeader
void VitsSvc::Destory()
{
	delete hubert;
	hubert = nullptr;

	delete VitsSvcModel;
	VitsSvcModel = nullptr;
}

VitsSvc::~VitsSvc()
{
	logger.log(L"[Info] unloading VitsSvc Models");
	Destory();
	logger.log(L"[Info] VitsSvc Models unloaded");
}

VitsSvc::VitsSvc(const MJson& _Config, const ProgressCallback& _ProgressCallback,
	ExecutionProviders ExecutionProvider_,
	unsigned DeviceID_, unsigned ThreadCount_):
	SingingVoiceConversion(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioVitsSingingVoiceConversion");

	//Check Folder
	if (_Config["Folder"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"folder\" (Model Folder)")
	if (!_Config["Folder"].IsString())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Must Be String")
	const auto _folder = to_wide_string(_Config["Folder"].GetString());
	const auto cluster_folder = GetCurrentFolder() + L"/Models/" + _folder;
	if (_folder.empty())
		LibDLVoiceCodecThrow("[Error] Field \"folder\" (Model Folder) Can Not Be Empty")
	const std::wstring _path = GetCurrentFolder() + L"/Models/" + _folder + L"/" + _folder;

	if (_Config["Hubert"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Hubert\" (Hubert Folder)")
	if (!_Config["Hubert"].IsString())
		LibDLVoiceCodecThrow("[Error] Field \"Hubert\" (Hubert Folder) Must Be String")
	const std::wstring HuPath = to_wide_string(_Config["Hubert"].GetString());
	if (HuPath.empty())
		LibDLVoiceCodecThrow("[Error] Field \"Hubert\" (Hubert Folder) Can Not Be Empty")

	std::map<std::string, std::wstring> _PathDict;
	_PathDict["Cluster"] = cluster_folder;
	_PathDict["Hubert"] = (GetCurrentFolder() + L"/hubert/" + HuPath + L".onnx");
	_PathDict["RVC"] = (_path + L"_RVC.onnx");
	_PathDict["SoVits"] = (_path + L"_SoVits.onnx");
	if (_Config["ShallowDiffusion"].IsString())
	{
		const std::wstring ShallowDiffusionConf = GetCurrentFolder() + L"/Models/" + to_wide_string(_Config["ShallowDiffusion"].GetString()) + L".json";
		_PathDict["ShallowDiffusionConfig"] = ShallowDiffusionConf;
		if (_Config["MelOperator"].IsString())
			_PathDict["MelOperator"] = GetCurrentFolder() + L"/MelOps" + to_wide_string(_Config["MelOperator"].GetString()) + L".onnx";
		else
			_PathDict["MelOperator"] = GetCurrentFolder() + L"/MelOps.onnx";
	}

	load(_PathDict, _Config, _ProgressCallback, ExecutionProvider_, DeviceID_, ThreadCount_, true);
}

VitsSvc::VitsSvc(const std::map<std::string, std::wstring>& _PathDict, const MJson& _Config, const ProgressCallback& _ProgressCallback,
	ExecutionProviders ExecutionProvider_,
	unsigned DeviceID_, unsigned ThreadCount_) :
	SingingVoiceConversion(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioVitsSingingVoiceConversion");
	load(_PathDict, _Config, _ProgressCallback, ExecutionProvider_, DeviceID_, ThreadCount_, false);
}

VitsSvc::VitsSvc(const Hparams& _Hps, const ProgressCallback& _ProgressCallback, ExecutionProviders ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_) :
	SingingVoiceConversion(ExecutionProvider_, DeviceID_, ThreadCount_)
{
	MoeVSClassName(L"MoeVoiceStudioVitsSingingVoiceConversion");

	_samplingRate = max(_Hps.SamplingRate, 2000);
	HopSize = max(_Hps.HopSize, 1);
	HiddenUnitKDims = max(_Hps.HiddenUnitKDims, 1);
	SpeakerCount = max(_Hps.SpeakerCount, 1);
	EnableVolume = _Hps.EnableVolume;
	EnableCharaMix = _Hps.EnableCharaMix;
	VitsSvcVersion = _Hps.TensorExtractor;

#ifdef MOEVSDMLPROVIDER
	if (ExecutionProvider_ == ExecutionProviders::DML && VitsSvcVersion == L"SoVits4.0-DDSP")
		LibDLVoiceCodecThrow("[Error] DirectXMl Not Support SoVits4.0V2, Please Use Cuda Or Cpu")
#endif

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

	try
	{
		logger.log(L"[Info] loading VitsSvcModel Models");
		hubert = new Ort::Session(*env, _Hps.HubertPath.c_str(), *session_options);
		VitsSvcModel = new Ort::Session(*env, _Hps.VitsSvc.VitsSvc.c_str(), *session_options);
		logger.log(L"[Info] VitsSvcModel Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		Destory();
		LibDLVoiceCodecThrow(_exception.what())
	}

	if (VitsSvcModel->GetInputCount() == 4 && VitsSvcVersion != L"SoVits3.0")
		VitsSvcVersion = L"SoVits2.0";

	MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Others _others_param;
	_others_param.Memory = *memory_info;
	try
	{
		_TensorExtractor = GetTensorExtractor(VitsSvcVersion, 48000, _samplingRate, HopSize, EnableCharaMix, EnableVolume, HiddenUnitKDims, SpeakerCount, _others_param);
	}
	catch (std::exception& e)
	{
		Destory();
		LibDLVoiceCodecThrow(e.what())
	}
}

void VitsSvc::load(const std::map<std::string, std::wstring>& _PathDict, const MJson& _Config, const ProgressCallback& _ProgressCallback, ExecutionProviders ExecutionProvider_, unsigned DeviceID_, unsigned ThreadCount_, bool MoeVoiceStudioFrontEnd)
{

	//Check SamplingRate
	if (_Config["Rate"].IsNull())
		LibDLVoiceCodecThrow("[Error] Missing field \"Rate\" (SamplingRate)")
	if (_Config["Rate"].IsInt() || _Config["Rate"].IsInt64())
		_samplingRate = _Config["Rate"].GetInt();
	else
		LibDLVoiceCodecThrow("[Error] Field \"Rate\" (SamplingRate) Must Be Int/Int64")

	logger.log(L"[Info] Current Sampling Rate is" + std::to_wstring(_samplingRate));

	if (!_Config["SoVits3"].IsNull() && _Config["SoVits3"].GetBool())
		VitsSvcVersion = L"SoVits3.0";
	else if (!_Config["SoVits2"].IsNull() && _Config["SoVits2"].GetBool())
		VitsSvcVersion = L"SoVits2.0";
	else if (!_Config["SoVits2.0"].IsNull() && _Config["SoVits2.0"].GetBool())
		VitsSvcVersion = L"SoVits2.0";
	else if (!_Config["SoVits3.0"].IsNull() && _Config["SoVits3.0"].GetBool())
		VitsSvcVersion = L"SoVits3.0";
	else if (_Config["Type"].GetString() == std::string("RVC"))
		VitsSvcVersion = L"RVC";

	if (!_Config["SoVits4.0V2"].IsNull() && _Config["SoVits4.0V2"].GetBool())
		VitsSvcVersion = L"SoVits4.0-DDSP";

#ifdef MOEVSDMLPROVIDER
	if (ExecutionProvider_ == ExecutionProviders::DML && VitsSvcVersion == L"SoVits4.0-DDSP")
		LibDLVoiceCodecThrow("[Error] DirectXMl Not Support SoVits4.0V2, Please Use Cuda Or Cpu")
#endif

	if (!(_Config["Hop"].IsInt() || _Config["Hop"].IsInt64()))
		LibDLVoiceCodecThrow("[Error] Hop Must Exist And Must Be Int")
	HopSize = _Config["Hop"].GetInt();

	if (!(_Config["HiddenSize"].IsInt() || _Config["HiddenSize"].IsInt64()))
		logger.log(L"[Warn] Missing Field \"HiddenSize\", Use Default Value (256)");
	else
		HiddenUnitKDims = _Config["HiddenSize"].GetInt();

	if (!_Config["CharaMix"].IsBool())
		logger.log(L"[Warn] Missing Field \"CharaMix\", Use Default Value (False)");
	else
		EnableCharaMix = _Config["CharaMix"].GetBool();

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

	if (HopSize < 1)
		LibDLVoiceCodecThrow("[Error] Hop Must > 0")

	if (_Config["Volume"].IsBool())
		EnableVolume = _Config["Volume"].GetBool();
	else
		logger.log(L"[Warn] Missing Field \"Volume\", Use Default Value (False)");

	if (_Config["Characters"].IsArray())
		SpeakerCount = int64_t(_Config["Characters"].Size());

	_callback = _ProgressCallback;

	//LoadModels
	try
	{
		logger.log(L"[Info] loading VitsSvcModel Models");
		hubert = new Ort::Session(*env, _PathDict.at("Hubert").c_str(), *session_options);
		if (VitsSvcVersion == L"RVC")
			VitsSvcModel = new Ort::Session(*env, _PathDict.at("RVC").c_str(), *session_options);
		else
			VitsSvcModel = new Ort::Session(*env, _PathDict.at("SoVits").c_str(), *session_options);
		logger.log(L"[Info] VitsSvcModel Models loaded");
	}
	catch (Ort::Exception& _exception)
	{
		Destory();
		LibDLVoiceCodecThrow(_exception.what())
	}

	if (VitsSvcModel->GetInputCount() == 4 && VitsSvcVersion != L"SoVits3.0")
		VitsSvcVersion = L"SoVits2.0";

	if (_Config["TensorExtractor"].IsString())
		VitsSvcVersion = to_wide_string(_Config["TensorExtractor"].GetString());

	MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Others _others_param;
	_others_param.Memory = *memory_info;
	try
	{
		_TensorExtractor = GetTensorExtractor(VitsSvcVersion, 48000, _samplingRate, HopSize, EnableCharaMix, EnableVolume, HiddenUnitKDims, SpeakerCount, _others_param);
	}
	catch (std::exception& e)
	{
		Destory();
		LibDLVoiceCodecThrow(e.what())
	}
}

std::vector<int16_t> VitsSvc::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcData& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Start!");
	std::vector<int16_t> _data;
	size_t total_audio_size = 0;
	for (const auto& data_size : _Slice.Slices)
		total_audio_size += data_size.OrgLen;
	_data.reserve(size_t(double(total_audio_size) * 1.5));
	_callback(0, _Slice.Slices.size());
	size_t process = 0;
	for (auto& CurSlice : _Slice.Slices)
	{
		const auto InferDurTime = clock();
		const auto CurRtn = SliceInference(CurSlice, _InferParams, process);
		_data.insert(_data.end(), CurRtn.data(), CurRtn.data() + CurRtn.size());
		if(CurSlice.IsNotMute)
			logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Segment[" + std::to_wstring(process) + L"] Finished! Segment Use Time: " + std::to_wstring(clock() - InferDurTime) + L"ms, Segment Duration: " + std::to_wstring((size_t)CurSlice.OrgLen * 1000ull / 48000ull) + L"ms");
		else
			logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L"\", Jump Empty Segment[" + std::to_wstring(process) + L"]!");
		_callback(++process, _Slice.Slices.size());
	}
	logger.log(L"[Inferring] \"" + _Slice.Path + L"\" Finished");
	return _data;
}

ShallowDiffusionData DataForDiffusion;

ShallowDiffusionData& GetDataForShallowDiffusion()
{
	return DataForDiffusion;
}

std::vector<int16_t> VitsSvc::SliceInference(const MoeVSProjectSpace::MoeVoiceStudioSvcSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, size_t& _Process) const
{
	DataForDiffusion._16KAudio.clear();
	DataForDiffusion.CUDAF0.clear();
	DataForDiffusion.CUDAVolume.clear();
	DataForDiffusion.CUDASpeaker.clear();
	DataForDiffusion.NeedPadding = false;
	if (_Slice.IsNotMute)
	{
		DataForDiffusion._16KAudio = InferTools::InterpResample(_Slice.Audio, 48000, 16000, 32768.0f);
		const auto src_audio_length = DataForDiffusion._16KAudio.size();
		bool NeedPadding = false;
		if (_cur_execution_provider == ExecutionProviders::CUDA)
		{
			NeedPadding = DataForDiffusion._16KAudio.size() % 16000;
			const size_t WavPaddedSize = DataForDiffusion._16KAudio.size() / 16000 + 1;
			if (NeedPadding)
				DataForDiffusion._16KAudio.resize(WavPaddedSize * 16000, 0.f);
		}

		const int64_t HubertInputShape[3] = { 1i64,1i64,(int64_t)DataForDiffusion._16KAudio.size() };
		std::vector<Ort::Value> HubertInputTensors, HubertOutPuts;
		HubertInputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, DataForDiffusion._16KAudio.data(), DataForDiffusion._16KAudio.size(), HubertInputShape, 3));
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
		if (HubertOutPutShape[2] != HiddenUnitKDims)
			LibDLVoiceCodecThrow("HiddenUnitKDims UnMatch")

		std::vector SrcHiddenUnits(HubertOutPutData, HubertOutPutData + HubertSize);

		int64_t SpeakerIdx = _InferParams.SpeakerId;
		if (SpeakerIdx >= SpeakerCount)
			SpeakerIdx = SpeakerCount;
		if (SpeakerIdx < 0)
			SpeakerIdx = 0;

		const auto max_cluster_size = int64_t((size_t)HubertOutPutShape[1] * src_audio_length / DataForDiffusion._16KAudio.size());
		if (EnableCluster && _InferParams.ClusterRate > 0.001f)
		{
			const auto pts = Cluster->find(SrcHiddenUnits.data(), long(SpeakerIdx), max_cluster_size);
			for (int64_t indexs = 0; indexs < max_cluster_size * HiddenUnitKDims; ++indexs)
				SrcHiddenUnits[indexs] = SrcHiddenUnits[indexs] * (1.f - _InferParams.ClusterRate) + pts[indexs] * _InferParams.ClusterRate;
		}

		MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::InferParams _Inference_Params;
		_Inference_Params.AudioSize = _Slice.Audio.size();
		_Inference_Params.Chara = SpeakerIdx;
		_Inference_Params.NoiseScale = _InferParams.NoiseScale;
		_Inference_Params.DDSPNoiseScale = _InferParams.DDSPNoiseScale;
		_Inference_Params.Seed = int(_InferParams.Seed);
		_Inference_Params.upKeys = _InferParams.Keys;

		MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Inputs InputTensors;

		DataForDiffusion.NeedPadding = NeedPadding;

		if (_cur_execution_provider == ExecutionProviders::CUDA && NeedPadding)
		{
			DataForDiffusion.CUDAF0 = _Slice.F0;
			DataForDiffusion.CUDAVolume = _Slice.Volume;
			DataForDiffusion.CUDASpeaker = _Slice.Speaker;
			const auto src_src_audio_length = _Slice.Audio.size();
			const size_t WavPaddedSize = ((src_src_audio_length / 48000) + 1) * 48000;
			const size_t AudioPadSize = WavPaddedSize - src_src_audio_length;
			const size_t PaddedF0Size = DataForDiffusion.CUDAF0.size() + (DataForDiffusion.CUDAF0.size() * AudioPadSize / src_src_audio_length);

			if (!DataForDiffusion.CUDAF0.empty()) DataForDiffusion.CUDAF0.resize(PaddedF0Size, 0.f);
			if (!DataForDiffusion.CUDAVolume.empty()) DataForDiffusion.CUDAVolume.resize(PaddedF0Size, 0.f);
			for (auto iSpeaker : DataForDiffusion.CUDASpeaker)
			{
				if (!iSpeaker.empty())
					iSpeaker.resize(PaddedF0Size, 0.f);
			}
			_Inference_Params.AudioSize = WavPaddedSize;
			InputTensors = _TensorExtractor->Extract(SrcHiddenUnits, DataForDiffusion.CUDAF0, DataForDiffusion.CUDAVolume, DataForDiffusion.CUDASpeaker, _Inference_Params);
		}
		else
			InputTensors = _TensorExtractor->Extract(SrcHiddenUnits, _Slice.F0, _Slice.Volume, _Slice.Speaker, _Inference_Params);


		std::vector<Ort::Value> finaOut;
		try
		{
			finaOut = VitsSvcModel->Run(Ort::RunOptions{ nullptr },
				InputTensors.InputNames,
				InputTensors.Tensor.data(),
				InputTensors.Tensor.size(),
				soVitsOutput.data(),
				soVitsOutput.size());
		}
		catch (Ort::Exception& e)
		{
			LibDLVoiceCodecThrow((std::string("Locate: VitsSvc\n") + e.what()))
		}

		const auto dstWavLen = (_Slice.OrgLen * int64_t(_samplingRate)) / 48000;
		/*if (shallow_diffusion && stft_operator && _InferParams.UseShallowDiffusion)
		{
			auto PCMAudioBegin = finaOut[0].GetTensorData<float>();
			auto PCMAudioEnd = PCMAudioBegin + finaOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
			auto MelSpec = MelExtractor(PCMAudioBegin, PCMAudioEnd);
			auto ShallowParam = _InferParams;
			ShallowParam.SrcSamplingRate = _samplingRate;
			auto ShallowDiffusionOutput = shallow_diffusion->ShallowDiffusionInference(
				RawWav,
				ShallowParam,
				std::move(MelSpec[0]),
				NeedPadding ? CUDAF0 : _Slice.F0,
				NeedPadding ? CUDAVolume : _Slice.Volume,
				NeedPadding ? CUDASpeaker : _Slice.Speaker
			);
			ShallowDiffusionOutput.resize(dstWavLen, 0);
			return ShallowDiffusionOutput;
		}*/

		const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
		std::vector<int16_t> TempVecWav = std::vector<int16_t>(dstWavLen, 0);
		if (shapeOut[2] < dstWavLen)
			for (int64_t bbb = 0; bbb < shapeOut[2]; bbb++)
				TempVecWav[bbb] = static_cast<int16_t>(Clamp(finaOut[0].GetTensorData<float>()[bbb]) * 32766.0f);
		else
			for (int64_t bbb = 0; bbb < dstWavLen; bbb++)
				TempVecWav[bbb] = static_cast<int16_t>(Clamp(finaOut[0].GetTensorData<float>()[bbb]) * 32766.0f);
		return TempVecWav;
	}
	//Mute clips
	const auto len = size_t(_Slice.OrgLen * int64_t(_samplingRate) / 48000);
	return { len, 0i16, std::allocator<int16_t>() };
}

std::vector<std::wstring> VitsSvc::Inference(std::wstring& _Paths,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
	const InferTools::SlicerSettings& _SlicerSettings) const
{
	std::vector<std::wstring> _Lens = GetOpenFileNameMoeVS();
	std::vector<std::wstring> AudioFolders;
	for (auto& path : _Lens)
	{
		path = std::regex_replace(path, std::wregex(L"\\\\"), L"/");
		auto PCMData = AudioPreprocess().codec(path, 48000);
		auto slicer_setting = _SlicerSettings;
		slicer_setting.SamplingRate = 48000;
		auto SlicePos = SliceAudio(PCMData, slicer_setting);
		auto Audio = GetAudioSlice(PCMData, SlicePos, slicer_setting);
		Audio.Path = path;
		PreProcessAudio(Audio, 48000, 512, _InferParams.F0Method);
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
		_data = InferTools::InterpResample(_data, _samplingRate, 48000, 1i16);
		InferTools::Wav::WritePCMData(48000, 1, _data, OutFolder);
	}
	return AudioFolders;
}

std::vector<int16_t> VitsSvc::InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	auto hubertin = InferTools::InterpResample<float>(PCMData, srcSr, 16000);
	int64_t SpeakerIdx = _InferParams.SpeakerId;
	if (SpeakerIdx >= SpeakerCount)
		SpeakerIdx = SpeakerCount;
	if (SpeakerIdx < 0)
		SpeakerIdx = 0;
	std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);
	float noise_scale = _InferParams.NoiseScale;
	float ddsp_noise_scale = _InferParams.DDSPNoiseScale;

	const int64_t inputShape[3] = { 1i64,1i64,(int64_t)hubertin.size() };
	std::vector<Ort::Value> inputTensorshu;
	inputTensorshu.emplace_back(Ort::Value::CreateTensor(*memory_info, hubertin.data(), hubertin.size(), inputShape, 3));
	std::vector<Ort::Value> hubertOut;

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
	auto HubertSize = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
	auto HubertOutPutData = hubertOut[0].GetTensorMutableData<float>();
	auto HubertOutPutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();
	inputTensorshu.clear();
	if (HubertOutPutShape[2] != HiddenUnitKDims)
		LibDLVoiceCodecThrow("HiddenUnitKDims UnMatch")

	std::vector HiddenUnitsSrc(HubertOutPutData, HubertOutPutData + HubertSize);

	if (EnableCluster && _InferParams.ClusterRate > 0.001f)
	{
		const auto clus_size = HubertOutPutShape[1];
		const auto pts = Cluster->find(HiddenUnitsSrc.data(), long(SpeakerIdx), clus_size);
		for (size_t indexs = 0; indexs < HiddenUnitsSrc.size(); ++indexs)
			HiddenUnitsSrc[indexs] = HiddenUnitsSrc[indexs] * (1.f - _InferParams.ClusterRate) + pts[indexs] * _InferParams.ClusterRate;
	}

	const auto HubertLen = int64_t(HubertSize) / HiddenUnitKDims;
	int64_t F0Shape[] = { 1, int64_t(PCMData.size() / HopSize) };
	int64_t HiddenUnitShape[] = { 1, HubertLen, HiddenUnitKDims };
	constexpr int64_t LengthShape[] = { 1 };
	int64_t CharaEmbShape[] = { 1 };
	int64_t CharaMixShape[] = { F0Shape[1], SpeakerCount };
	int64_t RandnShape[] = { 1, 192, F0Shape[1] };
	const int64_t IstftShape[] = { 1, 2048, F0Shape[1] };
	int64_t RandnCount = F0Shape[1] * 192;
	const int64_t IstftCount = F0Shape[1] * 2048;

	std::vector<float> RandnInput, IstftInput, UV, InterpedF0;
	std::vector<int64_t> alignment;
	int64_t XLength[1] = { HubertLen };
	std::vector<int64_t> Nsff0;
	//auto SoVitsInput = soVitsInput;
	int64_t Chara[] = { SpeakerIdx };
	std::vector<float> charaMix;

	const auto F0Extractor = MoeVSF0Extractor::GetF0Extractor(_InferParams.F0Method, _samplingRate, HopSize);

	auto srcF0Data = F0Extractor->ExtractF0(PCMData, PCMData.size() / HopSize);
	for (auto& ifo : srcF0Data)
		ifo *= (float)pow(2.0, static_cast<double>(_InferParams.Keys) / 12.0);
	std::vector<float> HiddenUnits;
	std::vector<float> F0Data;

	std::vector<Ort::Value> _Tensors;
	std::vector<const char*> SoVitsInput = soVitsInput;

	//Compatible with all versions
	if (VitsSvcVersion == L"SoVits3.0")
	{
		int64_t upSample = _samplingRate / 16000;
		HiddenUnits.reserve(HubertSize * (upSample + 1));
		for (int64_t itS = 0; itS < HiddenUnitShape[1]; ++itS)
			for (int64_t itSS = 0; itSS < upSample; ++itSS)
				HiddenUnits.insert(HiddenUnits.end(), HiddenUnitsSrc.begin() + itS * HiddenUnitKDims, HiddenUnitsSrc.begin() + (itS + 1) * HiddenUnitKDims);
		HiddenUnitShape[1] *= upSample;
		HubertSize *= upSample;
		F0Data = _TensorExtractor->GetInterpedF0(InferTools::InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1])));
		F0Shape[1] = HiddenUnitShape[1];
		XLength[0] = HiddenUnitShape[1];
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, F0Data.data(), F0Data.size(), F0Shape, 2));
	}
	else if (VitsSvcVersion == L"SoVits2.0")
	{
		HiddenUnits = std::move(HiddenUnitsSrc);
		F0Shape[1] = HiddenUnitShape[1];
		F0Data = InferTools::InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1]));
		XLength[0] = HiddenUnitShape[1];
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
		Nsff0 = _TensorExtractor->GetNSFF0(F0Data);
		_Tensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));
	}
	else if (VitsSvcVersion == L"RVC")
	{
		constexpr int64_t upSample = 2;
		HiddenUnits.reserve(HubertSize * (upSample + 1));
		for (int64_t itS = 0; itS < HiddenUnitShape[1]; ++itS)
			for (int64_t itSS = 0; itSS < upSample; ++itSS)
				HiddenUnits.insert(HiddenUnits.end(), HiddenUnitsSrc.begin() + itS * HiddenUnitKDims, HiddenUnitsSrc.begin() + (itS + 1) * HiddenUnitKDims);
		HiddenUnitShape[1] *= upSample;
		HubertSize *= upSample;
		F0Data = InferTools::InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1]));
		F0Shape[1] = HiddenUnitShape[1];
		XLength[0] = HiddenUnitShape[1];
		RandnCount = 192 * F0Shape[1];
		RandnShape[2] = F0Shape[1];
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
		InterpedF0 = _TensorExtractor->GetInterpedF0(F0Data);
		Nsff0 = _TensorExtractor->GetNSFF0(InterpedF0);
		_Tensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
		SoVitsInput = RVCInput;
		RandnInput = std::vector<float>(RandnCount, 0.f);
		for (auto& it : RandnInput)
			it = normal(gen) * noise_scale;
	}
	else
	{
		HiddenUnits = std::move(HiddenUnitsSrc);
		F0Data = InferTools::InterpFunc(srcF0Data, long(srcF0Data.size()), long(F0Shape[1]));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		InterpedF0 = _TensorExtractor->GetInterpedF0(F0Data);
		alignment = _TensorExtractor->GetAligments(F0Shape[1], HubertLen);
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), InterpedF0.size(), F0Shape, 2));
		if (VitsSvcVersion != L"SoVits4.0-DDSP")
		{
			UV = _TensorExtractor->GetUV(F0Data);
			SoVitsInput = { "c", "f0", "mel2ph", "uv", "noise", "sid" };
			_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, UV.data(), UV.size(), F0Shape, 2));
		}
		else
		{
			SoVitsInput = { "c", "f0", "mel2ph", "t_window", "noise", "sid" };
			IstftInput = std::vector<float>(IstftCount, ddsp_noise_scale);
			_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, IstftInput.data(), IstftInput.size(), IstftShape, 3));
		}
		RandnInput = std::vector<float>(RandnCount, 0.f);
		for (auto& it : RandnInput)
			it = normal(gen) * noise_scale;
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));
	}


	if (EnableCharaMix)
	{
		CharaMixShape[0] = F0Shape[1];
		std::vector charaMap(SpeakerCount, 0.f);
		charaMap[SpeakerIdx] = 1.f;
		charaMix.reserve((SpeakerCount + 1) * F0Shape[1]);
		for (int64_t index = 0; index < F0Shape[1]; ++index)
			charaMix.insert(charaMix.end(), charaMap.begin(), charaMap.end());
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, charaMix.data(), charaMix.size(), CharaMixShape, 2));
	}
	else
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, Chara, 1, CharaEmbShape, 1));

	if (VitsSvcVersion == L"RVC")
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));

	std::vector<float> VolumeData;

	if (EnableVolume)
	{
		SoVitsInput.emplace_back("vol");
		VolumeData = ExtractVolume(PCMData, HopSize);
		VolumeData.resize(F0Shape[1], 0.f);
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, VolumeData.data(), F0Shape[1], F0Shape, 2));
	}

	std::vector<Ort::Value> finaOut;

	finaOut = VitsSvcModel->Run(Ort::RunOptions{ nullptr },
		SoVitsInput.data(),
		_Tensors.data(),
		_Tensors.size(),
		soVitsOutput.data(),
		soVitsOutput.size());

	const auto dstWavLen = finaOut[0].GetTensorTypeAndShapeInfo().GetShape()[2];
	std::vector<int16_t> TempVecWav(dstWavLen, 0);
	for (int64_t bbb = 0; bbb < dstWavLen; bbb++)
		TempVecWav[bbb] = static_cast<int16_t>(Clamp(finaOut[0].GetTensorData<float>()[bbb]) * 32766.0f);
	if(VitsSvcVersion == L"RVC")
		TempVecWav = InferTools::InterpResample(TempVecWav, _samplingRate, 48000, 1i16);
	return TempVecWav;
}

/*std::vector<Ort::Value> VitsSvc::MelExtractor(const float* PCMAudioBegin, const float* PCMAudioEnd) const
{
	std::vector<float> _SRAudio{PCMAudioBegin, PCMAudioEnd};
	_SRAudio = InferTools::InterpResample(_SRAudio, _samplingRate, 16000, 1.f);
	const auto _MelLength = int64_t(_SRAudio.size() * shallow_diffusion->GetSamplingRate() / 16000ull / shallow_diffusion->GetHopSize());
	const auto _SpecLength = (int64_t)ceil(double(_SRAudio.size()) / 16000. * 100.);
	auto Alignment = _TensorExtractor->GetAligments(_MelLength, _SpecLength);
	Alignment.resize(_MelLength);
	std::vector<Ort::Value> AudioTensors;
	const int64_t AudioInputShape[] = { 1,1,int64_t(_SRAudio.size()) };
	const int64_t AligShape[] = { 1,_MelLength };
	AudioTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, _SRAudio.data(), _SRAudio.size(), AudioInputShape, 3));
	AudioTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, Alignment.data(), Alignment.size(), AligShape, 2));
	try
	{
		return stft_operator->Run(Ort::RunOptions{ nullptr },
			StftInput.data(),
			AudioTensors.data(),
			AudioTensors.size(),
			StftOutput.data(),
			StftOutput.size()
		);
	}
	catch (Ort::Exception& e)
	{
		LibDLVoiceCodecThrow((std::string("Locate: ShallowDiffusionStftOperator\n") + e.what()).c_str());
	}
}*/

//已弃用（旧MoeSS的推理函数）
#ifdef MOESSDFN
std::vector<int16_t> VitsSvc::InferBatch() const
{
	std::wstring RawPath;
	int ret = InsertMessageToEmptyEditBox(RawPath);
	if (ret == -1)
		LibDLVoiceCodecThrow("TTS Does Not Support Automatic Completion");
	if (ret == -2)
		LibDLVoiceCodecThrow("Please Select Files");
	RawPath += L'\n';
	std::vector<std::wstring> _Lens = CutLens(RawPath);
	const auto params = _get_init_params();
	auto tran = static_cast<int>(params.keys);
	auto threshold = static_cast<double>(params.threshold);
	auto minLen = static_cast<unsigned long>(params.minLen);
	auto frame_len = static_cast<unsigned long>(params.frame_len);
	auto frame_shift = static_cast<unsigned long>(params.frame_shift);
	if (frame_shift < 512 || frame_shift > 10240)
		frame_shift = 512;
	if (frame_len < 1024 || frame_len > 20480)
		frame_len = 1024;
	if (frame_shift > frame_len)
	{
		frame_shift = 512;
		frame_len = 4 * 1024;
	}
	if (minLen < 3 || minLen > 30)
		minLen = 3;
	if (threshold < 10.0 || threshold > 2000.0)
		threshold = 30.0;
	int64_t charEmb = params.chara;

	std::mt19937 gen(int(params.seed));
	std::normal_distribution<float> normal(0, 1);
	float noise_scale = params.noise_scale;
	float ddsp_noise_scale = params.noise_scale_w;
	if (noise_scale > 50.0f)
		noise_scale = 1.0f;
	if (ddsp_noise_scale > 50.0f)
		ddsp_noise_scale = 1.0f;
	for (auto& path : _Lens)
	{
		logger.log(L"[Inferring] Inferring \"" + path + L'\"');
		size_t proc = 0;
		if (path[0] == L'\"')
			path = path.substr(1);
		if (path[path.length() - 1] == L'\"')
			path.pop_back();
		std::vector<int16_t> _data;
		//Audio
		logger.log(L"[Inferring] PreProcessing \"" + path + L"\" Encoder");
		auto RawWav = AudioPreprocess().codec(path, _samplingRate);
		auto HubertWav = AudioPreprocess().codec(path, 16000);
		auto info = cutWav(RawWav, threshold, minLen, static_cast<unsigned short>(frame_len), static_cast<unsigned short>(frame_shift));
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
			if ((info.cutOffset[i] - info.cutOffset[i - 1]) / RawWav.getHeader().bytesPerSec > 90)
				LibDLVoiceCodecThrow("Reached max slice length, please change slicer param");

		const auto LenFactor = ((double)16000 / (double)_samplingRate);
		_callback(proc, info.cutTag.size());
		logger.log(L"[Inferring] Inferring \"" + path + L"\" Svc");
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
		{
			if (info.cutTag[i - 1])
			{
				auto featureInput = F0PreProcess(_samplingRate, (short)hop);
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				const auto srcPos = info.cutOffset[i - 1] / 2;
				std::vector<double> source(len, 0.0);

				for (unsigned long long j = 0; j < len; ++j)
					source[j] = (double)RawWav[srcPos + j] / 32768.0;

				//hubertIn
				const auto hubertInLen = (size_t)((double)len * LenFactor);
				std::vector<float> hubertin(hubertInLen, 0.0);
				const auto startPos = (size_t)(((double)info.cutOffset[i - 1] / 2.0) * LenFactor);
				for (size_t j = 0; j < hubertInLen; ++j)
					hubertin[j] = (float)HubertWav[startPos + j] / 32768.0f;

				//hubert
				const int64_t inputShape[3] = { 1i64,1i64,(int64_t)hubertInLen };
				std::vector<Ort::Value> inputTensors;
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, hubertin.data(), hubertInLen, inputShape, 3));
				std::vector<Ort::Value> hubertOut;

				try {
					hubertOut = hubert->Run(Ort::RunOptions{ nullptr },
						hubertInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						hubertOutput.data(),
						hubertOutput.size());
				}
				catch (Ort::Exception& e)
				{
					LibDLVoiceCodecThrow((std::string("Locate: hubert\n") + e.what()).c_str());
				}
				auto hubertOutData = hubertOut[0].GetTensorMutableData<float>();
				auto hubertOutLen = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
				auto hubertOutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();

				std::vector<long long> inputPitch1, alignment;
				std::vector<float> inputPitch2;
				F0PreProcess::V4Return v4_return;
				std::vector<float> hubOutData(hubertOutData, hubertOutData + hubertOutLen);
				if (SV3)
				{
					if (_samplingRate / 16000 == 2)
					{
						for (int64_t itS = hubertOutShape[1] - 1; itS + 1; --itS)
						{
							hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
						}
						hubertOutShape[1] *= 2;
						hubertOutLen *= 2;
						inputPitch2 = featureInput.GetF0AndOtherInput3(source.data(), (long long)len, (long long)hubertOutShape[1], (long long)tran);
					}
					else if (_samplingRate / 16000 == 3)
					{
						for (int64_t itS = hubertOutShape[1] - 1; itS + 1; --itS)
						{
							hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
							hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
						}
						hubertOutShape[1] *= 3;
						hubertOutLen *= 3;
						inputPitch2 = featureInput.GetF0AndOtherInput3(source.data(), (long long)len, (long long)hubertOutShape[1], (long long)tran);
					}
					else
						LibDLVoiceCodecThrow("SoVits3.0 Only Support Sr: 48K,32K");
				}
				else if (SV4)
				{
					alignment = getAligments(len / hop, hubertOutShape[1]);
					v4_return = featureInput.GetF0AndOtherInput4(source.data(), (long long)len, tran);
				}
				else if (_modelType == modelType::RVC)
				{
					for (int64_t itS = hubertOutShape[1] - 1; itS + 1; --itS)
					{
						hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
					}
					hubertOutShape[1] *= 2;
					hubertOutLen *= 2;
					auto f0data = featureInput.GetF0AndOtherInputR(source.data(), (long long)len, (long long)hubertOutShape[1], (long long)tran);
					inputPitch1 = std::move(f0data.f0);
					inputPitch2 = std::move(f0data.f0f);
				}
				else
				{
					inputPitch1 = featureInput.GetF0AndOtherInput(source.data(), (long long)len, (long long)hubertOutShape[1], (long long)tran);
				}

				inputTensors.clear();
				auto SoVitsInput = soVitsInput;
				//vits
				constexpr long long ashape[1] = { 1 };
				long long ainput[1] = { hubertOutShape[1] };
				const long long bshape[2] = { 1, featureInput.getLen() };
				long long cdata[1] = { charEmb };
				constexpr long long cshape[1] = { 1 };
				const int64 zinputShape[3] = { 1,192,featureInput.getLen() };
				const int64 zinputCount = featureInput.getLen() * 192;
				std::vector<float> zinput(zinputCount, 0.0);

				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, hubOutData.data(), hubertOutLen, hubertOutShape.data(), 3));
				std::vector<float> T_Window(2048 * featureInput.getLen(), ddsp_noise_scale);

				int64_t T_WindowShape[] = { 1, 2048, featureInput.getLen() };
				if (!SV4)
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, ainput, 1, ashape, 1));
				if (SV3)
					inputTensors.emplace_back(Ort::Value::CreateTensor<float>(*memory_info, inputPitch2.data(), featureInput.getLen(), bshape, 2));
				else if (SV4)
				{
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, v4_return.f0.data(), featureInput.getLen(), bshape, 2));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), featureInput.getLen(), bshape, 2));

					if (!SVV2)
					{
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, v4_return.uv.data(), featureInput.getLen(), bshape, 2));
						SoVitsInput = { "c", "f0", "mel2ph", "uv", "noise", "sid" };
					}
					else
					{
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, T_Window.data(), T_Window.size(), T_WindowShape, 3));
						SoVitsInput = { "c", "f0", "mel2ph", "t_window", "noise", "sid" };
					}
					for (auto& it : zinput)
						it = normal(gen) * noise_scale;
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, zinput.data(), zinputCount, zinputShape, 3));
				}
				else
				{
					inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, inputPitch1.data(), featureInput.getLen(), bshape, 2));
					if (_modelType == modelType::RVC)
					{
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, inputPitch2.data(), featureInput.getLen(), bshape, 2));
						SoVitsInput = RVCInput;
						for (auto& it : zinput)
							it = normal(gen) * noise_scale;
					}
				}
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, cdata, 1, cshape, 1));
				std::vector<Ort::Value> finaOut;
				if (_modelType == modelType::RVC)
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, zinput.data(), zinputCount, zinputShape, 3));
				try
				{
					finaOut = VitsSvcModel->Run(Ort::RunOptions{ nullptr },
						SoVitsInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						soVitsOutput.data(),
						soVitsOutput.size());
				}
				catch (Ort::Exception& e)
				{
					LibDLVoiceCodecThrow((std::string("Locate: sovits\n") + e.what()).c_str());
				}

				const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
				const auto dstWavLen = int64_t(len);
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
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				const auto data = new int16_t[len];
				memset(data, 0, len * 2);
				_data.insert(_data.end(), data, data + len);
				delete[] data;
			}
			_callback(++proc, info.cutTag.size());
		}
		logger.log(L"[Inferring] \"" + path + L"\" Finished");
		if (_Lens.size() == 1)
		{
			logger.log(L"[Info] Finished, Send To FrontEnd");
			return _data;
		}
		std::wstring outPutPath = GetCurrentFolder() + L"\\OutPuts\\" + path.substr(path.rfind(L'\\') + 1, path.rfind(L'.')) + L'-' + std::to_wstring(uint64_t(path.data())) + L".wav";
		logger.log(L"[Inferring] Write To \"" + outPutPath + L'\"');
		Wav(_samplingRate, long(_data.size()) * 2, _data.data()).Writef(outPutPath);
	}
	logger.log(L"[Info] Finished");
	return {};
}
#endif

/*
std::vector<Ort::Value> VitsSvc::InferSliceTensor(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice, size_t SliceIdx,
	const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, std::vector<Ort::Value>& _Tensors,
	std::vector<const char*>& SoVitsInput) const
{
	int64_t charEmb = _InferParams.SpeakerId;
	float noise_scale = _InferParams.NoiseScale;
	float ddsp_noise_scale = _InferParams.DDSPNoiseScale;

	auto RawWav = TensorPreprocess::MoeVoiceStudioTensorExtractor::InterpResample(_Slice.Audio[SliceIdx], 48000, 16000, 32768.0f);
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
		LibDLVoiceCodecThrow((std::string("Locate: hubert\n") + e.what()).c_str());
	}
	auto HubertSize = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetElementCount();
	auto HubertOutPutData = HubertOutPuts[0].GetTensorMutableData<float>();
	auto HubertOutPutShape = HubertOutPuts[0].GetTensorTypeAndShapeInfo().GetShape();
	HubertInputTensors.clear();
	if (HubertOutPutShape[2] != HiddenUnitKDims)
		LibDLVoiceCodecThrow("HiddenUnitKDims UnMatch");

	std::vector srcHiddenUnits(HubertOutPutData, HubertOutPutData + HubertSize);

	if (EnableCluster && _InferParams.ClusterRate > 0.001f)
	{
		for (int64_t indexs = 0; indexs < HubertOutPutShape[1]; ++indexs)
		{
			const auto curbeg = srcHiddenUnits.data() + indexs * HubertOutPutShape[2];
			const auto curend = srcHiddenUnits.data() + (indexs + 1) * HubertOutPutShape[2];
			const auto hu = Cluster->find({ curbeg ,curend }, long(charEmb));
			for (int64_t ind = 0; ind < HubertOutPutShape[2]; ++ind)
				*(curbeg + ind) = *(curbeg + ind) * (1.f - _InferParams.ClusterRate) + hu[ind] * _InferParams.ClusterRate;
		}
	}

	const auto HubertLen = int64_t(HubertSize) / HiddenUnitKDims;
	int64_t F0Shape[] = { 1, int64_t(_Slice.Audio[SliceIdx].size() * _samplingRate / 48000 / HopSize) };
	int64_t HiddenUnitShape[] = { 1, HubertLen, HiddenUnitKDims };
	constexpr int64_t LengthShape[] = { 1 };
	int64_t CharaEmbShape[] = { 1 };
	int64_t CharaMixShape[] = { F0Shape[1], SpeakerCount };
	int64_t RandnShape[] = { 1, 192, F0Shape[1] };
	const int64_t IstftShape[] = { 1, 2048, F0Shape[1] };
	int64_t RandnCount = F0Shape[1] * 192;
	const int64_t IstftCount = F0Shape[1] * 2048;

	std::vector<float> RandnInput, IstftInput, UV, InterpedF0;
	std::vector<int64_t> alignment;
	int64_t XLength[1] = { HubertLen };
	std::vector<int64_t> Nsff0;
	//auto SoVitsInput = soVitsInput;
	int64_t Chara[] = { charEmb };
	std::vector<float> charaMix;

	auto srcF0Data = _Slice.F0[SliceIdx];
	for (auto& ifo : srcF0Data)
		ifo *= (float)pow(2.0, static_cast<double>(_InferParams.Keys) / 12.0);
	std::vector<float> HiddenUnits;
	std::vector<float> F0Data;

	//Compatible with all versions
	if (SoVits3)
	{
		int64_t upSample = _samplingRate / 16000;
		HiddenUnits.reserve(HubertSize * (upSample + 1));
		for (int64_t itS = 0; itS < HiddenUnitShape[1]; ++itS)
			for (int64_t itSS = 0; itSS < upSample; ++itSS)
				HiddenUnits.insert(HiddenUnits.end(), srcHiddenUnits.begin() + itS * HiddenUnitKDims, srcHiddenUnits.begin() + (itS + 1) * HiddenUnitKDims);
		HiddenUnitShape[1] *= upSample;
		HubertSize *= upSample;
		F0Data = GetInterpedF0(InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1])));
		F0Shape[1] = HiddenUnitShape[1];
		XLength[0] = HiddenUnitShape[1];
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, F0Data.data(), F0Data.size(), F0Shape, 2));
	}
	else if (SoVits2)
	{
		HiddenUnits = std::move(srcHiddenUnits);
		F0Shape[1] = HiddenUnitShape[1];
		F0Data = InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1]));
		XLength[0] = HiddenUnitShape[1];
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
		Nsff0 = GetNSFF0(F0Data);
		_Tensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));
	}
	else if (_modelType == modelType::RVC)
	{
		constexpr int64_t upSample = 2;
		HiddenUnits.reserve(HubertSize * (upSample + 1));
		for (int64_t itS = 0; itS < HiddenUnitShape[1]; ++itS)
			for (int64_t itSS = 0; itSS < upSample; ++itSS)
				HiddenUnits.insert(HiddenUnits.end(), srcHiddenUnits.begin() + itS * HiddenUnitKDims, srcHiddenUnits.begin() + (itS + 1) * HiddenUnitKDims);
		HiddenUnitShape[1] *= upSample;
		HubertSize *= upSample;
		F0Data = InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1]));
		F0Shape[1] = HiddenUnitShape[1];
		XLength[0] = HiddenUnitShape[1];
		RandnCount = 192 * F0Shape[1];
		RandnShape[2] = F0Shape[1];
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
		InterpedF0 = GetInterpedF0(F0Data);
		Nsff0 = GetNSFF0(InterpedF0);
		_Tensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
		SoVitsInput = RVCInput;
		RandnInput = std::vector<float>(RandnCount, 0.f);
		for (auto& it : RandnInput)
			it = normal(gen) * noise_scale;
	}
	else
	{
		HiddenUnits = std::move(srcHiddenUnits);
		F0Data = InterpFunc(srcF0Data, long(srcF0Data.size()), long(F0Shape[1]));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
		InterpedF0 = GetInterpedF0(F0Data);
		alignment = GetAligments(F0Shape[1], HubertLen);
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), InterpedF0.size(), F0Shape, 2));
		if (!SoVitsDDSP)
		{
			UV = GetUV(F0Data);
			SoVitsInput = { "c", "f0", "mel2ph", "uv", "noise", "sid" };
			_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, UV.data(), UV.size(), F0Shape, 2));
		}
		else
		{
			SoVitsInput = { "c", "f0", "mel2ph", "t_window", "noise", "sid" };
			IstftInput = std::vector<float>(IstftCount, ddsp_noise_scale);
			_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, IstftInput.data(), IstftInput.size(), IstftShape, 3));
		}
		RandnInput = std::vector<float>(RandnCount, 0.f);
		for (auto& it : RandnInput)
			it = normal(gen) * noise_scale;
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));
	}


	if (EnableCharaMix)
	{
		if (SpeakerCount > 1)
			charaMix = GetCurrectSpkMixData(_Slice.Speaker[SliceIdx], F0Shape[1]);
		CharaMixShape[0] = F0Shape[1];
		if (charaMix.empty())
		{
			std::vector charaMap(SpeakerCount, 0.f);
			charaMap[charEmb] = 1.f;
			charaMix.reserve((SpeakerCount + 1) * F0Shape[1]);
			for (int64_t index = 0; index < F0Shape[1]; ++index)
				charaMix.insert(charaMix.end(), charaMap.begin(), charaMap.end());
		}
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, charaMix.data(), charaMix.size(), CharaMixShape, 2));
	}
	else
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, Chara, 1, CharaEmbShape, 1));

	if (_modelType == modelType::RVC)
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));

	std::vector<float> VolumeData;

	if (EnableVolume)
	{
		SoVitsInput.emplace_back("vol");
		VolumeData = InterpFunc(_Slice.Volume[SliceIdx], long(_Slice.Volume[SliceIdx].size()), long(F0Shape[1]));
		_Tensors.emplace_back(Ort::Value::CreateTensor(*memory_info, VolumeData.data(), F0Shape[1], F0Shape, 2));
	}

	return VitsSvcModel->Run(Ort::RunOptions{ nullptr },
	   SoVitsInput.data(),
	   _Tensors.data(),
	   _Tensors.size(),
	   soVitsOutput.data(),
	   soVitsOutput.size());
}
 */

#ifdef WIN32
#ifdef MoeVSMui

void VitsSvc::StartRT(Mui::Window::UIWindowBasic* window, const MoeVSProjectSpace::MoeVSSvcParams& _Params)
{
	if (RTSTAT)
		return;

	std::wstring error;

	recoder = new MRecorder();
	recoder->InitRecorder(error, _samplingRate, 1, 16, Mui::_m_uint(_samplingRate * 0.5));
	recoder->Start();

	audio_player = new Mui::Render::MDS_AudioPlayer(window);
	audio_player->InitAudioPlayer(error);

	audio_stream = audio_player->CreateStreamPCM(recoder->GetFrameSize(), recoder->GetSampleRate(), recoder->GetChannels(), recoder->GetBitRate(), recoder->GetBlockAlign());
	audio_player->PlayTrack(audio_stream);

	emptyLen = InferPCMData(std::vector<int16_t>(recoder->GetFrameSize() / 4 * 3), _samplingRate, _Params).size();

	std::thread RT_RECORD_THREAD = std::thread([&]()
		{
			logger.log(L"[RTInference] Recording Thread Start!");
			while (RTSTAT)
			{
				const auto PCM = recoder->GetStreamData();
				if (!PCM) continue;
				rawInputBuffer.emplace_back((int16_t*)PCM->data, (int16_t*)(PCM->data + PCM->size));
				delete PCM;
			}
			logger.log(L"[RTInference] Recording Thread End!");
		});

	std::thread RT_INPUT_CROSSFADE_THREAD = std::thread([&]()
		{
			logger.log(L"[RTInference] Input Crossfade Thread Start!");
			//const auto config = _get_init_params();
			while (RTSTAT)
			{
				if (rawInputBuffer.size() > 1)
				{
					std::vector<int16_t> pBuffer;
					pBuffer.reserve(2 * rawInputBuffer[0].size());
					pBuffer.insert(pBuffer.end(), rawInputBuffer[0].begin(), rawInputBuffer[0].end());
					pBuffer.insert(pBuffer.end(), rawInputBuffer[1].begin(),
						rawInputBuffer[1].begin() + int64_t(rawInputBuffer[1].size()) / 2);
					inputBuffer.emplace_back(std::move(pBuffer));
					rawInputBuffer.pop_front();
				}
				if (rawInputBuffer.size() > 100)
				{
					logger.log(L"[RTInferenceWarn] Raw Input Buffer Is Too Large");
				}
			}
			logger.log(L"[RTInference] Input Crossfade Thread End!");
		});

	std::thread RT_INFERENCE_THREAD = std::thread([&]()
		{
			logger.log(L"[RTInference] Inferencing Thread Start!");
			const auto PCMSIZE = recoder->GetFrameSize() / 2;
			while (RTSTAT)
			{
				//const auto configs = _get_init_params();
				if (!inputBuffer.empty())
				{
					try
					{
						bool zeroVector = true;
						for(const auto& i16data : inputBuffer[0])
						{
							if(i16data> 1600)
							{
								zeroVector = false;
								break;
							}
						}
						if(zeroVector)
							rawOutputBuffer.emplace_back(std::vector<int16_t>(emptyLen, 0));
						else
						{
							auto RtRES = InferPCMData(inputBuffer[0], _samplingRate, _Params);
							rawOutputBuffer.emplace_back(std::move(RtRES));
						}
						inputBuffer.pop_front();
					}
					catch (std::exception& e)
					{
						RTSTAT = false;
						inputBuffer.clear();
						outputBuffer.clear();
						rawInputBuffer.clear();
						rawOutputBuffer.clear();
						LibDLVoiceCodecThrow(e.what());
					}
				}
				if (inputBuffer.size() > 100)
				{
					logger.log(L"[RTInferenceWarn] Input Buffer Is Too Large");
				}
			}
			logger.log(L"[RTInference] Inferencing Thread End!");
		});

	std::thread RT_OUTPUT_CROSSFADE_THREAD = std::thread([&]()
		{
			logger.log(L"[RTInference] OutPut Crossfade Thread Start!");
			while (RTSTAT)
			{
				if (rawOutputBuffer.size() > 1)
				{
					const auto cross_fade_size = int64_t(rawOutputBuffer[0].size()) / 3;
					const auto PCMSIZE = cross_fade_size * 2;
					for (int64_t i = 0; i < cross_fade_size; ++i)
					{
						rawOutputBuffer[0][PCMSIZE + i] = int16_t(((double)rawOutputBuffer[0][PCMSIZE + i] * (1.0 - (double(i) / double(cross_fade_size)))) + ((double)rawOutputBuffer[1][i] * (double(i) / double(cross_fade_size))));
						rawOutputBuffer[1][PCMSIZE + i] = int16_t(((double)rawOutputBuffer[0][PCMSIZE + i] * (1.0 - (double(i) / double(cross_fade_size)))) + ((double)rawOutputBuffer[1][i] * (double(i) / double(cross_fade_size))));
					}
					std::vector pBuffer(rawOutputBuffer[0].data() + cross_fade_size / 2, rawOutputBuffer[0].data() + 2 * cross_fade_size + cross_fade_size / 2);
					outputBuffer.emplace_back(std::move(pBuffer));
					rawOutputBuffer.pop_front();
				}
			}
			logger.log(L"[RTInference] OutPut Crossfade Thread End!");
		});

	std::thread RT_OUTPUT_THREAD = std::thread([&]()
		{
			logger.log(L"[RTInference] OutPut Thread Start!");
			const auto PCMSIZE = recoder->GetFrameSize();
			std::vector<Mui::_m_byte> pBuffer(PCMSIZE);
			Mui::_m_size lastSize = 0;
			while (RTSTAT)
			{
				if (!outputBuffer.empty())
				{
					//audio_player->WriteStreamPCM(audio_stream, (Mui::_m_byte*)outputBuffer.front().data(), Mui::_m_size(RTAudioSize));
					//outputBuffer.pop_front();
					auto queue_data = (Mui::_m_byte*)outputBuffer[0].data();
					auto queue_dataSize = Mui::_m_size(outputBuffer[0].size() * 2);
					while (queue_dataSize + lastSize >= PCMSIZE)
					{
						//每次从队列中拿走帧大小的数据
						const auto dataSize = PCMSIZE - lastSize;
						memcpy(pBuffer.data() + lastSize, queue_data, dataSize);
						//调用WriteStream 播放
						audio_player->WriteStreamPCM(audio_stream, pBuffer.data(), PCMSIZE);
						//指针偏移
						queue_data += dataSize;
						queue_dataSize -= dataSize;
						lastSize = 0;
					}
					//如果剩余数据不足PCMLength
					if (queue_dataSize > 0)
					{
						//检查本次queue能否把缓冲区填满
						if (lastSize + queue_dataSize >= PCMSIZE)
						{
							//计算剩余空间
							const auto dataSize = PCMSIZE - lastSize;
							//复制数据到buffer
							memcpy(pBuffer.data() + lastSize, queue_data, dataSize);
							audio_player->WriteStreamPCM(audio_stream, pBuffer.data(), PCMSIZE);
							queue_data += dataSize;
							queue_dataSize -= dataSize;
							lastSize = 0;
						}
						//如果剩余空间仍然不足PCMLength，将剩余数据复制到buffer中 等待下次循环
						if (queue_dataSize > 0)
						{
							memcpy(pBuffer.data() + lastSize, queue_data, queue_dataSize);
							lastSize += queue_dataSize;
						}
					}
					logger.log(L"OUTPUTBUFFERLEN:" + std::to_wstring(outputBuffer.size()));
					logger.log(L"INPUTBUFFERLEN:" + std::to_wstring(inputBuffer.size()));
					outputBuffer.pop_front();
				}
			}
			logger.log(L"[RTInference] OutPut Thread End!");
		});

	logger.log(L"[RTInference] Start RTInference!");
	for (uint64_t idx = 0; idx < 1; ++idx)
		rawInputBuffer.emplace_back(recoder->GetFrameSize() / 2, 0);
	RTSTAT = true;
	RT_RECORD_THREAD.detach();
	RT_INPUT_CROSSFADE_THREAD.detach();
	RT_INFERENCE_THREAD.detach();
	RT_OUTPUT_CROSSFADE_THREAD.detach();
	RT_OUTPUT_THREAD.detach();
	while (true);
}

void VitsSvc::EndRT()
{
	recoder->Stop();
	audio_player->StopTrack(audio_stream);
	audio_player->DeleteTrack(audio_stream);
	delete audio_stream;
	delete audio_player;
	delete recoder;
	audio_stream = nullptr;
	audio_player = nullptr;
	recoder = nullptr;
	if (RTSTAT)
		RTSTAT = false;
	inputBuffer.clear();
	outputBuffer.clear();
	rawInputBuffer.clear();
	rawOutputBuffer.clear();
}
#endif
#endif

MoeVoiceStudioCoreEnd