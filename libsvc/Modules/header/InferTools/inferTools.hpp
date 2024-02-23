/**
 * FileName: InferTools.hpp
 * Note: MoeVoiceStudioCore 推理工具的定义
 *
 * Copyright (C) 2022-2023 NaruseMioShirakana (shirakanamio@foxmail.com)
 *
 * This file is part of MoeVoiceStudioCore library.
 * MoeVoiceStudioCore library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License as published by the Free Software Foundation, either version 3
 * of the License, or any later version.
 *
 * MoeVoiceStudioCore library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
 *
 * date: 2022-10-17 Create
*/

#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <functional>
#include <matlabfunctions.h>
#include <filesystem>
#define MOEVSINFERTOOLSHEADER namespace InferTools {
#define MOEVSINFERTOOLSEND }

#define LibDLVoiceCodecThrow(message) { \
	const std::string LibDlCodecThrowMessage = message;\
	const std::string LibDlCodecThrowMessagePrefix = std::string("[In \"") + std::filesystem::path(__FILE__).filename().string() + "\" Line " + std::to_string(__LINE__) + "] "; \
	if(LibDlCodecThrowMessage.substr(0, LibDlCodecThrowMessagePrefix.length()) != LibDlCodecThrowMessagePrefix) \
		throw std::exception((LibDlCodecThrowMessagePrefix + LibDlCodecThrowMessage).c_str()); \
	else \
		throw std::exception(LibDlCodecThrowMessage.c_str()); \
}

MOEVSINFERTOOLSHEADER
struct SlicerSettings
{
	int32_t SamplingRate = 48000;
	double Threshold = 30.;
	double MinLength = 3.;
	int32_t WindowLength = 2048;
	int32_t HopSize = 512;
};
class Wav {
public:
	static constexpr int HEAD_LENGTH = 1024;
	struct WAV_HEADER {
		char             RIFF[4] = { 'R','I','F','F' };
		unsigned long    ChunkSize;
		char             WAVE[4] = { 'W','A','V','E' };
		char             fmt[4] = { 'f','m','t',' ' };
		unsigned long    Subchunk1Size;
		unsigned short   AudioFormat;
		unsigned short   NumOfChan;
		unsigned long    SamplesPerSec;
		unsigned long    bytesPerSec;
		unsigned short   blockAlign;
		unsigned short   bitsPerSample;
		char             Subchunk2ID[4] = { 'd','a','t','a' };
		unsigned long    Subchunk2Size;
		WAV_HEADER(unsigned long cs = 36, unsigned long sc1s = 16, unsigned short af = 1, unsigned short nc = 1, unsigned long sr = 22050, unsigned long bps = 44100, unsigned short ba = 2, unsigned short bips = 16, unsigned long sc2s = 0) :ChunkSize(cs), Subchunk1Size(sc1s), AudioFormat(af), NumOfChan(nc), SamplesPerSec(sr), bytesPerSec(bps), blockAlign(ba), bitsPerSample(bips), Subchunk2Size(sc2s) {}
	};
	using iterator = int16_t*;
	Wav(unsigned long cs = 36, unsigned long sc1s = 16, unsigned short af = 1, unsigned short nc = 1, unsigned long sr = 22050, unsigned long bps = 44100, unsigned short ba = 2, unsigned short bips = 16, unsigned long sc2s = 0) :header({
			cs,
			sc1s,
			af,
			nc,
			sr,
			bps,
			ba,
			bips,
			sc2s
		}), Data(nullptr), StartPos(44) {
		dataSize = 0;
		SData = nullptr;
	}
	Wav(unsigned long sr, unsigned long length, const void* data) :header({
			36,
			16,
			1,
			1,
			sr,
			sr * 2,
			2,
			16,
			length
		}), Data(new char[length + 1]), StartPos(44)
	{
		header.ChunkSize = 36 + length;
		memcpy(Data, data, length);
		SData = reinterpret_cast<int16_t*>(Data);
		dataSize = length / 2;
	}
	Wav(const wchar_t* Path);
	Wav(const Wav& input);
	Wav(Wav&& input) noexcept;
	Wav& operator=(const Wav& input) = delete;
	Wav& operator=(Wav&& input) noexcept;
	~Wav() { destory(); }
	Wav& cat(const Wav& input);
	[[nodiscard]] bool isEmpty() const { return this->header.Subchunk2Size == 0; }
	[[nodiscard]] const char* getData() const { return Data; }
	char* getData() { return Data; }
	[[nodiscard]] WAV_HEADER getHeader() const { return header; }
	WAV_HEADER& Header() { return header; }
	void destory() const { delete[] Data; }
	void changeData(const void* indata, long length, int sr)
	{
		delete[] Data;
		Data = new char[length];
		memcpy(Data, indata, length);
		header.ChunkSize = 36 + length;
		header.Subchunk2Size = length;
		header.SamplesPerSec = sr;
		header.bytesPerSec = 2 * sr;
	}
	int16_t& operator[](const size_t index) const
	{
		if (index < dataSize)
			return *(SData + index);
		return *(SData + dataSize - 1);
	}
	[[nodiscard]] iterator begin() const
	{
		return reinterpret_cast<int16_t*>(Data);
	}
	[[nodiscard]] iterator end() const
	{
		return reinterpret_cast<int16_t*>(Data + header.Subchunk2Size);
	}
	[[nodiscard]] int64_t getDataLen()const
	{
		return static_cast<int64_t>(dataSize);
	}
	void Writef(const std::wstring& filepath) const
	{
		FILE* FOut = nullptr;
		_wfopen_s(&FOut, filepath.c_str(), L"wb");
		if (!FOut)
			return;
		fwrite(&header, 1, sizeof(header), FOut);
		fwrite(Data, 1, dataSize * 2, FOut);
		fclose(FOut);
		FOut = nullptr;
	}

	static void WritePCMData(int samplingrate, int channel, const std::vector<int16_t>& PCMDATA, const std::wstring& filepath)
	{
		const WAV_HEADER TmpHeader(long(36 + 2 * PCMDATA.size()), 16, 1, short(channel), samplingrate, samplingrate * 2 * channel, short(2 * channel), short(16), long(2 * PCMDATA.size()));
		FILE* FOut = nullptr;
		_wfopen_s(&FOut, filepath.c_str(), L"wb");
		if (!FOut) return;
		fwrite(&TmpHeader, 1, sizeof(TmpHeader), FOut);
		fwrite(PCMDATA.data(), 1, PCMDATA.size() * 2, FOut);
		fclose(FOut);
		FOut = nullptr;
	}

private:
	WAV_HEADER header;
	char* Data;
	int16_t* SData;
	size_t dataSize;
	int StartPos;
};

/**
 * \brief 切片机切片（获取切片位置）
 * \param input PCM数据（SignedInt16）
 * \param _slicer 切片机设置
 * \return 切片位置
 */
LibSvcApi std::vector<size_t> SliceAudio(const std::vector<int16_t>& input, const SlicerSettings& _slicer);

/**
 * \brief 均值滤波器
 * \param vec 待滤波的数据
 * \param window_size 窗口大小
 * \return 滤波后数据
 */
LibSvcApi std::vector<float> mean_filter(const std::vector<float>& vec, size_t window_size);

template<typename T>
double getAvg(const T* start, const T* end)
{
	const auto size = end - start + 1;
	auto avg = (double)(*start);
	for (auto i = 1; i < size; i++)
		avg = avg + (abs((double)start[i]) - avg) / (double)(i + 1ull);
	return avg;
}

LibSvcApi std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0);

/**
 * \brief 重采样（插值）
 * \tparam TOut 输出类型 
 * \tparam TIn 输入类型
 * \param _Data 输入数据
 * \param src 输入采样率
 * \param dst 输出采样率
 * \param n_Div 给输出的数据统一除以这个数
 * \return 输出数据
 */
template<typename TOut, typename TIn>
static std::vector<TOut> InterpResample(const std::vector<TIn>& _Data, long src, long dst, TOut n_Div = TOut(1))
{
	if (src != dst)
	{
		const double intstep = double(src) / double(dst);
		const auto xi = InferTools::arange(0, double(_Data.size()), intstep);
		auto x0 = InferTools::arange(0, double(_Data.size()));
		while (x0.size() < _Data.size())
			x0.emplace_back(x0[x0.size() - 1] + 1.0);
		while (x0.size() > _Data.size())
			x0.pop_back();

		std::vector<double> y0(_Data.size());
		for (size_t i = 0; i < _Data.size(); ++i)
			y0[i] = double(_Data[i]) / double(n_Div);

		std::vector<double> yi(xi.size());
		interp1(x0.data(), y0.data(), long(x0.size()), xi.data(), long(xi.size()), yi.data());

		std::vector<TOut> out(xi.size());
		for (size_t i = 0; i < yi.size(); ++i)
			out[i] = TOut(yi[i]);
		return out;
	}
	std::vector<TOut> out(_Data.size());
	for (size_t i = 0; i < _Data.size(); ++i)
		out[i] = TOut(_Data[i]) / n_Div;
	return out;
}

/**
 * \brief 重采样（插值）
 * \tparam T 数据类型
 * \param Data 输入数据
 * \param src 输入采样率
 * \param dst 输出采样率
 * \return 输出数据
 */
template<typename T>
static std::vector<T> InterpFunc(const std::vector<T>& Data, long src, long dst)
{
	if (src != dst)
	{
		const double intstep = double(src) / double(dst);
		auto xi = InferTools::arange(0, double(Data.size()), intstep);
		while (xi.size() < size_t(dst))
			xi.emplace_back(xi[xi.size() - 1] + 1.0);
		while (xi.size() > size_t(dst))
			xi.pop_back();
		auto x0 = InferTools::arange(0, double(Data.size()));
		while (x0.size() < Data.size())
			x0.emplace_back(x0[x0.size() - 1] + 1.0);
		while (x0.size() > Data.size())
			x0.pop_back();
		std::vector<double> y0(Data.size());
		for (size_t i = 0; i < Data.size(); ++i)
			y0[i] = Data[i] <= T(0.0001) ? NAN : double(Data[i]);
		std::vector<double> yi(xi.size());
		interp1(x0.data(), y0.data(), long(x0.size()), xi.data(), long(xi.size()), yi.data());
		std::vector<T> out(xi.size());
		for (size_t i = 0; i < yi.size(); ++i)
			out[i] = isnan(yi[i]) ? T(0.0) : T(yi[i]);
		return out;
	}
	return Data;
}
#ifdef MoeVoiceStudioAvxAcc
class FloatTensorWrapper
{
public:
	FloatTensorWrapper() = delete;
	~FloatTensorWrapper() { _data_ptr = nullptr; }
	FloatTensorWrapper(float* const data_p, size_t _size) : _data_ptr(data_p), _data_size(_size) {}
	FloatTensorWrapper(const FloatTensorWrapper& _copy) = delete;
	FloatTensorWrapper& operator=(const FloatTensorWrapper&) = delete;
	FloatTensorWrapper(FloatTensorWrapper&& _move) noexcept:_data_ptr(_move._data_ptr), _data_size(_move._data_size) {}
	FloatTensorWrapper& operator=(FloatTensorWrapper&& _move) noexcept
	{
		_data_ptr = _move._data_ptr;
		_data_size = _move._data_size;
		return *this;
	}
	template<typename T>
	static const T& Min(const T& a, const T& b) { return (a > b) ? b : a; }
	float& operator[](size_t index) const { return *(_data_ptr + Min(index, _data_size)); }
	FloatTensorWrapper& operator+=(const FloatTensorWrapper& _right);
	FloatTensorWrapper& operator-=(const FloatTensorWrapper& _right);
	FloatTensorWrapper& operator*=(const FloatTensorWrapper& _right);
	FloatTensorWrapper& operator/=(const FloatTensorWrapper& _right);
	FloatTensorWrapper& operator+=(float _right);
	FloatTensorWrapper& operator-=(float _right);
	FloatTensorWrapper& operator*=(float _right);
	FloatTensorWrapper& operator/=(float _right);
private:
	float* _data_ptr = nullptr;
	size_t _data_size = 0;
};
#endif
MOEVSINFERTOOLSEND