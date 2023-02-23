#include <queue>

#include "../Window/MainWindow.h"
#include "MioShirakanaTensor.hpp"
#include "inferTools.hpp"
#include "..\Helper\Helper.h"
#include <random>
#include "../Lib/rapidjson/stringbuffer.h"
#include "../AvCodeResample.h"
#pragma warning(disable : 4996)
const std::vector<const char*> ganIn = { "x" };
const std::vector<const char*> ganOut = { "audio" };
const std::vector<const char*> inputNodeNamesSessionEncoder = { "sequences","sequence_lengths" };
const std::vector<const char*> outputNodeNamesSessionEncoder = { "memory","processed_memory","lens" };
const std::vector<const char*> inputNodeNamesSessionDecoderIter = { "decoder_input","attention_hidden","attention_cell","decoder_hidden","decoder_cell","attention_weights","attention_weights_cum","attention_context","memory","processed_memory","mask" };
const std::vector<const char*> outputNodeNamesSessionDecoderIter = { "decoder_output","gate_prediction","out_attention_hidden","out_attention_cell","out_decoder_hidden","out_decoder_cell","out_attention_weights","out_attention_weights_cum","out_attention_context" };
const std::vector<const char*> inputNodeNamesSessionPostNet = { "mel_outputs" };
const std::vector<const char*> outputNodeNamesSessionPostNet = { "mel_outputs_postnet" };

const std::vector<const char*> DecInputTmp = { "z_in", "g" };
const std::vector<const char*> DecOutput = { "o" };
const std::vector<const char*> DpInputTmp = { "x", "x_mask", "zin", "g" };
const std::vector<const char*> DpOutput = { "logw" };
const std::vector<const char*> EncInput = { "x", "x_lengths" };
const std::vector<const char*> EnceInput = { "x", "x_lengths", "emotion" };
const std::vector<const char*> EncOutput = { "xout", "m_p", "logs_p", "x_mask" };
const std::vector<const char*> FlowInputTmp = { "z_p", "y_mask", "g" };
const std::vector<const char*> FlowOutput = { "z" };
const std::vector<const char*> EMBInput = { "sid" };
const std::vector<const char*> EMBOutput = { "g" };

const std::vector<const char*> hubertOutput = { "embed" };
const std::vector<const char*> soVitsOutput = { "audio" };
const std::vector<const char*> hubertInput = { "source" };
const std::vector<const char*> soVitsInput = { "hidden_unit", "lengths", "pitch", "sid" };

const std::vector<const char*> nsfInput = { "c", "f0" };
const std::vector<const char*> nsfOutput = { "audio" };
const std::vector<const char*> DiffInput = { "hubert", "mel2ph", "spk_embed", "f0", "initial_noise", "speedup"};
const std::vector<const char*> DiffOutput = { "mel_pred", "f0_pred" };

const std::vector<const char*> encoderInput = { "hubert", "mel2ph", "spk_embed", "f0" };
const std::vector<const char*> denoiseInput = { "noise", "time", "condition" };
const std::vector<const char*> predInput = { "noise", "noise_pred", "time", "time_prev" };
const std::vector<const char*> afterInput = { "x" };
const std::vector<const char*> encoderOutput = { "mel_pred", "f0_pred" };
const std::vector<const char*> denoiseOutput = { "noise_pred" };
const std::vector<const char*> predOutput = { "noise_pred_o" };
const std::vector<const char*> afterOutput = { "mel_out" };

const std::vector<const char*> SingerEncoderInput = { "tokens", "durations", "f0", "spk_embed"};
const std::vector<const char*> SingerEncoderOutput = { "condition" };
const std::vector<const char*> SingerHifiganInput = { "mel", "f0" };
const std::vector<const char*> SingerHifiganOutput = { "waveform" };

const std::vector<const char*> SingerInput = { "tokens", "durations", "f0", "speedup", "spk_embed"};
const std::vector<const char*> SingerOutput = { "mel" };

void cat(std::vector<float>& tensorA, std::vector<int64>& Shape, const MTensor& tensorB) {
	const int64 n = Shape[1];
	for (int64 i = n; i > 0; --i)
		tensorA.insert(tensorA.begin() + (i * Shape[2]), tensorB.GetTensorData<float>()[i - 1]);
	++Shape[2];
}

namespace ttsUI
{
	class BoolTensor
	{
	public:
		BoolTensor() = default;
		BoolTensor(size_t _size, bool end)
		{
			_data = new bool[_size];
			for (size_t i = 0; i < _size; ++i)
				_data[i] = false;
			if (end)
				_data[_size - 1] = true;
		}
		~BoolTensor()
		{
			delete[] _data;
		}
		bool* data() const
		{
			return _data;
		}
		void set(size_t _size, bool end)
		{
			delete[] _data;
			_data = new bool[_size];
			for (size_t i = 0; i < _size; ++i)
				_data[i] = false;
			if (end)
				_data[_size - 1] = true;
		}
	private:
		bool* _data = nullptr;
	};

	std::wregex changeLineSymb(L"\r");

	std::vector<float> MioTTSControl::GetEmotionVector(std::wstring src)
	{
		std::vector<float> dst(1024, 0.0);
		std::wsmatch mat;
		uint64_t mul = 0;
		while(std::regex_search(src ,mat, EmoReg))
		{
			long emoId;
			const auto emoStr = to_byte_string(mat.str());
			if (!EmoJson[emoStr.c_str()].Empty())
				emoId = EmoJson[emoStr.c_str()].GetInt();
			else
				emoId = atoi(emoStr.c_str());
			auto emoVec = emoLoader[emoId];
			for (size_t i = 0; i < 1024; ++i)
				dst[i] = dst[i] + (emoVec[i] - dst[i]) / (float)(mul + 1ull);
			src = mat.suffix();
			++mul;
		}
		return dst;
	}

	void MioTTSControl::InferTaco2(const std::wstring& _input)
	{
		std::vector<int64_t> text;
		text.reserve(_input.length());
		for (size_t i = 0; i < _input.length(); i++)
			text[i] = _symbol[_input[i]];
		int64 textLength[1] = { static_cast<int64>(_input.length()) };
		std::vector<MTensor> inputTensors;
		const int64 inputShape1[2] = { 1,textLength[0] };
		constexpr int64 inputShape2[1] = { 1 };
		inputTensors.push_back(MTensor::CreateTensor<int64>(
			*memory_info, text.data(), textLength[0], inputShape1, 2));
		inputTensors.push_back(MTensor::CreateTensor<int64>(
			*memory_info, textLength, 1, inputShape2, 1));
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
		auto BooleanVec = new bool[seqLen[0] * seqLen[1]];
		memset(BooleanVec, 0, seqLen[0] * seqLen[1] * sizeof(bool));
		inputTensors.push_back(MTensor::CreateTensor<bool>(
			*memory_info, BooleanVec, seqLen[0] * seqLen[1], seqLen.data(), seqLen.size()));
		int32_t notFinished = 1;
		int32_t melLengths = 0;
		std::vector<float> melGateAlig;
		std::vector<int64> melGateAligShape;
		bool firstIter = true;
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
				MessageBox(hWnd, to_wide_string(e.what()).c_str(), L"失败", MB_OK);
				delete[] BooleanVec;
				return;
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
				delete[] BooleanVec;
				throw(std::exception("reach max decode steps"));
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
		outputTensors = sessionPostNet->Run(Ort::RunOptions{ nullptr },
			inputNodeNamesSessionPostNet.data(),
			melInput.data(),
			melInput.size(),
			outputNodeNamesSessionPostNet.data(),
			outputNodeNamesSessionPostNet.size());
		const std::vector<MTensor> wavOuts = sessionGan->Run(Ort::RunOptions{ nullptr },
			ganIn.data(),
			outputTensors.data(),
			outputTensors.size(),
			ganOut.data(),
			ganOut.size());
		const std::vector<int64> wavOutsSharp = wavOuts[0].GetTensorTypeAndShapeInfo().GetShape();
		const auto TempVecWav = static_cast<int16_t*>(malloc(sizeof(int16_t) * wavOutsSharp[2]));
		if (TempVecWav == nullptr)
		{
			delete[] BooleanVec;
			return;
		}
		for (int i = 0; i < wavOutsSharp[2]; i++) {
			*(TempVecWav + i) = static_cast<int16_t>(wavOuts[0].GetTensorData<float>()[i] * 32768.0f);
		}
		_wavData.insert(_wavData.end(), TempVecWav, TempVecWav + (wavOutsSharp[2]));
		free(TempVecWav);
		delete[] BooleanVec;
	}

	/*
	void MioTTSControl::InferVits(const std::vector<std::wstring>& _input)
	{
		std::wstring noises = noise_scale_box->GetCurText(), noisews = noise_scale_w_box->GetCurText(), lengths = length_scale_box->GetCurText();
		auto noise_scale = static_cast<float>(_wtof(noises.c_str()));
		auto noise_scale_w = static_cast<float>(_wtof(noisews.c_str()));
		auto length_scale = static_cast<float>(_wtof(lengths.c_str()));
		if (noise_scale < 0.0f || noise_scale > 50.0f)
			noise_scale = 0.667f;
		if (noise_scale_w < 0.0f || noise_scale_w > 50.0f)
			noise_scale_w = 0.800f;
		if (length_scale < 0.01f || length_scale > 50.0f)
			length_scale = 1.000f;
		const auto seed = _wtoi(tran_seeds->GetCurText().c_str());
		//
		std::mt19937 gen(static_cast<unsigned int>(seed));
		std::normal_distribution<float> normal(0, 1);
		int procs = 0;
		for (const auto& _inputStr : _input)
		{
			std::vector<int64> text;
			text.reserve((_inputStr.length() + 2) * 2);
			for (auto it : _inputStr)
			{
				text.push_back(0);
				text.push_back(_symbol[it]);
			}
			text.push_back(0);
			int64 textLength[] = { (int64)text.size() };
			std::vector<MTensor> outputTensors;
			std::vector<MTensor> inputTensors;
			const int64 inputShape1[2] = { 1,textLength[0] };
			constexpr int64 inputShape2[1] = { 1 };
			inputTensors.push_back(MTensor::CreateTensor<int64>(
				*memory_info, text.data(), textLength[0], inputShape1, 2));
			inputTensors.push_back(MTensor::CreateTensor<int64>(
				*memory_info, textLength, 1, inputShape2, 1));
			try
			{
				outputTensors = sessionEnc_p->Run(Ort::RunOptions{ nullptr },
					EncInput.data(),
					inputTensors.data(),
					inputTensors.size(),
					EncOutput.data(),
					EncOutput.size());
			}
			catch (Ort::Exception& e)
			{
				throw std::exception((std::string("Locate: enc_p\n") + e.what()).c_str());
			}
			std::vector<float>
				x(outputTensors[0].GetTensorData<float>(), outputTensors[0].GetTensorData<float>() + outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount()),
				m_p(outputTensors[1].GetTensorData<float>(), outputTensors[1].GetTensorData<float>() + outputTensors[1].GetTensorTypeAndShapeInfo().GetElementCount()),
				logs_p(outputTensors[2].GetTensorData<float>(), outputTensors[2].GetTensorData<float>() + outputTensors[2].GetTensorTypeAndShapeInfo().GetElementCount()),
				x_mask(outputTensors[3].GetTensorData<float>(), outputTensors[3].GetTensorData<float>() + outputTensors[3].GetTensorTypeAndShapeInfo().GetElementCount());

			//  x = STensor::copy<float>(outputTensors[0], *memory_info, _ptrs),
			//	m_p = STensor::copy<float>(outputTensors[1], *memory_info, _ptrs),
			//	logs_p = STensor::copy<float>(outputTensors[2], *memory_info, _ptrs),
			//	x_mask = STensor::copy<float>(outputTensors[3], *memory_info, _ptrs);

			const auto xshape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

			const int64 zinputShape[3] = { xshape[0],2,xshape[2] };
			const int64 zinputCount = xshape[0] * xshape[2] * 2;
			std::vector<float> zinput(zinputCount, 0.0);
			for (auto& it : zinput)
				it = normal(gen) * noise_scale_w;

			inputTensors.clear();
			inputTensors.push_back(std::move(outputTensors[0]));
			inputTensors.push_back(std::move(outputTensors[3]));
			inputTensors.push_back(MTensor::CreateTensor<float>(
				*memory_info, zinput.data(), zinputCount, zinputShape, 3));
			try
			{
				outputTensors = sessionDp->Run(Ort::RunOptions{ nullptr },
					DpSInput.data(),
					inputTensors.data(),
					inputTensors.size(),
					DpOutput.data(),
					DpOutput.size());
			}
			catch (Ort::Exception& e)
			{
				throw std::exception((std::string("Locate: dp\n") + e.what()).c_str());
			}
			const auto w_ceil = outputTensors[0].GetTensorMutableData<float>();
			const size_t w_ceilSize = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
			for (size_t i = 0; i < w_ceilSize; ++i)
				w_ceil[i] = ceil(exp(w_ceil[i]) * x_mask[i] * length_scale);
			if (customPathStat->GetSel())
			{
				std::vector<UISliderLayer::itemParam> dstValue;
				dstValue.reserve((_inputStr.length()));
				for (size_t itr = 0; itr < _inputStr.length(); ++itr)
					dstValue.emplace_back(UISliderLayer::itemParam(_inputStr[itr] + std::to_wstring((int)w_ceil[itr * 2 + 1]), 1.0f));
				auto callback = [&](const std::vector<float>& input)
				{
					for (size_t itr = 0; itr < input.size(); ++itr)
						w_ceil[itr * 2 + 1] = ceil(w_ceil[itr * 2 + 1] * input[itr]);
					_mainWindow->tts_layer->opened = false;
				};
				_mainWindow->tts_layer->ShowLayer(L"设置音素Duration的倍率", std::move(dstValue), callback);
				while (_mainWindow->tts_layer->opened)
					Sleep(200);
			}
			float y_length_f = 0.0;
			int64 y_length;
			for (size_t i = 0; i < w_ceilSize; ++i)
				y_length_f += w_ceil[i];
			if (y_length_f < 1.0f)
				y_length = 1;
			else
				y_length = (int64)y_length_f;
			const auto maskSize = x_mask.size();
			auto attn = generatePath(w_ceil, y_length, maskSize);
			std::vector<std::vector<float>> logVec(192, std::vector<float>(y_length, 0.0f));
			std::vector<std::vector<float>> mpVec(192, std::vector<float>(y_length, 0.0f));
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
					nlogs_pData[j * y_length + i] = mpVec[j][i] + normal(gen) * exp(logVec[j][i]) * noise_scale;
				}
			}
			inputTensors.clear();
			std::vector<float> y_mask(y_length);
			for (size_t i = 0; i < static_cast<size_t>(y_length); ++i)
				y_mask[i] = 1.0f;
			const int64 zshape[3] = { 1,192,y_length };
			const int64 yshape[3] = { 1,1,y_length };
			try
			{
				inputTensors.push_back(MTensor::CreateTensor<float>(
					*memory_info, nlogs_pData.data(), 192 * y_length, zshape, 3));
				inputTensors.push_back(MTensor::CreateTensor<float>(
					*memory_info, y_mask.data(), y_length, yshape, 3));
				outputTensors = sessionFlow->Run(Ort::RunOptions{ nullptr },
					FlowSInput.data(),
					inputTensors.data(),
					inputTensors.size(),
					FlowOutput.data(),
					FlowOutput.size());
				inputTensors[0] = std::move(outputTensors[0]);
				inputTensors.pop_back();
				outputTensors = sessionDec->Run(Ort::RunOptions{ nullptr },
					DecSInput.data(),
					inputTensors.data(),
					inputTensors.size(),
					DecOutput.data(),
					DecOutput.size());
			}
			catch (Ort::Exception& e)
			{
				throw std::exception((std::string("Locate: dec & flow\n") + e.what()).c_str());
			}
			const auto shapeOut = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
			const auto TempVecWav = static_cast<int16_t*>(malloc(sizeof(int16_t) * shapeOut[2]));
			if (TempVecWav == nullptr)
				throw std::exception("OutOfMemory");
			for (int bbb = 0; bbb < shapeOut[2]; bbb++)
				*(TempVecWav + bbb) = static_cast<int16_t>(outputTensors[0].GetTensorData<float>()[bbb] * 32768.0f);
			_wavData.insert(_wavData.end(), TempVecWav, TempVecWav + (shapeOut[2]));
			free(TempVecWav);
			process->SetAttribute(L"value", std::to_wstring(++procs));
		}
	}
	 */

	void MioTTSControl::InferVits(const std::vector<std::wstring>& _input, const std::vector<std::vector<std::wstring>>& Params)
	{
		int procs = 0;
		for (const auto& _inputStr : _input)
		{
			if (_inputStr.empty())
				continue;
			auto noise_scale = static_cast<float>(_wtof(Params[procs][0].c_str()));
			auto noise_scale_w = static_cast<float>(_wtof(Params[procs][1].c_str()));
			auto length_scale = static_cast<float>(_wtof(Params[procs][2].c_str()));
			if (noise_scale < 0.0f || noise_scale > 50.0f)
				noise_scale = 0.667f;
			if (noise_scale_w < 0.0f || noise_scale_w > 50.0f)
				noise_scale_w = 0.800f;
			if (length_scale < 0.01f || length_scale > 50.0f)
				length_scale = 1.000f;
			const auto seed = _wtoi(Params[procs][3].c_str());
			const auto chara = _wtoi(Params[procs][4].c_str());
			//
			std::mt19937 gen(static_cast<unsigned int>(seed));
			std::normal_distribution<float> normal(0, 1);

			std::vector<int64> text;
			text.reserve((_inputStr.length() + 2) * 2);
			for (auto it : _inputStr)
			{
				text.push_back(0);
				text.push_back(_symbol[it]);
			}
			text.push_back(0);
			int64 textLength[] = { (int64)text.size() };
			std::vector<MTensor> outputTensors;
			std::vector<MTensor> inputTensors;
			const int64 inputShape1[2] = { 1,textLength[0] };
			constexpr int64 inputShape2[1] = { 1 };
			inputTensors.push_back(MTensor::CreateTensor<int64>(
				*memory_info, text.data(), textLength[0], inputShape1, 2));
			inputTensors.push_back(MTensor::CreateTensor<int64>(
				*memory_info, textLength, 1, inputShape2, 1));
			if(emo)
			{
				constexpr int64 inputShape3[1] = { 1024 };
				auto emoVec = GetEmotionVector(Params[procs][5]);
				inputTensors.push_back(MTensor::CreateTensor<float>(
					*memory_info, emoVec.data(), 1024, inputShape3, 1));
				try
				{
					outputTensors = sessionEnc_p->Run(Ort::RunOptions{ nullptr },
						EnceInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						EncOutput.data(),
						EncOutput.size());
				}
				catch (Ort::Exception& e)
				{
					throw std::exception((std::string("Locate: enc_p\n") + e.what()).c_str());
				}
			}
			else
			{
				try
				{
					outputTensors = sessionEnc_p->Run(Ort::RunOptions{ nullptr },
						EncInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						EncOutput.data(),
						EncOutput.size());
				}
				catch (Ort::Exception& e)
				{
					throw std::exception((std::string("Locate: enc_p\n") + e.what()).c_str());
				}
			}
			std::vector<float>
				m_p(outputTensors[1].GetTensorData<float>(), outputTensors[1].GetTensorData<float>() + outputTensors[1].GetTensorTypeAndShapeInfo().GetElementCount()),
				logs_p(outputTensors[2].GetTensorData<float>(), outputTensors[2].GetTensorData<float>() + outputTensors[2].GetTensorTypeAndShapeInfo().GetElementCount()),
				x_mask(outputTensors[3].GetTensorData<float>(), outputTensors[3].GetTensorData<float>() + outputTensors[3].GetTensorTypeAndShapeInfo().GetElementCount());

			const auto xshape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
			std::vector<const char*> DpInput = DpInputTmp, FlowInput = FlowInputTmp, DecInput = DecInputTmp;
			std::vector<MTensor> inputSid;
			std::vector<MTensor> outputG;
			if(sessionEmb)
			{
				int64 Character[1] = { chara };
				inputSid.push_back(MTensor::CreateTensor<int64>(
					*memory_info, Character, 1, inputShape2, 1));
				try {
					outputG = sessionEmb->Run(Ort::RunOptions{ nullptr },
						EMBInput.data(),
						inputSid.data(),
						inputSid.size(),
						EMBOutput.data(),
						EMBOutput.size());
				}
				catch (Ort::Exception& e)
				{
					throw std::exception((std::string("Locate: emb\n") + e.what()).c_str());
				}
			}
			else
			{
				DpInput.pop_back();
				FlowInput.pop_back();
				DecInput.pop_back();
			}
			const int64 zinputShape[3] = { xshape[0],2,xshape[2] };
			const int64 zinputCount = xshape[0] * xshape[2] * 2;
			std::vector<float> zinput(zinputCount, 0.0);
			for (auto& it : zinput)
				it = normal(gen) * noise_scale_w;

			inputTensors.clear();
			inputTensors.push_back(std::move(outputTensors[0]));
			inputTensors.push_back(std::move(outputTensors[3]));
			inputTensors.push_back(MTensor::CreateTensor<float>(
				*memory_info, zinput.data(), zinputCount, zinputShape, 3));
			std::vector<int64> GShape;
			if (sessionEmb){
				GShape = outputG[0].GetTensorTypeAndShapeInfo().GetShape();
				GShape.push_back(1);
				inputTensors.push_back(MTensor::CreateTensor<float>(
					*memory_info, outputG[0].GetTensorMutableData<float>(), outputG[0].GetTensorTypeAndShapeInfo().GetElementCount(), GShape.data(), 3));
			}
			try
			{
				outputTensors = sessionDp->Run(Ort::RunOptions{ nullptr },
					DpInput.data(),
					inputTensors.data(),
					inputTensors.size(),
					DpOutput.data(),
					DpOutput.size());
			}
			catch (Ort::Exception& e)
			{
				throw std::exception((std::string("Locate: dp\n") + e.what()).c_str());
			}
			const auto w_ceil = outputTensors[0].GetTensorMutableData<float>();
			const size_t w_ceilSize = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
			for (size_t i = 0; i < w_ceilSize; ++i)
				w_ceil[i] = ceil(exp(w_ceil[i]) * x_mask[i] * length_scale);
			const auto maskSize = x_mask.size();
			if (customPathStat->GetSel())
			{
				std::vector<UISliderLayer::itemParam> dstValue;
				dstValue.reserve((_inputStr.length()));
				for (size_t itr = 0; itr < _inputStr.length(); ++itr)
					dstValue.emplace_back(UISliderLayer::itemParam(_inputStr[itr]+std::to_wstring((int)w_ceil[itr*2+1]), 1.0f));
				auto callback = [&](const std::vector<float>& input)
				{
					for (size_t itr = 0; itr < input.size(); ++itr)
						w_ceil[itr * 2 + 1] = ceil(w_ceil[itr * 2 + 1] * input[itr]);
					_mainWindow->tts_layer->opened = false;
				};
				_mainWindow->tts_layer->ShowLayer(L"设置音素Duration的倍率", std::move(dstValue), callback);
				while (_mainWindow->tts_layer->opened)
					Sleep(200);
			}
			float y_length_f = 0.0;
			int64 y_length;
			for (size_t i = 0; i < w_ceilSize; ++i)
				y_length_f += w_ceil[i];
			if (y_length_f < 1.0f)
				y_length = 1;
			else
				y_length = (int64)y_length_f;
			auto attn = generatePath(w_ceil, y_length, maskSize);
			std::vector<std::vector<float>> logVec(192, std::vector<float>(y_length, 0.0f));
			std::vector<std::vector<float>> mpVec(192, std::vector<float>(y_length, 0.0f));
			std::vector<float> nlogs_pData(192 * y_length);
			for (size_t i = 0; i < static_cast<size_t>(y_length); ++i)
			{
				for (size_t j = 0; j < 192; ++j)
				{
					for (size_t k = 0; k < maskSize; k++)
					{
						if(attn[i][k])
						{
							mpVec[j][i] += m_p[j * maskSize + k];
							logVec[j][i] += logs_p[j * maskSize + k];
						}
					}
					nlogs_pData[j * y_length + i] = mpVec[j][i] + normal(gen) * exp(logVec[j][i]) * noise_scale;
				}
			}
			inputTensors.clear();
			std::vector<float> y_mask(y_length);
			for (size_t i = 0; i < static_cast<size_t>(y_length); ++i)
				y_mask[i] = 1.0f;
			const int64 zshape[3] = { 1,192,y_length };
			const int64 yshape[3] = { 1,1,y_length };
			try
			{
				inputTensors.push_back(MTensor::CreateTensor<float>(
					*memory_info, nlogs_pData.data(), 192 * y_length, zshape, 3));
				inputTensors.push_back(MTensor::CreateTensor<float>(
					*memory_info, y_mask.data(), y_length, yshape, 3));
				if (sessionEmb) 
				{
					inputTensors.push_back(MTensor::CreateTensor<float>(
						*memory_info, outputG[0].GetTensorMutableData<float>(), outputG[0].GetTensorTypeAndShapeInfo().GetElementCount(), GShape.data(), 3));
				}
				outputTensors = sessionFlow->Run(Ort::RunOptions{ nullptr },
					FlowInput.data(),
					inputTensors.data(),
					inputTensors.size(),
					FlowOutput.data(),
					FlowOutput.size());
				inputTensors[0] = std::move(outputTensors[0]);
				if (sessionEmb)
					inputTensors[1] = std::move(inputTensors[2]);
				inputTensors.pop_back();
				outputTensors = sessionDec->Run(Ort::RunOptions{ nullptr },
					DecInput.data(),
					inputTensors.data(),
					inputTensors.size(),
					DecOutput.data(),
					DecOutput.size());
			}
			catch (Ort::Exception& e)
			{
				throw std::exception((std::string("Locate: dec & flow\n") + e.what()).c_str());
			}
			const auto shapeOut = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
			const auto TempVecWav = static_cast<int16_t*>(malloc(sizeof(int16_t) * shapeOut[2]));
			if (TempVecWav == nullptr) 
				throw std::exception("OutOfMemory");
			for (int bbb = 0; bbb < shapeOut[2]; bbb++)
				*(TempVecWav + bbb) = static_cast<int16_t>(outputTensors[0].GetTensorData<float>()[bbb] * 32768.0f);
			_wavData.insert(_wavData.end(), TempVecWav, TempVecWav + (shapeOut[2]));
			free(TempVecWav);
			process->SetAttribute(L"value", std::to_wstring(++procs));
		}
	}

	std::wstring MioTTSControl::getCleanerStr(std::wstring& input) const {
		if(_modelType == modelType::Vits || _modelType == modelType::Taco)
		{
			input += L'\n';
			input = std::regex_replace(input, changeLineSymb, L"\n");
			std::wstring output;
			output.reserve(input.size());
			auto fit = input.find(L'\n');
			while (fit != std::wstring::npos)
			{
				std::wstring tmp = input.substr(0, fit);
				const size_t fit_t = tmp.rfind(EndString);
				if (fit_t != std::wstring::npos)
				{
					output += tmp.substr(0, fit_t + 1);
					tmp = tmp.substr(fit_t + 1);
				}
				tmp = pluginApi.functionAPI(tmp);
				output += tmp + L'\n';
				input = input.substr(fit + 1);
				fit = input.find(L'\n');
			}
			return output;
		}
		return pluginApi.functionAPI(input);
	}

	void MioTTSControl::inferSovits(std::wstring& path, const CutInfo& cutinfo, int charEmb)
	{
		if (path[0] == L'\"')
			path = path.substr(1);
		if (path[path.length() - 1] == L'\"')
			path.pop_back();
		auto tran = static_cast<int>(cutinfo.keys);
		auto threshold = static_cast<double>(cutinfo.threshold);
		auto minLen = static_cast<unsigned long>(cutinfo.minLen);
		auto frame_len = static_cast<unsigned long>(cutinfo.frame_len);
		auto frame_shift = static_cast<unsigned long>(cutinfo.frame_shift);
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
		//Audio
		auto RawWav = AudioPreprocess().codec(path, sampingRate);
		auto HubertWav = AudioPreprocess().codec(path, 16000);
		auto info = cutWav(RawWav, threshold, minLen, static_cast<unsigned short>(frame_len), static_cast<unsigned short>(frame_shift));
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
			if ((info.cutOffset[i] - info.cutOffset[i - 1]) / RawWav.getHeader().bytesPerSec > 90)
				throw std::exception("Reached max slice length, please change slicer param");
		process->SetAttribute(L"value", L"0");
		process->SetAttribute(L"max", std::to_wstring(info.cutTag.size()));
		const auto LenFactor = ((double)16000 / (double)sampingRate);
		std::mt19937 gen(_wtoi(tran_seeds->GetCurText().c_str()));
		std::normal_distribution<float> normal(0, 1);
		float noise_scale = (float)_wtof(noise_scale_box->GetCurText().c_str());
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
		{
			if(info.cutTag[i-1])
			{
				auto featureInput = FeatureInput(sampingRate, (short)curHop);
				//输入处理
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				const auto srcPos = info.cutOffset[i - 1] / 2;
				std::vector<double> source(len, 0.0);
				//const auto source = new double[len];//source
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
				}catch (Ort::Exception& e)
				{
					throw std::exception((std::string("Locate: hubert\n") + e.what()).c_str());
				}
				auto hubertOutData = hubertOut[0].GetTensorMutableData<float>();
				auto hubertOutLen = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
				auto hubertOutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();

				std::vector<long long> inputPitch1, alignment;
				std::vector<float> inputPitch2;
				FeatureInput::V4Return v4_return;
				std::vector<float> hubOutData(hubertOutData, hubertOutData + hubertOutLen);
				if(Sv3)
				{
					if (sampingRate / 16000 == 2)
					{
						for (int64_t itS = hubertOutShape[1] - 1; itS + 1; --itS)
						{
							hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
						}
						hubertOutShape[1] *= 2;
						hubertOutLen *= 2;
						inputPitch2 = featureInput.transcribe3(source.data(), (long long)len, (long long)hubertOutShape[1], (long long)tran);
					}
					else if (sampingRate / 16000 == 3)
					{
						for (int64_t itS = hubertOutShape[1] - 1; itS + 1; --itS)
						{
							hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
							hubOutData.insert(hubOutData.begin() + itS * 256, hubOutData.begin() + itS * 256, hubOutData.begin() + (itS + 1) * 256);
						}
						hubertOutShape[1] *= 3;
						hubertOutLen *= 3;
						inputPitch2 = featureInput.transcribe3(source.data(), (long long)len, (long long)hubertOutShape[1], (long long)tran);
					}
					else
						throw std::exception("SoVits3.0 Only Support Sr: 48K,32K");
				}
				else if(Sv4)
				{
					alignment = getAligments(len / curHop, hubertOutShape[1]);
					v4_return = featureInput.transcribe4(source.data(), (long long)len, tran);
				}
				else
				{
					inputPitch1 = featureInput.transcribe(source.data(), (long long)len, (long long)hubertOutShape[1], (long long)tran);
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
				if(!Sv4)
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, ainput, 1, ashape, 1));
				if (Sv3)
					inputTensors.emplace_back(Ort::Value::CreateTensor<float>(*memory_info, inputPitch2.data(), featureInput.getLen(), bshape, 2));
				else if (Sv4)
				{
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, v4_return.f0.data(), featureInput.getLen(), bshape, 2));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), featureInput.getLen(), bshape, 2));
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, v4_return.uv.data(), featureInput.getLen(), bshape, 2));
					for (auto& it : zinput)
						it = normal(gen) * noise_scale;
					inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, zinput.data(), zinputCount, zinputShape, 3));
					SoVitsInput = { "c", "f0", "mel2ph", "uv", "noise", "sid" };
				}
				else
					inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, inputPitch1.data(), featureInput.getLen(), bshape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, cdata, 1, cshape, 1));
				std::vector<Ort::Value> finaOut;
				try
				{
					finaOut = soVits->Run(Ort::RunOptions{ nullptr },
						SoVitsInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						soVitsOutput.data(),
						soVitsOutput.size());
				}catch (Ort::Exception& e)
				{
					throw std::exception((std::string("Locate: sovits\n") + e.what()).c_str());
				}

				const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<int16_t> TempVecWav(shapeOut[2], 0);
				for (int bbb = 0; bbb < shapeOut[2]; bbb++) {
					TempVecWav[bbb] = static_cast<int16_t>(finaOut[0].GetTensorData<float>()[bbb] * 32768.0f);
				}
				_wavData.insert(_wavData.end(), TempVecWav.data(), TempVecWav.data() + (shapeOut[2]));
			}else
			{
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				const auto data = new int16_t[len];
				memset(data, 0, len * 2);
				_wavData.insert(_wavData.end(), data, data + len);
				delete[] data;
			}
			process->SetAttribute(L"value", std::to_wstring(i + 1));
		}
	}

	void MioTTSControl::inferSvc(std::wstring& path, const CutInfo& cutinfo, int charEmb)
	{
		if (path[0] == L'\"')
			path = path.substr(1);
		if (path[path.length() - 1] == L'\"')
			path.pop_back();
		std::mt19937 gen(_wtoi(tran_seeds->GetCurText().c_str()));
		std::normal_distribution<float> normal(0, 1);
		auto tran = static_cast<int>(cutinfo.keys);
		auto threshold = static_cast<double>(cutinfo.threshold);
		auto minLen = static_cast<unsigned long>(cutinfo.minLen);
		auto frame_len = static_cast<unsigned long>(cutinfo.frame_len);
		auto frame_shift = static_cast<unsigned long>(cutinfo.frame_shift);
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
		//Audio
		auto RawWav = AudioPreprocess().codec(path, sampingRate);
		auto HubertWav = AudioPreprocess().codec(path, 16000);
		//wav
		auto info = cutWav(RawWav, threshold, minLen, static_cast<unsigned short>(frame_len), static_cast<unsigned short>(frame_shift));
		process->SetAttribute(L"value", L"0");
		process->SetAttribute(L"max", std::to_wstring(info.cutTag.size()));
		const auto LenFactor = ((double)16000 / (double)sampingRate);
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
		{
			if (info.cutTag[i - 1])
			{
				auto featureInput = FeatureInput(sampingRate, (short)curHop);
				//输入处理
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				const auto srcPos = info.cutOffset[i - 1] / 2;
				std::vector<double> source(len, 0.0);
				//const auto source = new double[len];//source
				for (unsigned long long j = 0; j < len; ++j)
					source[j] = (double)RawWav[srcPos + j] / 32768.0;

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
				catch (Ort::Exception& e1)
				{
					throw std::exception((std::string("Locate: hubert\n") + e1.what()).c_str());
				}
				auto hubertOutData = hubertOut[0].GetTensorMutableData<float>();
				auto hubertOutLen = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
				auto hubertOutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<float> hubOutData(hubertOutData, hubertOutData + hubertOutLen);

				auto alignment = getAligments(len / curHop, hubertOutShape[1]);
				auto rf0 = featureInput.transcribeF0(source.data(), (long long)len, tran);

				const long long F0Shape[2] = { 1, static_cast<long long>(len / curHop) };
				long long charData[1] = { charEmb };
				constexpr long long charShape[1] = { 1 };
				long long speedData[1] = { curPndm };

				inputTensors.clear();

				std::vector<float> initial_noise(curMelBins * F0Shape[1], 0.0);
				for (auto& it : initial_noise)
					it = normal(gen);

				long long noise_shape[4] = { 1,1,curMelBins,F0Shape[1] };

				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, hubOutData.data(), hubertOutLen, hubertOutShape.data(), 3));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, charData, 1, charShape, 1));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, rf0.data(), F0Shape[1], F0Shape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, initial_noise.data(), initial_noise.size(), noise_shape, 4));
				inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, speedData, 1, charShape, 1));
				
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
				std::vector<Ort::Value> finaOut;
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
				rf0.clear();
				alignment.clear();
				const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<float> out(finaOut[0].GetTensorData<float>(), finaOut[0].GetTensorData<float>() + shapeOut[2]);
				std::vector<int16_t> TempVecWav(shapeOut[2], 0);
				for (int bbb = 0; bbb < shapeOut[2]; bbb++) {
					TempVecWav[bbb] = static_cast<int16_t>(out[bbb] * 32768.0f);
				}
				_wavData.insert(_wavData.end(), TempVecWav.data(), TempVecWav.data() + shapeOut[2]);
			}
			else
			{
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				std::vector<int16_t> data(len, 0);
				_wavData.insert(_wavData.end(), data.data(), data.data() + len);
			}
			process->SetAttribute(L"value", std::to_wstring(i + 1));
		}
	}

	void MioTTSControl::inferSovitsT(const std::vector<std::wstring>& _Lens, const std::vector<std::vector<std::wstring>>& Params, const CutInfo& cutinfo, int charEmb)
	{
		process->SetAttribute(L"max", std::to_wstring(_Lens.size()));
		InferVits(_Lens, Params);
		GetUIResourceFromDataChunk(tModelSr);
		std::wstring path(GetCurrentFolder() + L"\\temp\\tmp.wav");
		DMResources().WriteFiles(path, tmpWav);
		tmpWav.Release();
		_wavData.clear();
		inferSovits(path, cutinfo, charEmb);
	}

	void MioTTSControl::inferSvcT(const std::vector<std::wstring>& _Lens, const std::vector<std::vector<std::wstring>>& Params, const CutInfo& cutinfo, int charEmb, const DiffusionInfo& finfo)
	{
		process->SetAttribute(L"max", std::to_wstring(_Lens.size()));
		InferVits(_Lens, Params);
		GetUIResourceFromDataChunk(tModelSr);
		std::wstring path(GetCurrentFolder() + L"\\temp\\tmp.wav");
		DMResources().WriteFiles(path, tmpWav);
		tmpWav.Release();
		_wavData.clear();
		if (charEmb < 0)
			charEmb = 0;
		if(V2)
			inferDiff(path, cutinfo, charEmb, finfo);
		else
			inferSvc(path, cutinfo, charEmb);
	}

	void MioTTSControl::inferDiff(std::wstring& path, const CutInfo& cutinfo, int charEmb, const DiffusionInfo& finfo)
	{
		int64 speedup = finfo.pndm;
		int64 step = finfo.step;
		if (step > 1000 || step < 30)
			step = 1000;
		if (speedup > step || speedup < 2)
			speedup = 2;
		if (path[0] == L'\"')
			path = path.substr(1);
		if (path[path.length() - 1] == L'\"')
			path.pop_back();
		std::mt19937 gen(_wtoi(tran_seeds->GetCurText().c_str()));
		std::normal_distribution<float> normal(0, 1);
		auto tran = static_cast<int>(cutinfo.keys);
		auto threshold = static_cast<double>(cutinfo.threshold);
		auto minLen = static_cast<unsigned long>(cutinfo.minLen);
		auto frame_len = static_cast<unsigned long>(cutinfo.frame_len);
		auto frame_shift = static_cast<unsigned long>(cutinfo.frame_shift);
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
		auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;
		//Audio
		auto RawWav = AudioPreprocess().codec(path, sampingRate);
		auto HubertWav = AudioPreprocess().codec(path, 16000);
		//wav
		auto info = cutWav(RawWav, threshold, minLen, static_cast<unsigned short>(frame_len), static_cast<unsigned short>(frame_shift));
		process->SetAttribute(L"value", L"0");
		process->SetAttribute(L"max", std::to_wstring(info.cutTag.size() * RealDiffSteps));
		const auto LenFactor = ((double)16000 / (double)sampingRate);
		size_t proc = 0;
		std::vector<int64> diffusionSteps;
		for (int64 itt = step - speedup; itt >= 0; itt -= speedup)
			diffusionSteps.push_back(itt);
		for (size_t i = 1; i < info.cutOffset.size(); ++i)
		{
			if (info.cutTag[i - 1])
			{
				auto featureInput = FeatureInput(sampingRate, (short)curHop);
				//输入处理
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				const auto srcPos = info.cutOffset[i - 1] / 2;
				std::vector<double> source(len, 0.0);
				//const auto source = new double[len];//source
				for (unsigned long long j = 0; j < len; ++j)
					source[j] = (double)RawWav[srcPos + j] / 32768.0;

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
				catch (Ort::Exception& e1)
				{
					throw std::exception((std::string("Locate: hubert\n") + e1.what()).c_str());
				}
				auto hubertOutData = hubertOut[0].GetTensorMutableData<float>();
				auto hubertOutLen = hubertOut[0].GetTensorTypeAndShapeInfo().GetElementCount();
				auto hubertOutShape = hubertOut[0].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<float> hubOutData(hubertOutData, hubertOutData + hubertOutLen);

				auto alignment = getAligments(len / curHop, hubertOutShape[1]);
				auto rf0 = featureInput.transcribeF0(source.data(), (long long)len, tran);

				const long long F0Shape[2] = { 1, static_cast<long long>(len / curHop) };
				long long charData[1] = { charEmb };
				constexpr long long charShape[1] = { 1 };

				inputTensors.clear();

				std::vector<float> initial_noise(curMelBins * F0Shape[1], 0.0);
				for (auto& it : initial_noise)
					it = normal(gen);

				long long noise_shape[4] = { 1,1,curMelBins,F0Shape[1] };

				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, hubOutData.data(), hubertOutLen, hubertOutShape.data(), 3));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, alignment.data(), F0Shape[1], F0Shape, 2));
				inputTensors.emplace_back(Ort::Value::CreateTensor<long long>(*memory_info, charData, 1, charShape, 1));
				inputTensors.emplace_back(Ort::Value::CreateTensor(*memory_info, rf0.data(), F0Shape[1], F0Shape, 2));

				std::vector<Ort::Value> encTensors;
				try {
					encTensors = encoder->Run(Ort::RunOptions{ nullptr },
						encoderInput.data(),
						inputTensors.data(),
						inputTensors.size(),
						encoderOutput.data(),
						encoderOutput.size());
				}
				catch (Ort::Exception& e1)
				{
					throw std::exception((std::string("Locate: encoder\n") + e1.what()).c_str());
				}

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
					if(noiseList.empty())
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
					}else
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
					process->SetAttribute(L"value", std::to_wstring(++proc));

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
				rf0.clear();
				alignment.clear();
				const auto shapeOut = finaOut[0].GetTensorTypeAndShapeInfo().GetShape();
				std::vector<float> out(finaOut[0].GetTensorData<float>(), finaOut[0].GetTensorData<float>() + shapeOut[2]);
				std::vector<int16_t> TempVecWav(shapeOut[2], 0);
				for (int bbb = 0; bbb < shapeOut[2]; bbb++) {
					TempVecWav[bbb] = static_cast<int16_t>(out[bbb] * 32768.0f);
				}
				_wavData.insert(_wavData.end(), TempVecWav.data(), TempVecWav.data() + shapeOut[2]);
			}
			else
			{
				const auto len = (info.cutOffset[i] - info.cutOffset[i - 1]) / 2;
				std::vector<int16_t> data(len, 0);
				_wavData.insert(_wavData.end(), data.data(), data.data() + len);
				proc += RealDiffSteps;
				process->SetAttribute(L"value", std::to_wstring(proc));
			}
		}
	}

	std::vector<double> linspace(size_t step, double start = 0.0,double end = 1.0)
	{
		const double off = (end - start) / static_cast<double>(step);
		std::vector<double> out(step + 1);
		for (size_t i = 0; i <= step; ++i)
			out[i] = static_cast<double>(i) * off + start;
		return out;
	}

	void MioTTSControl::inferDiffSinger(const DiffSingerInput& input, const CutInfo& cutinfo, int chara, const DiffusionInfo& finfo)
	{
		int64 speedup = finfo.pndm;
		int64 step = finfo.step;
		const auto RealDiffSteps = step % speedup ? step / speedup + 1 : step / speedup;
		if (step > 1000 || step < 30)
			step = 1000;
		if (speedup > step || speedup < 2)
			speedup = 2;
		size_t proc = 0;
		std::vector<int64> diffusionSteps;
		for (int64 itt = step - speedup; itt >= 0; itt -= speedup)
			diffusionSteps.push_back(itt);
		std::mt19937 gen(_wtoi(tran_seeds->GetCurText().c_str()));
		std::normal_distribution<float> normal(0, 1);
		process->SetAttribute(L"value", L"0");
		process->SetAttribute(L"max", std::to_wstring(input.inputLens.size() * RealDiffSteps));
		auto tran = static_cast<int>(cutinfo.keys);
		for (size_t i = 0; i < input.inputLens.size(); ++i)
		{
			const long long TokenShape[2] = { 1, static_cast<long long>(input.inputLens[i].size())};
			const long long F0Shape[2] = { 1, static_cast<long long>(input.f0[i].size())};
			long long charData[1] = { chara };
			constexpr long long charShape[1] = { 1 };
			auto token = input.inputLens[i];
			auto duration = input.durations[i];
			auto f0 = input.f0[i];
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

			std::vector<float> initial_noise(curMelBins * F0Shape[1], 0.0);
			for (auto& it : initial_noise)
				it = normal(gen);
			long long noise_shape[4] = { 1,1,curMelBins,F0Shape[1] };

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
				process->SetAttribute(L"value", std::to_wstring(++proc));

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
			const auto silent_length = static_cast<int64_t>(round(input.offset[i] * sampingRate)) - static_cast<int64_t>(_wavData.size());
			if (silent_length > 0)
			{
				std::vector<int16_t> iwav(silent_length, 0);
				_wavData.insert(_wavData.end(), iwav.data(), iwav.data() + iwav.size());
				_wavData.insert(_wavData.end(), TempVecWav.data(), TempVecWav.data() + shapeOut[1]);
			}else
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

	void MioTTSControl::inferDiffSinger2(const DiffSingerInput& input, const CutInfo& cutinfo, int chara, const DiffusionInfo& finfo)
	{
		int64 speedup = finfo.pndm;
		if (speedup > 500 || speedup < 2)
			speedup = 2;
		process->SetAttribute(L"value", L"0");
		process->SetAttribute(L"max", std::to_wstring(input.inputLens.size()));
		const auto tran = static_cast<int>(cutinfo.keys);
		for (size_t i = 0; i < input.inputLens.size(); ++i)
		{
			const long long TokenShape[2] = { 1, static_cast<long long>(input.inputLens[i].size()) };
			const long long F0Shape[2] = { 1, static_cast<long long>(input.f0[i].size()) };
			//long long charData[1] = { chara };
			long long speed_up[1] = { speedup };
			constexpr long long speedShape[1] = { 1 };
			auto token = input.inputLens[i];
			auto duration = input.durations[i];
			auto f0 = input.f0[i];
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
			const auto silent_length = static_cast<int64_t>(round(input.offset[i] * sampingRate)) - static_cast<int64_t>(_wavData.size());
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
			process->SetAttribute(L"value", std::to_wstring(i + 1));
		}
	}
}