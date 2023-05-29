#include "../header/VitsSvc.hpp"
#include "../../AvCodec/AvCodeResample.h"
#include <random>

INFERCLASSHEADER
VitsSvc::~VitsSvc()
{
	logger.log(L"[Info] unloading VitsSvc Models");
	delete hubert;
	delete VitsSvcModel;
	hubert = nullptr;
	VitsSvcModel = nullptr;
	delete kmeans_;
	kmeans_ = nullptr;
	logger.log(L"[Info] VitsSvc Models unloaded");
}

VitsSvc::VitsSvc(const rapidjson::Document& _config, const callback& _cb, const callback_params& _mr, Device _dev)
{
	if(_config["Type"].GetString() == std::string("RVC"))
		_modelType = modelType::RVC;
	else
		_modelType = modelType::SoVits;

	ChangeDevice(_dev);

	//Check Folder
	if (_config["Folder"].IsNull())
		throw std::exception("[Error] Missing field \"folder\" (Model Folder)");
	if (!_config["Folder"].IsString())
		throw std::exception("[Error] Field \"folder\" (Model Folder) Must Be String");
	const auto _folder = to_wide_string(_config["Folder"].GetString());
	const auto K_means_folder = GetCurrentFolder() + L"\\Models\\" + _folder + L"\\" + L"kmeans.npy";
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

	//LoadModels
	try
	{
		logger.log(L"[Info] loading VitsSvcModel Models");
		hubert = new Ort::Session(*env, (GetCurrentFolder() + L"\\hubert\\" + HuPath + L".onnx").c_str(), *session_options);
		if (_modelType == modelType::RVC)
			VitsSvcModel = new Ort::Session(*env, (_path + L"_RVC.onnx").c_str(), *session_options);
		else
			VitsSvcModel = new Ort::Session(*env, (_path + L"_SoVits.onnx").c_str(), *session_options);
		logger.log(L"[Info] VitsSvcModel Models loaded");
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

	if (!_config["Cleaner"].IsNull())
	{
		const auto Cleaner = to_wide_string(_config["Cleaner"].GetString());
		if (!Cleaner.empty())
			switch (_plugin.Load(Cleaner))
			{
			case (-1):
				throw std::exception("[Error] Plugin File Does Not Exist");
			case (1):
				throw std::exception("[Error] Plugin Has Some Error");
			default:
				logger.log(L"[Info] Plugin Loaded");
				break;
			}
		else
			logger.log(L"[Info] Disable Plugin");
	}
	else
		logger.log(L"[Info] Disable Plugin");

	if (!_config["SoVits3"].IsNull())
		SV3 = _config["SoVits3"].GetBool();
	if (!_config["SoVits4"].IsNull())
		SV4 = _config["SoVits4"].GetBool();

	if (SV3 && SV4)
		throw std::exception("[Error] SoVits3 && SoVits4 Must Be False");

	if(!(_config["Hop"].IsInt() || _config["Hop"].IsInt64()))
		throw std::exception("[Error] Hop Must Exist And Must Be Int");
	hop = _config["Hop"].GetInt();

	if (!(_config["HiddenSize"].IsInt() || _config["HiddenSize"].IsInt64()))
		logger.log(L"[Warn] Missing Field \"HiddenSize\", Use Default Value (256)");
	else
		Hidden_Size = _config["HiddenSize"].GetInt();

	if (!_config["CharaMix"].IsBool())
		logger.log(L"[Warn] Missing Field \"CharaMix\", Use Default Value (False)");
	else
		CharaMix = _config["CharaMix"].GetBool();

	if(_waccess(K_means_folder.c_str(), 0) != -1)
	{
		KMenas_Stat = true;
		if (!(_config["KMeansLength"].IsInt() || _config["KMeansLength"].IsInt64()))
			logger.log(L"[Warn] Missing Field \"KMeansLength\", Use Default Value (10000)");
		else
			KMeans_Size = _config["KMeansLength"].GetInt();
		kmeans_ = new Kmeans(K_means_folder, Hidden_Size, KMeans_Size);
	}

	if(hop < 1)
		throw std::exception("[Error] Hop Must > 0");

	if (SV4)
	{
		const auto nameinp = VitsSvcModel->GetInputNameAllocated(3, allocator);
		const std::string inpname = nameinp.get();
		SVV2 = inpname != "uv";
	}

	if (_config["Volume"].IsBool())
		VolumeB = _config["Volume"].GetBool();
	else
		logger.log(L"[Warn] Missing Field \"Volume\", Use Default Value (False)");

	if (_config["Characters"].IsArray())
		n_speaker = _config["Characters"].Size();

	_callback = _cb;
	_get_init_params = _mr;
}

//已弃用（旧MoeSS的推理函数）
#ifdef MOESSDFN
std::vector<int16_t> VitsSvc::InferBatch() const
{
	std::wstring RawPath;
	int ret = InsertMessageToEmptyEditBox(RawPath);
	if (ret == -1)
		throw std::exception("TTS Does Not Support Automatic Completion");
	if (ret == -2)
		throw std::exception("Please Select Files");
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
	for(auto & path : _Lens)
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
				throw std::exception("Reached max slice length, please change slicer param");

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
					throw std::exception((std::string("Locate: hubert\n") + e.what()).c_str());
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
						throw std::exception("SoVits3.0 Only Support Sr: 48K,32K");
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
					throw std::exception((std::string("Locate: sovits\n") + e.what()).c_str());
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
		logger.log(L"[Inferring] Write To \""+ outPutPath + L'\"');
		Wav(_samplingRate, long(_data.size()) * 2, _data.data()).Writef(outPutPath);
	}
	logger.log(L"[Info] Finished");
	return {};
}
#endif

//推理
std::vector<int16_t> VitsSvc::InferWithF0AndHiddenUnit(std::vector<MoeVSProject::Params>& Inputs) const
{
	// Hidden_Unit -> Shape -> [audio, slice]
	const auto params = _get_init_params();
	int64_t charEmb = params.chara;
	std::mt19937 gen(int(params.seed));
	std::normal_distribution<float> normal(0, 1);
	float noise_scale = params.noise_scale;
	float ddsp_noise_scale = params.noise_scale_w;
	if (noise_scale > 50.0f)
		noise_scale = 1.0f;
	if (ddsp_noise_scale > 50.0f)
		ddsp_noise_scale = 1.0f;

	const auto total_audio_count = Inputs.size();
	for(auto& inp_audio : Inputs)
	{
		if(!inp_audio.paths.empty())
			logger.log(L"[Inferring] Inferring \"" + inp_audio.paths + L'\"');
		size_t proc = 0;
		const auto Total_Slice_Count = inp_audio.Hidden_Unit.size();
		_callback(proc, Total_Slice_Count);
		logger.log(L"[Inferring] Inferring \"" + inp_audio.paths + L"\" Svc");

		std::vector<int16_t> _data;
		size_t total_audio_size = 0;
		for (const auto& data_size : inp_audio.OrgLen)
			total_audio_size += data_size;
		_data.reserve(size_t(double(total_audio_size) * 1.5));
		for(size_t slice = 0; slice < Total_Slice_Count; ++slice)
		{
			if (inp_audio.symbolb[slice])
			{
				auto HubertSize = inp_audio.Hidden_Unit[slice].size();
				const auto HubertLen = int64_t(HubertSize) / Hidden_Size;
				int64_t F0Shape[] = { 1, int64_t(inp_audio.F0[slice].size()) };
				int64_t HiddenUnitShape[] = { 1, HubertLen, Hidden_Size };
				constexpr int64_t LengthShape[] = { 1 };
				int64_t CharaEmbShape[] = { 1 };
				int64_t CharaMixShape[] = { F0Shape[1], n_speaker };
				int64_t RandnShape[] = { 1, 192, F0Shape[1] };
				const int64_t IstftShape[] = { 1, 2048, F0Shape[1] };
				int64_t RandnCount = F0Shape[1] * 192;
				const int64_t IstftCount = F0Shape[1] * 2048;

				std::vector<float> RandnInput, IstftInput, UV, InterpedF0;
				std::vector<int64_t> alignment;
				int64_t XLength[1] = { HubertLen };
				std::vector<int64_t> Nsff0;
				auto SoVitsInput = soVitsInput;
				int64_t Chara[] = { charEmb };
				std::vector<float> charaMix;

				std::vector<Ort::Value> inputTensors;

				const auto& srcHiddenUnits = inp_audio.Hidden_Unit[slice];
				const auto& srcF0Data = inp_audio.F0[slice];
				std::vector<float> HiddenUnits; // = inp_audio.Hidden_Unit[slice]
				std::vector<float> F0Data; // = inp_audio.F0[slice]

				if (SV3)
				{
					int64_t upSample = _samplingRate / 16000;
					HiddenUnits.reserve(HubertSize * (upSample + 1));
					for (int64_t itS = 0; itS < HiddenUnitShape[1]; ++itS)
						for (int64_t itSS = 0; itSS < upSample; ++itSS)
							HiddenUnits.insert(HiddenUnits.end(), srcHiddenUnits.begin() + itS * 256, srcHiddenUnits.begin() + (itS + 1) * 256);
					HiddenUnitShape[1] *= upSample;
					HubertSize *= upSample;
					F0Data = GetInterpedF0(InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1])));
					F0Shape[1] = HiddenUnitShape[1];
					XLength[0] = HiddenUnitShape[1];
				}
				else if (SV4)
				{
					HiddenUnits = srcHiddenUnits;
					F0Data = srcF0Data;
				}
				else if (_modelType == modelType::RVC)
				{
					constexpr int64_t upSample = 2;
					HiddenUnits.reserve(HubertSize * (upSample + 1));
					for (int64_t itS = 0; itS < HiddenUnitShape[1]; ++itS)
						for (int64_t itSS = 0; itSS < upSample; ++itSS)
							HiddenUnits.insert(HiddenUnits.end(), srcHiddenUnits.begin() + itS * 256, srcHiddenUnits.begin() + (itS + 1) * 256);
					HiddenUnitShape[1] *= upSample;
					HubertSize *= upSample;
					F0Data = InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1]));
					F0Shape[1] = HiddenUnitShape[1];
					XLength[0] = HiddenUnitShape[1];
					RandnCount = 192 * F0Shape[1];
					RandnShape[2] = F0Shape[1];
				}
				else
				{
					HiddenUnits = srcHiddenUnits;
					F0Shape[1] = HiddenUnitShape[1];
					F0Data = InterpFunc(srcF0Data, long(srcF0Data.size()), long(HiddenUnitShape[1]));
					XLength[0] = HiddenUnitShape[1];
				}

				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, HiddenUnits.data(), HubertSize, HiddenUnitShape, 3));
				if(!SV4)
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
				if(SV3)
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, F0Data.data(), F0Data.size(), F0Shape, 2));
				else if (SV4)
				{
					InterpedF0 = GetInterpedF0(F0Data);
					alignment = GetAligments(F0Shape[1], HubertLen);
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), InterpedF0.size(), F0Shape, 2));
					if (!SVV2)
					{
						UV = GetUV(F0Data);
						SoVitsInput = { "c", "f0", "mel2ph", "uv", "noise", "sid" };
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, UV.data(), UV.size(), F0Shape, 2));
					}
					else
					{
						SoVitsInput = { "c", "f0", "mel2ph", "t_window", "noise", "sid" };
						IstftInput = std::vector<float>(IstftCount, ddsp_noise_scale);
						inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, IstftInput.data(), IstftInput.size(), IstftShape, 3));
					}
					RandnInput = std::vector<float>(RandnCount, 0.f);
					for (auto& it : RandnInput)
						it = normal(gen) * noise_scale;
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));
				}
				else if (_modelType == modelType::RVC)
				{
					InterpedF0 = GetInterpedF0(F0Data);
					Nsff0 = GetNSFF0(InterpedF0);
					inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
					SoVitsInput = RVCInput;
					RandnInput = std::vector<float>(RandnCount, 0.f);
					for (auto& it : RandnInput)
						it = normal(gen) * noise_scale;
				}
				else 
				{
					Nsff0 = GetNSFF0(F0Data);
					inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));
				}
				if(CharaMix)
				{
					CharaMixShape[0] = F0Shape[1];
					if (!inp_audio.Speaker[slice].empty())
						charaMix = inp_audio.Speaker[slice];
					else
					{
						std::vector<float> charaMap(n_speaker, 0.f);
						charaMap[params.chara] = 1.f;
						//std::vector<float>(n_speaker * CharaMixShape[0], 1.f / float(n_speaker));
						charaMix.reserve((n_speaker + 1) * F0Shape[1]);
						for (int64_t index = 0; index < F0Shape[1]; ++index)
							charaMix.insert(charaMix.end(), charaMap.begin(), charaMap.end());
					}
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, charaMix.data(), charaMix.size(), CharaMixShape, 2));
				}
				else
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, Chara, 1, CharaEmbShape, 1));
				
				if (_modelType == modelType::RVC)
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));

				if(VolumeB)
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, inp_audio.Volume[slice].data(), UV.size(), F0Shape, 2));

				std::vector<Ort::Value> finaOut;
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
					throw std::exception((std::string("Locate: sovits\n") + e.what()).c_str());
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
			}
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

std::vector<int16_t> VitsSvc::Inference(std::wstring& _inputLens) const
{
	auto VitsSvcParams = GetSvcParam(_inputLens);
	return InferWithF0AndHiddenUnit(VitsSvcParams);
}

#ifdef WIN32
#ifdef MoeVSMui
std::vector<int16_t> VitsSvc::RTInference(const std::vector<int16_t>& PCMData, long srcSr) const
{
	auto hubertin = InterpResample<float>(PCMData, srcSr, 16000);
	auto source = InterpResample<double>(PCMData, srcSr, _samplingRate);
	const auto params = _get_init_params();
	const auto filter_window_len = params.filter_window_len;
	auto tran = static_cast<int>(params.keys);
	auto featureInput = F0PreProcess(_samplingRate, (short)hop);
	int64_t charEmb = params.chara;
	std::mt19937 gen(int(params.seed));
	std::normal_distribution<float> normal(0, 1);
	float noise_scale = params.noise_scale;
	float ddsp_noise_scale = params.noise_scale_w;
	if (noise_scale > 50.0f)
		noise_scale = 1.0f;
	if (ddsp_noise_scale > 50.0f)
		ddsp_noise_scale = 1.0f;
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
		throw std::exception((std::string("Locate: hubert\n") + e.what()).c_str());
	}
	auto hubertOutData = hubertOut[0].GetTensorMutableData<float>();
	auto hubertOutLen = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
	auto hubertOutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();

	if (KMenas_Stat && params.kmeans_rate > 0.001f)
	{
		for (int64_t indexs = 0; indexs < hubertOutShape[1]; ++indexs)
		{
			const auto curbeg = hubertOutData + indexs * hubertOutShape[2];
			const auto curend = hubertOutData + (indexs + 1) * hubertOutShape[2];
			const auto hu = kmeans_->find({ curbeg ,curend }, long(params.chara));
			for (int64_t ind = 0; ind < hubertOutShape[2]; ++ind)
				*(curbeg + ind) = *(curbeg + ind) * (1.f - params.kmeans_rate) + hu[ind] * params.kmeans_rate;
		}
	}

	std::vector<float> hubOutData(hubertOutData, hubertOutData + hubertOutLen);
	std::vector<float> F0Data;

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
			F0Data = featureInput.GetOrgF0(source.data(), (int64_t)source.size(), hubertOutShape[1], tran);
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
			F0Data = featureInput.GetOrgF0(source.data(), (int64_t)source.size(), hubertOutShape[1], tran);
		}
		else
			throw std::exception("SoVits3.0 Only Support Sr: 48K,32K");
	}
	else if (SV4)
	{
		F0Data = featureInput.GetOrgF0(source.data(), (int64_t)source.size(), int64_t(source.size()) / hop, tran);
	}
	else if (_modelType == modelType::RVC)
	{
		for (int64_t itS = hubertOutShape[1] - 1; itS + 1; --itS)
		{
			hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
		}
		hubertOutShape[1] *= 2;
		hubertOutLen *= 2;
		F0Data = featureInput.GetOrgF0(source.data(), (int64_t)source.size(), hubertOutShape[1], tran);
	}
	else
	{
		F0Data = featureInput.GetOrgF0(source.data(), (int64_t)source.size(), hubertOutShape[1], (long long)tran);
	}

	const size_t F0DataSize = F0Data.size();
	F0Data = mean_filter(F0Data, filter_window_len);
	F0Data.resize(F0DataSize);


	const auto HubertSize = hubOutData.size();
	const auto HubertLen = int64_t(HubertSize) / Hidden_Size;
	const int64_t F0Shape[] = { 1, int64_t(F0Data.size()) };
	const int64_t HiddenUnitShape[] = { 1, HubertLen, Hidden_Size };
	constexpr int64_t LengthShape[] = { 1 };
	int64_t CharaEmbShape[] = { 1 };
	const int64_t RandnShape[] = { 1, 192, F0Shape[1] };
	const int64_t IstftShape[] = { 1, 2048, F0Shape[1] };
	const int64_t RandnCount = F0Shape[1] * 192;
	const int64_t IstftCount = F0Shape[1] * 2048;

	std::vector<float> RandnInput, IstftInput, UV, InterpedF0;
	std::vector<int64_t> alignment;
	int64_t XLength[1] = { HubertLen };
	std::vector<int64_t> Nsff0;
	auto SoVitsInput = soVitsInput;
	int64_t Chara[] = { charEmb };

	std::vector<Ort::Value> inputTensors;
	inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, hubOutData.data(), HubertSize, HiddenUnitShape, 3));
	if (!SV4)
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, XLength, 1, LengthShape, 1));
	if (SV3)
	{
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, F0Data.data(), F0Data.size(), F0Shape, 2));
	}
	else if (SV4)
	{
		InterpedF0 = GetInterpedF0(F0Data);
		alignment = GetAligments(F0Shape[1], HubertLen);
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), InterpedF0.size(), F0Shape, 2));
		if (!SVV2)
		{
			UV = GetUV(F0Data);
			SoVitsInput = { "c", "f0", "mel2ph", "uv", "noise", "sid" };
			inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, UV.data(), UV.size(), F0Shape, 2));
		}
		else
		{
			SoVitsInput = { "c", "f0", "mel2ph", "t_window", "noise", "sid" };
			IstftInput = std::vector<float>(IstftCount, ddsp_noise_scale);
			inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, IstftInput.data(), IstftInput.size(), IstftShape, 3));
		}
		RandnInput = std::vector<float>(RandnCount, 0.f);
		for (auto& it : RandnInput)
			it = normal(gen) * noise_scale;
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));
	}
	else if (_modelType == modelType::RVC)
	{
		InterpedF0 = GetInterpedF0(F0Data);
		Nsff0 = GetNSFF0(InterpedF0);
		inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, InterpedF0.data(), InterpedF0.size(), F0Shape, 2));
		SoVitsInput = RVCInput;
		RandnInput = std::vector<float>(RandnCount, 0.f);
		for (auto& it : RandnInput)
			it = normal(gen) * noise_scale;
	}
	else
	{
		Nsff0 = GetNSFF0(F0Data);
		inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, Nsff0.data(), Nsff0.size(), F0Shape, 2));

	}
	std::vector<float> charaMix;
	if (CharaMix)
	{
		CharaEmbShape[0] = n_speaker;
		if (size_t(n_speaker) == params.chara_mix.size())
			charaMix = params.chara_mix;
		else
			charaMix = std::vector<float>(n_speaker, 1.f / float(n_speaker));
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, charaMix.data(), 1, CharaEmbShape, 1));
	}
	else
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, Chara, 1, CharaEmbShape, 1));

	if (_modelType == modelType::RVC)
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, RandnInput.data(), RandnCount, RandnShape, 3));
	
	std::vector<float> Volume;

	if (VolumeB)
	{
		inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, Volume.data(), UV.size(), F0Shape, 2));
	}

	std::vector<Ort::Value> finaOut;
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
		throw std::exception((std::string("Locate: sovits\n") + e.what()).c_str());
	}

	const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
	const auto dstWavLen = shapeOut[2];
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
	return TempVecWav;
}

/*
void VitsSvc::StartRT(Mui::Window::UIWindowBasic* window)
{
	if (RTSTAT)
		return;

	RTParams = _get_init_params();
	std::wstring error;

	recoder = new MRecorder();
	recoder->InitRecorder(error, _samplingRate, 1, 16, Mui::_m_uint(_samplingRate * RTParams.rt_batch_length / 1000.0));
	recoder->Start();

	audio_player = new Mui::MDS_AudioPlayer(window);
	audio_player->InitAudioPlayer(error);

	audio_stream = audio_player->CreateStreamPCM(recoder->GetFrameSize(), recoder->GetSampleRate(), recoder->GetChannels(), recoder->GetBitRate(), recoder->GetBlockAlign());
	audio_player->PlayTrack(audio_stream);

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
				if (rawInputBuffer.size() > RTParams.rt_batch_count)
				{
					std::vector<int16_t> pBuffer;
					pBuffer.reserve(RTParams.rt_batch_count * rawInputBuffer[0].size());
					for (uint64_t idx = 0; idx < RTParams.rt_batch_count; ++idx)
						pBuffer.insert(pBuffer.end(), rawInputBuffer[idx].begin(), rawInputBuffer[idx].end());
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
							if(i16data> RTParams.rt_th)
							{
								zeroVector = false;
								break;
							}
						}
						if(zeroVector)
							rawOutputBuffer.emplace_back(std::vector<int16_t>(inputBuffer[0].size(), 0), inputBuffer[0].size() / PCMSIZE);
						else
						{
							auto RtRES = RTInference(inputBuffer[0], _samplingRate);
							rawOutputBuffer.emplace_back(std::move(RtRES), inputBuffer[0].size() / PCMSIZE);
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
						throw std::exception(e.what());
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
			const auto PCMSIZE = recoder->GetFrameSize() / 2;
			while (RTSTAT)
			{
				if (rawOutputBuffer.size() > 1)
				{
					const auto olength = PCMSIZE; //rawOutputBuffer[0].first.size() / rawOutputBuffer[0].second;
					const auto adata = rawOutputBuffer[0].first.data() + olength * rawOutputBuffer[0].second / 2,
					bdata = rawOutputBuffer[1].first.data() + olength * (rawOutputBuffer[1].second / 2 - 1);
					std::vector<int16_t> pBuffer;
					pBuffer.reserve(PCMSIZE);
					for (size_t idx = 0; idx < olength; ++idx)
						pBuffer.emplace_back(int16_t(double(adata[idx]) * (double(idx) / double(olength)) + double(bdata[idx]) * (double(olength - idx) / double(olength))));
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
	for (uint64_t idx = 0; idx < RTParams.rt_batch_count / 2; ++idx)
		rawInputBuffer.emplace_back(recoder->GetFrameSize() / 2, 0);
	RTSTAT = true;
	RT_RECORD_THREAD.detach();
	RT_INPUT_CROSSFADE_THREAD.detach();
	RT_INFERENCE_THREAD.detach();
	RT_OUTPUT_CROSSFADE_THREAD.detach();
	RT_OUTPUT_THREAD.detach();
	while (true);
}
 */
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

INFERCLASSEND