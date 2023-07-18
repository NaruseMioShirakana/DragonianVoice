#include "../header/DiffSvc.hpp"
#include <random>
#include "../../AvCodec/AvCodeResample.h"
#include <regex>
#include "../../InferTools/Sampler/MoeVSSamplerManager.hpp"
#include "../../InferTools/F0Extractor/F0ExtractorManager.hpp"

MoeVoiceStudioCoreHeader
DiffusionSvc::~DiffusionSvc()
{
	logger.log(L"[Info] unloading DiffSvc Models");
	delete hubert;
	delete nsfHifigan;
	delete encoder;
	delete denoise;
	delete pred;
	delete after;
	delete diffSvc;
	delete alpha;
	delete naive;
	hubert = nullptr;
	nsfHifigan = nullptr;
	encoder = nullptr;
	denoise = nullptr;
	diffSvc = nullptr;
	pred = nullptr;
	after = nullptr;
	alpha = nullptr;
	naive = nullptr;
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
	const std::wstring _path = GetCurrentFolder() + L"\\Models\\" + _folder + L"\\" + _folder;

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
		delete hubert;
		delete nsfHifigan;
		delete encoder;
		delete denoise;
		delete pred;
		delete after;
		delete diffSvc;
		delete alpha;
		delete naive;
		hubert = nullptr;
		nsfHifigan = nullptr;
		encoder = nullptr;
		denoise = nullptr;
		diffSvc = nullptr;
		pred = nullptr;
		after = nullptr;
		alpha = nullptr;
		naive = nullptr;
		throw std::exception(_exception.what());
	}

	if (_Config["TensorExtractor"].IsString())
		DiffSvcVersion = to_wide_string(_Config["TensorExtractor"].GetString());

	if (_Config["MaxStep"].IsInt())
		MaxStep = _Config["MaxStep"].GetInt();

	MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::Others _others_param;
	_others_param.Memory = *memory_info;
	_TensorExtractor = GetTensorExtractor(DiffSvcVersion, 48000, _samplingRate, HopSize, EnableCharaMix, EnableVolume, HiddenUnitKDims, SpeakerCount, _others_param);

	_callback = _ProgressCallback;
}

std::vector<int16_t> DiffusionSvc::SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L'\"');
	std::vector<int16_t> _data;
	size_t total_audio_size = 0;
	for (const auto& data_size : _Slice.OrgLen)
		total_audio_size += data_size;
	_data.reserve(size_t(double(total_audio_size) * 1.5));

	std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);

	auto speedup = (int64_t)_InferParams.Pndm;
	auto step = (int64_t)_InferParams.Step;

	if (step > MaxStep)
		step = MaxStep;
	if (speedup >= step)
		speedup /= 2;
	if (speedup == 0)
		speedup = 1;
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
			auto RawWav = InferTools::InterpResample(_Slice.Audio[slice], 48000, 16000, 32768.0f);
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
			if (!EnableCharaMix && EnableCluster && _InferParams.ClusterRate > 0.001f)
			{
				for (int64_t indexs = 0; indexs < HubertOutPutShape[1]; ++indexs)
				{
					const auto curbeg = srcHiddenUnits.data() + indexs * HubertOutPutShape[2];
					const auto curend = srcHiddenUnits.data() + (indexs + 1) * HubertOutPutShape[2];
					const auto hu = Cluster->find({ curbeg ,curend }, long(_InferParams.SpeakerId));
					for (int64_t ind = 0; ind < HubertOutPutShape[2]; ++ind)
						*(curbeg + ind) = *(curbeg + ind) * (1.f - _InferParams.ClusterRate) + hu[ind] * _InferParams.ClusterRate;
				}
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
				auto InterpedF0 = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetInterpedF0log(InferTools::InterpFunc(_Slice.F0[slice], long(_Slice.F0[slice].size()), long(F0Shape[1])), true);
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
				auto InputTensors = _TensorExtractor->Extract(srcHiddenUnits, _Slice.F0[slice], _Slice.Volume[slice], _Slice.Speaker[slice], _Inference_Params);

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
		InferTools::Wav(_samplingRate, long(_data.size() * 2), _data.data()).Writef(OutFolder);
	}
	return AudioFolders;
}

std::vector<int16_t> DiffusionSvc::SliceInference(const MoeVSProjectSpace::MoeVSAudioSliceRef& _Slice, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const
{
	logger.log(L"[Inferring] Inferring \"" + _Slice.Path + L'\"');
	std::vector<int16_t> _data;
	_data.reserve(size_t(double(_Slice.OrgLen) * 1.5));

	std::mt19937 gen(int(_InferParams.Seed));
	std::normal_distribution<float> normal(0, 1);

	auto speedup = (int64_t)_InferParams.Pndm;
	auto step = (int64_t)_InferParams.Step;
	const auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;
	if (diffSvc)
		_callback(0, 1);
	else
		_callback(0, RealDiffSteps);
	size_t process = 0;
	if (_Slice.IsNotMute)
	{
		auto RawWav = InferTools::InterpResample(_Slice.Audio, 48000, 16000, 32768.0f);
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
		if (!EnableCharaMix && EnableCluster && _InferParams.ClusterRate > 0.001f)
		{
			for (int64_t indexs = 0; indexs < HubertOutPutShape[1]; ++indexs)
			{
				const auto curbeg = srcHiddenUnits.data() + indexs * HubertOutPutShape[2];
				const auto curend = srcHiddenUnits.data() + (indexs + 1) * HubertOutPutShape[2];
				const auto hu = Cluster->find({ curbeg ,curend }, long(_InferParams.SpeakerId));
				for (int64_t ind = 0; ind < HubertOutPutShape[2]; ++ind)
					*(curbeg + ind) = *(curbeg + ind) * (1.f - _InferParams.ClusterRate) + hu[ind] * _InferParams.ClusterRate;
			}
		}
		std::vector<Ort::Value> finaOut;
		std::vector<Ort::Value> DiffOut;
		if (diffSvc)
		{
			const auto HubertLen = int64_t(HubertSize) / HiddenUnitKDims;
			const int64_t F0Shape[] = { 1, int64_t(_Slice.Audio.size() * _samplingRate / 48000 / HopSize) };
			const int64_t HiddenUnitShape[] = { 1, HubertLen, HiddenUnitKDims };
			constexpr int64_t CharaEmbShape[] = { 1 };
			int64_t speedData[] = { Pndms };
			auto InterpedF0 = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetInterpedF0log(InferTools::InterpFunc(_Slice.F0, long(_Slice.F0.size()), long(F0Shape[1])), true);
			auto alignment = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetAligments(F0Shape[1], HubertLen);
			std::vector<Ort::Value> TensorsInp;
			std::vector<float> initial_noise(melBins * (F0Shape[1] + 2), 0.0);
			if (_Slice.Mel.empty())
				for (auto& it : initial_noise)
					it = normal(gen) * _InferParams.NoiseScale;
			else
			{
				const auto mel_len = int64_t(_Slice.Mel.size() / melBins);
				if (mel_len != F0Shape[1])
				{
					auto alig = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetAligments(F0Shape[1], mel_len);
					alig.resize(F0Shape[1]);
					for (size_t i = 0; i < alig.size(); ++i)
						memcpy(initial_noise.data() + i * melBins, _Slice.Mel.data() + alig[i] * melBins, melBins * sizeof(float));
				}
				else
					initial_noise = _Slice.Mel;
			}
			initial_noise.resize(melBins * F0Shape[1]);

			long long noise_shape[4] = { 1,1,melBins,F0Shape[1] };
			int64_t Chara[] = { _InferParams.SpeakerId };

			TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, srcHiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
			TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
			TensorsInp.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Chara, 1, CharaEmbShape, 1));
			TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), F0Shape[1], F0Shape, 2));
			TensorsInp.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
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
			_Inference_Params.AudioSize = _Slice.Audio.size();
			_Inference_Params.Chara = _InferParams.SpeakerId;
			_Inference_Params.NoiseScale = _InferParams.NoiseScale;
			_Inference_Params.DDSPNoiseScale = _InferParams.DDSPNoiseScale;
			_Inference_Params.Seed = int(_InferParams.Seed);
			auto InputTensors = _TensorExtractor->Extract(srcHiddenUnits, _Slice.F0, _Slice.Volume, _Slice.Speaker, _Inference_Params);

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

			std::vector<float> initial_noise(melBins* (InputTensors.Data.FrameShape[1] + 2), 0.0);
			if (_Slice.Mel.empty())
				for (auto& it : initial_noise)
					it = normal(gen) * _InferParams.NoiseScale;
			else
			{
				const auto mel_len = int64_t(_Slice.Mel.size() / melBins);
				if (mel_len != InputTensors.Data.FrameShape[1])
				{
					auto alig = MoeVSTensorPreprocess::MoeVoiceStudioTensorExtractor::GetAligments(InputTensors.Data.FrameShape[1], mel_len);
					alig.resize(InputTensors.Data.FrameShape[1]);
					for (size_t i = 0; i < alig.size(); ++i)
						memcpy(initial_noise.data() + i * melBins, _Slice.Mel.data() + alig[i] * melBins, melBins * sizeof(float));
				}
				else
					initial_noise = _Slice.Mel;
			}
			initial_noise.resize(melBins * InputTensors.Data.FrameShape[1]);

			long long noise_shape[4] = { 1,1,melBins,InputTensors.Data.FrameShape[1] };
			DenoiseInTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));

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
		const auto dstWavLen = (_Slice.OrgLen * int64_t(_samplingRate)) / 48000;
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
		//Mute clips
		const auto len = _Slice.OrgLen * int64_t(_samplingRate) / 48000;
		const auto data = new int16_t[len];
		memset(data, 0, int64_t(len) * 2);
		_data.insert(_data.end(), data, data + len);
		delete[] data;
		if (!diffSvc)
		{
			process += RealDiffSteps;
			_callback(process, RealDiffSteps);
		}
	}
	if (diffSvc)
		_callback(++process, 1);
	logger.log(L"[Inferring] \"" + _Slice.Path + L"\" Finished");
	return _data;
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
		for (int64_t indexs = 0; indexs < HubertOutPutShape[1]; ++indexs)
		{
			const auto curbeg = HiddenUnits.data() + indexs * HubertOutPutShape[2];
			const auto curend = HiddenUnits.data() + (indexs + 1) * HubertOutPutShape[2];
			const auto hu = Cluster->find({ curbeg ,curend }, long(charEmb));
			for (int64_t ind = 0; ind < HubertOutPutShape[2]; ++ind)
				*(curbeg + ind) = *(curbeg + ind) * (1.f - _InferParams.ClusterRate) + hu[ind] * _InferParams.ClusterRate;
		}
	}

	const auto HubertLen = int64_t(HubertSize) / HiddenUnitKDims;
	const int64_t F0Shape[] = { 1, int64_t(PCMData.size() / HopSize) };
	const int64_t HiddenUnitShape[] = { 1, HubertLen, HiddenUnitKDims };
	constexpr int64_t CharaEmbShape[] = { 1 };
	const int64_t CharaMixShape[] = { F0Shape[1], SpeakerCount };

	const auto F0Extractor = MoeVSF0Extractor::GetF0Extractor(_InferParams.F0Method, _samplingRate, HopSize);
	auto F0Data = F0Extractor->ExtractF0(PCMData, PCMData.size() / HopSize);
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

	const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
	const auto dstWavLen = (int64_t)PCMData.size();
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
	TempVecWav.resize(dstWavLen);
	return TempVecWav;
}

MoeVoiceStudioCoreEnd
