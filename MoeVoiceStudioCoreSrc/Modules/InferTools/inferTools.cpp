#include "MioShirakanaTensor.hpp"
#include "inferTools.hpp"
#include "../../Lib/World/src/world/dio.h"
#include "../../Lib/World/src/world/stonemask.h"
#include "../../Lib/World/src/world/matlabfunctions.h"
#include <codecvt>
#include <locale>
#include <regex>
#include <FileSystem/DreamMoonRes.h>

std::string to_byte_string(const std::wstring& input)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(input);
}

std::wstring to_wide_string(const std::string& input)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(input);
}

Wav::Wav(const wchar_t* Path) :header(WAV_HEADER()) {
	char buf[1024];
	FILE* stream;
	_wfreopen_s(&stream, Path, L"rb", stderr);
	if (stream == nullptr) {
		throw (std::exception("File not exists"));
	}
	fread(buf, 1, HEAD_LENGTH, stream);
	int pos = 0;
	while (pos < HEAD_LENGTH) {
		if ((buf[pos] == 'R') && (buf[pos + 1] == 'I') && (buf[pos + 2] == 'F') && (buf[pos + 3] == 'F')) {
			pos += 4;
			break;
		}
		++pos;
	}
	if (pos >= HEAD_LENGTH)
		throw (std::exception("Don't order fried rice (annoyed)"));
	header.ChunkSize = *(int*)&buf[pos];
	pos += 8;
	while (pos < HEAD_LENGTH) {
		if ((buf[pos] == 'f') && (buf[pos + 1] == 'm') && (buf[pos + 2] == 't')) {
			pos += 4;
			break;
		}
		++pos;
	}
	if (pos >= HEAD_LENGTH)
		throw (std::exception("Don't order fried rice (annoyed)"));
	header.Subchunk1Size = *(int*)&buf[pos];
	pos += 4;
	header.AudioFormat = *(short*)&buf[pos];
	pos += 2;
	header.NumOfChan = *(short*)&buf[pos];
	pos += 2;
	header.SamplesPerSec = *(int*)&buf[pos];
	pos += 4;
	header.bytesPerSec = *(int*)&buf[pos];
	pos += 4;
	header.blockAlign = *(short*)&buf[pos];
	pos += 2;
	header.bitsPerSample = *(short*)&buf[pos];
	pos += 2;
	while (pos < HEAD_LENGTH) {
		if ((buf[pos] == 'd') && (buf[pos + 1] == 'a') && (buf[pos + 2] == 't') && (buf[pos + 3] == 'a')) {
			pos += 4;
			break;
		}
		++pos;
	}
	if (pos >= HEAD_LENGTH)
		throw (std::exception("Don't order fried rice (annoyed)"));
	header.Subchunk2Size = *(int*)&buf[pos];
	pos += 4;
	StartPos = pos;
	Data = new char[header.Subchunk2Size + 1];
	fseek(stream, StartPos, SEEK_SET);
	fread(Data, 1, header.Subchunk2Size, stream);
	if (stream != nullptr) {
		fclose(stream);
	}
	SData = reinterpret_cast<int16_t*>(Data);
	dataSize = header.Subchunk2Size / 2;
}

Wav::Wav(const Wav& input) :header(WAV_HEADER()) {
	Data = new char[(input.header.Subchunk2Size + 1)];
	if (Data == nullptr) { throw std::exception("OOM"); }
	memcpy(header.RIFF, input.header.RIFF, 4);
	memcpy(header.fmt, input.header.fmt, 4);
	memcpy(header.WAVE, input.header.WAVE, 4);
	memcpy(header.Subchunk2ID, input.header.Subchunk2ID, 4);
	header.ChunkSize = input.header.ChunkSize;
	header.Subchunk1Size = input.header.Subchunk1Size;
	header.AudioFormat = input.header.AudioFormat;
	header.NumOfChan = input.header.NumOfChan;
	header.SamplesPerSec = input.header.SamplesPerSec;
	header.bytesPerSec = input.header.bytesPerSec;
	header.blockAlign = input.header.blockAlign;
	header.bitsPerSample = input.header.bitsPerSample;
	header.Subchunk2Size = input.header.Subchunk2Size;
	StartPos = input.StartPos;
	memcpy(Data, input.Data, input.header.Subchunk2Size);
	SData = reinterpret_cast<int16_t*>(Data);
	dataSize = header.Subchunk2Size / 2;
}

Wav::Wav(Wav&& input) noexcept
{
	Data = input.Data;
	input.Data = nullptr;
	memcpy(header.RIFF, input.header.RIFF, 4);
	memcpy(header.fmt, input.header.fmt, 4);
	memcpy(header.WAVE, input.header.WAVE, 4);
	memcpy(header.Subchunk2ID, input.header.Subchunk2ID, 4);
	header.ChunkSize = input.header.ChunkSize;
	header.Subchunk1Size = input.header.Subchunk1Size;
	header.AudioFormat = input.header.AudioFormat;
	header.NumOfChan = input.header.NumOfChan;
	header.SamplesPerSec = input.header.SamplesPerSec;
	header.bytesPerSec = input.header.bytesPerSec;
	header.blockAlign = input.header.blockAlign;
	header.bitsPerSample = input.header.bitsPerSample;
	header.Subchunk2Size = input.header.Subchunk2Size;
	StartPos = input.StartPos;
	SData = reinterpret_cast<int16_t*>(Data);
	dataSize = header.Subchunk2Size / 2;
}

Wav& Wav::operator=(Wav&& input) noexcept
{
	destory();
	Data = input.Data;
	input.Data = nullptr;
	memcpy(header.RIFF, input.header.RIFF, 4);
	memcpy(header.fmt, input.header.fmt, 4);
	memcpy(header.WAVE, input.header.WAVE, 4);
	memcpy(header.Subchunk2ID, input.header.Subchunk2ID, 4);
	header.ChunkSize = input.header.ChunkSize;
	header.Subchunk1Size = input.header.Subchunk1Size;
	header.AudioFormat = input.header.AudioFormat;
	header.NumOfChan = input.header.NumOfChan;
	header.SamplesPerSec = input.header.SamplesPerSec;
	header.bytesPerSec = input.header.bytesPerSec;
	header.blockAlign = input.header.blockAlign;
	header.bitsPerSample = input.header.bitsPerSample;
	header.Subchunk2Size = input.header.Subchunk2Size;
	StartPos = input.StartPos;
	SData = reinterpret_cast<int16_t*>(Data);
	dataSize = header.Subchunk2Size / 2;
	return *this;
}

Wav& Wav::cat(const Wav& input)
{
	if (header.AudioFormat != 1) return *this;
	if (header.SamplesPerSec != input.header.bitsPerSample || header.NumOfChan != input.header.NumOfChan) return *this;
	char* buffer = new char[(int64_t)header.Subchunk2Size + (int64_t)input.header.Subchunk2Size + 1];
	if (buffer == nullptr)return *this;
	memcpy(buffer, Data, header.Subchunk2Size);
	memcpy(buffer + header.Subchunk2Size, input.Data, input.header.Subchunk2Size);
	header.ChunkSize += input.header.Subchunk2Size;
	header.Subchunk2Size += input.header.Subchunk2Size;
	delete[] Data;
	Data = buffer;
	SData = reinterpret_cast<int16_t*>(Data);
	dataSize = header.Subchunk2Size / 2;
	return *this;
}

double getAvg(const short* start, const short* end)
{
	const auto size = end - start + 1;
	auto avg = (double)(*start);
	for (auto i = 1; i < size; i++)
	{
		avg = avg + (abs((double)start[i]) - avg) / (double)(i + 1ull);
	}
	return avg;
}

cutResult cutWav(Wav& input, double threshold, unsigned long minLen, unsigned short frame_len, unsigned short frame_shift)
{
	const auto header = input.getHeader();
	if (header.Subchunk2Size < minLen * header.bytesPerSec)
		return { {0,header.Subchunk2Size},{true} };
	auto ptr = input.getData();
	std::vector<unsigned long long> output;
	std::vector<bool> tag;
	auto n = (header.Subchunk2Size / frame_shift) - 2 * (frame_len / frame_shift);
	unsigned long nn = 0;
	bool cutTag = true;
	output.emplace_back(0);
	while (n--)
	{
		//if (nn > minLen * header.bytesPerSec)
		if (cutTag)
		{
			const auto vol = abs(getAvg((short*)ptr, (short*)ptr + frame_len));
			if (vol < threshold)
			{
				cutTag = false;
				if (nn > minLen * header.bytesPerSec)
				{
					nn = 0;
					output.emplace_back((ptr - input.getData()) + (frame_len / 2));
				}
			}
			else
			{
				cutTag = true;
			}
		}
		else
		{
			const auto vol = abs(getAvg((short*)ptr, (short*)ptr + frame_len));
			if (vol < threshold)
			{
				cutTag = false;
			}
			else
			{
				cutTag = true;
				if (nn > minLen * header.bytesPerSec)
				{
					nn = 0;
					output.emplace_back((ptr - input.getData()) + (frame_len / 2));
				}
			}
		}
		nn += frame_shift;
		ptr += frame_shift;
	}
	output.push_back(header.Subchunk2Size);
	for (size_t i = 1; i < output.size(); i++)
	{
		tag.push_back(abs(getAvg((short*)(input.getData() + output[i - 1]), (short*)(input.getData() + output[i]))) > threshold);
	}
	return { std::move(output),std::move(tag) };
}

void F0PreProcess::compute_f0(const double* audio, int64_t len)
{
	DioOption Doption;
	InitializeDioOption(&Doption);
	Doption.f0_ceil = 800;
	Doption.frame_period = 1000.0 * hop / fs;
	f0Len = GetSamplesForDIO(fs, (int)len, Doption.frame_period);
	const auto tp = new double[f0Len];
	const auto tmpf0 = new double[f0Len];
	rf0 = new double[f0Len];
	Dio(audio, (int)len, fs, &Doption, tp, tmpf0);
	StoneMask(audio, (int)len, fs, tp, tmpf0, (int)f0Len, rf0);
	delete[] tmpf0;
	delete[] tp;
}

std::vector<double> arange(double start,double end,double step = 1.0,double div = 1.0)
{
	std::vector<double> output;
	while(start<end)
	{
		output.push_back(start / div);
		start += step;
	}
	return output;
}

void F0PreProcess::InterPf0(int64_t len)
{
	const auto xi = arange(0.0, (double)f0Len * (double)len, (double)f0Len, (double)len);
	const auto tmp = new double[xi.size() + 1];
	interp1(arange(0, (double)f0Len).data(), rf0, static_cast<int>(f0Len), xi.data(), (int)xi.size(), tmp);
	for (size_t i = 0; i < xi.size(); i++)
		if (isnan(tmp[i]))
			tmp[i] = 0.0;
	delete[] rf0;
    rf0 = nullptr;
	rf0 = tmp;
	f0Len = (int64_t)xi.size();
}

long long* F0PreProcess::f0Log()
{
	const auto tmp = new long long[f0Len];
	const auto f0_mel = new double[f0Len];
	for (long long i = 0; i < f0Len; i++)
	{
		f0_mel[i] = 1127 * log(1.0 + rf0[i] / 700.0);
		if (f0_mel[i] > 0.0)
			f0_mel[i] = (f0_mel[i] - f0_mel_min) * (f0_bin - 2.0) / (f0_mel_max - f0_mel_min) + 1.0;
		if (f0_mel[i] < 1.0)
			f0_mel[i] = 1;
		if (f0_mel[i] > f0_bin - 1)
			f0_mel[i] = f0_bin - 1;
		tmp[i] = (long long)round(f0_mel[i]);
	}
	delete[] f0_mel;
	delete[] rf0;
    rf0 = nullptr;
	return tmp;
}

std::vector<long long> F0PreProcess::GetF0AndOtherInput(const double* audio, int64_t audioLen, int64_t hubLen, int64_t tran)
{
	compute_f0(audio, audioLen);
	for (int64_t i = 0; i < f0Len; ++i)
	{
		rf0[i] = rf0[i] * pow(2.0, static_cast<double>(tran) / 12.0);
		if (rf0[i] < 0.001)
			rf0[i] = NAN;
	}
	InterPf0(hubLen);
	const auto O0f = f0Log();
	std::vector<long long> Of0(O0f, O0f + f0Len);
    delete[] O0f;
	return Of0;
}

std::vector<float> F0PreProcess::GetF0AndOtherInput3(const double* audio, int64_t audioLen, int64_t hubLen, int64_t tran)
{
	compute_f0(audio, audioLen);
	for (int64_t i = 0; i < f0Len; ++i)
	{
		rf0[i] = rf0[i] * pow(2.0, static_cast<double>(tran) / 12.0);
		if (rf0[i] < 0.001)
			rf0[i] = NAN;
	}
	InterPf0(hubLen);
	std::vector<float> Of0(hubLen, 0.0);
	for (int64_t i = 0; i < hubLen; ++i)
		Of0[i] = static_cast<float>(rf0[i]);
	delete[] rf0;
    rf0 = nullptr;
	return Of0;
}

F0PreProcess::V4Return F0PreProcess::GetF0AndOtherInput4(const double* audio, int64_t audioLen, int64_t tran)
{
    const int64_t specLen = audioLen / hop;
    compute_f0(audio, audioLen);
    for (int64_t i = 0; i < f0Len; ++i)
    {
        rf0[i] = rf0[i] * pow(2.0, static_cast<double>(tran) / 12.0);
        if (rf0[i] < 0.001)
            rf0[i] = NAN;
    }
    InterPf0(specLen);

    std::vector<float> Of0(specLen, 0.0);
    std::vector<float> ruv(specLen, 1.0);

    for (int64_t i = 0; i < specLen; ++i)
    {
        Of0[i] = static_cast<float>(rf0[i]);
        if (rf0[i] < 0.001)
            ruv[i] = 0.0;
    }

    double last_value = 0.0;
    for (int64_t i = 0; i < specLen; ++i)
    {
        if(rf0[i] <= 0.0)
        {
            int64_t j = i + 1;
            for (; j < specLen; ++j)
            {
	            if (rf0[j] > 0.0)
	            	break;
            }
            if (j < specLen - 1)
            {
                if (last_value > 0.0)
                {
                    const auto step = (rf0[j] - rf0[i - 1]) / double(j - i);
                    for (int64_t k = i; k < j; ++k)
                        Of0[k] = float(rf0[i - 1] + step * double(k - i + 1));
                }
                else
                    for (int64_t k = i; k < j; ++k)
                        Of0[k] = float(rf0[j]);
                i = j;
            }
            else
            {
	            for (int64_t k = i; k < specLen; ++k)
	            	Of0[k] = float(last_value);
                i = specLen;
            }
        }
        else
        {
            Of0[i] = float(rf0[i - 1]);
            last_value = rf0[i];
        }
    }

    delete[] rf0;
    rf0 = nullptr;
    return { std::move(Of0),std::move(ruv) };
}

std::vector<float> F0PreProcess::GetF0AndOtherInputF0(const double* audio, int64_t audioLen, int64_t tran)
{
	compute_f0(audio, audioLen);
	for (int64_t i = 0; i < f0Len; ++i)
	{
		rf0[i] = log2(rf0[i] * pow(2.0, static_cast<double>(tran) / 12.0));
		if (rf0[i] < 0.001)
			rf0[i] = NAN;
	}
	const int64_t specLen = audioLen / hop;
	InterPf0(specLen);

    std::vector<float> Of0(specLen, 0.0);

    double last_value = 0.0;
    for (int64_t i = 0; i < specLen; ++i)
    {
        if (rf0[i] <= 0.0)
        {
            int64_t j = i + 1;
            for (; j < specLen; ++j)
            {
                if (rf0[j] > 0.0)
                    break;
            }
            if (j < specLen - 1)
            {
                if (last_value > 0.0)
                {
                    const auto step = (rf0[j] - rf0[i - 1]) / double(j - i);
                    for (int64_t k = i; k < j; ++k)
                        Of0[k] = float(rf0[i - 1] + step * double(k - i + 1));
                }
                else
                    for (int64_t k = i; k < j; ++k)
                        Of0[k] = float(rf0[j]);
                i = j;
            }
            else
            {
                for (int64_t k = i; k < specLen; ++k)
                    Of0[k] = float(last_value);
                i = specLen;
            }
        }
        else
        {
            Of0[i] = float(rf0[i - 1]);
            last_value = rf0[i];
        }
    }
    delete[] rf0;
    rf0 = nullptr;
	return Of0;
}

std::vector<std::vector<bool>> generatePath(float* duration, size_t durationSize, size_t maskSize)
{
	for (size_t i = 1; i < durationSize; ++i)
		duration[i] = duration[i - 1] + duration[i];
	std::vector<std::vector<bool>> path(durationSize,std::vector<bool>(maskSize, false));
	//const auto path = new float[maskSize * durationSize];
	/*
	for (size_t i = 0; i < maskSize; ++i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] = (j < (size_t)duration[i] ? 1.0f : 0.0f);
	for (size_t i = maskSize - 1; i > 0ull; --i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] -= path[i-1][j];
	 */
	auto dur = (size_t)duration[0];
	for (size_t j = 0; j < dur; ++j)
		path[j][0] = true;
	/*
	for (size_t i = maskSize - 1; i > 0ull; --i)
		for (size_t j = 0; j < durationSize; ++j)
			path[i][j] = (j < (size_t)duration[i] && j >= (size_t)duration[i - 1]);
	std::vector<std::vector<float>> tpath(durationSize, std::vector<float>(maskSize));
	for (size_t i = 0; i < maskSize; ++i)
		for (size_t j = 0; j < durationSize; ++j)
			tpath[j][i] = path[i][j];
	 */
	for (size_t j = maskSize - 1; j > 0ull; --j)
	{
		dur = (size_t)duration[j];
		for (auto i = (size_t)duration[j - 1]; i < dur; ++i)
			path[i][j] = true;
	}
	return path;
}

std::vector<long long> getAligments(size_t specLen, size_t hubertLen)
{
	std::vector<long long> mel2ph(specLen + 1, 0);

	size_t startFrame = 0;
	const double ph_durs = static_cast<double>(specLen) / static_cast<double>(hubertLen);
	for (size_t iph = 0; iph < hubertLen; ++iph)
	{
		const auto endFrame = static_cast<size_t>(round(static_cast<double>(iph) * ph_durs + ph_durs));
		for (auto j = startFrame; j < endFrame + 1; ++j)
			mel2ph[j] = static_cast<long long>(iph) + 1;
		startFrame = endFrame + 1;
	}

	return mel2ph;
}

namespace tranTokens
{
    std::wregex numReg(L"[0-9\\.]+");
    std::wregex tokenReg(L"[A-Za-z]+");

    std::wstring ChineseToJapanese(std::wstring text)
    {
        std::wsmatch amatch_results;
        std::vector<std::wstring> Tokens;
        uint64_t PHSEQOff = text.find(L"<<<<DURATION>>>>");
        std::wstring phstr = text.substr(PHSEQOff + 16);
        text = text.substr(0, PHSEQOff + 1);
        while (std::regex_search(text, amatch_results, tokenReg))
        {
            Tokens.push_back(amatch_results[0].str());
            text = amatch_results.suffix();
        }
        std::vector<double> ph_dur;
        ph_dur.reserve(Tokens.size());

        while (std::regex_search(phstr, amatch_results, numReg))
        {
            ph_dur.push_back(_wtof(amatch_results[0].str().c_str()));
            phstr = amatch_results.suffix();
        }
        std::wstring ph_dur_str;
        std::wstring ph_seq_str;
        for (size_t i = 0; i < Tokens.size(); ++i)
        {
            if (Tokens[i] == L"ai")
            {
                ph_seq_str += L"e ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"E")
            {
                ph_seq_str += L"e ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"EN" || Tokens[i] == L"En")
            {
                ph_seq_str += L"e N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"an")
            {
                ph_seq_str += L"a N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"ang")
            {
                ph_seq_str += L"a N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"ao")
            {
                ph_seq_str += L"a o ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"c")
            {
                ph_seq_str += L"ts ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"en")
            {
                ph_seq_str += L"e N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"eng")
            {
                ph_seq_str += L"e N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"ei")
            {
                ph_seq_str += L"e i ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"er")
            {
                ph_seq_str += L"e ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"i0")
            {
                ph_seq_str += L"i ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"ia")
            {
                ph_seq_str += L"i a ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"ian")
            {
                ph_seq_str += L"i a N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"iang")
            {
                ph_seq_str += L"i a N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"iao")
            {
                ph_seq_str += L"i a o ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"ie")
            {
                ph_seq_str += L"i e ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"in")
            {
                ph_seq_str += L"i N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"ing")
            {
                ph_seq_str += L"i N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"iong")
            {
                ph_seq_str += L"i o N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"iu")
            {
                ph_seq_str += L"i u ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"ir")
            {
                ph_seq_str += L"i ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"l")
            {
                ph_seq_str += L"r ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"ong")
            {
                ph_seq_str += L"o N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"ou")
            {
                ph_seq_str += L"o u ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"q")
            {
                ph_seq_str += L"ch ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"ua")
            {
                ph_seq_str += L"u a ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"uai")
            {
                ph_seq_str += L"u a i ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"uan")
            {
                ph_seq_str += L"u a N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"uang")
            {
                ph_seq_str += L"u a N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"ui")
            {
                ph_seq_str += L"u i ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"un")
            {
                ph_seq_str += L"u N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"uo")
            {
                ph_seq_str += L"u o ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 2) + L' ';
            }
            else if (Tokens[i] == L"v")
            {
                ph_seq_str += L"i y u ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 3) + L' ';
            }
            else if (Tokens[i] == L"van")
            {
                ph_seq_str += L"i y u a N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 5) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 5) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 5) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 5) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 5) + L' ';
            }
            else if (Tokens[i] == L"ve")
            {
                ph_seq_str += L"i y u e ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
            }
            else if (Tokens[i] == L"vn")
            {
                ph_seq_str += L"i y u N ";
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i] / 4) + L' ';
            }
            else if (Tokens[i] == L"x")
            {
                ph_seq_str += L"sh ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"zh")
            {
                ph_seq_str += L"z ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else
            {
                ph_seq_str += Tokens[i] + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
        }
        return ph_seq_str + L"<<<<DURATION>>>>" + ph_dur_str;
    }

    std::wstring JapaneseToChinese(std::wstring text)
    {
        std::wsmatch amatch_results;
        std::vector<std::wstring> Tokens;
        uint64_t PHSEQOff = text.find(L"<<<<DURATION>>>>");
        std::wstring phstr = text.substr(PHSEQOff + 16);
        text = text.substr(0, PHSEQOff);
        text = std::regex_replace(text, std::wregex(L"([kthgdbpnmr])y u"), L"$1 v");
        text = std::regex_replace(text, std::wregex(L"([kthgdbpnmr])y ([ae])"), L"$1 i$2");
        text = std::regex_replace(text, std::wregex(L"([kthgdbpnmr])y ([io])"), L"$1 $2");
        while (std::regex_search(text, amatch_results, tokenReg))
        {
            Tokens.push_back(amatch_results[0].str());
            text = amatch_results.suffix();
        }
        std::vector<double> ph_dur;
        ph_dur.reserve(Tokens.size());
        while (std::regex_search(phstr, amatch_results, numReg))
        {
            ph_dur.push_back(_wtof(amatch_results[0].str().c_str()));
            phstr = amatch_results.suffix();
        }
        std::wstring ph_dur_str;
        std::wstring ph_seq_str;
        for (size_t i = 0; i < Tokens.size(); ++i)
        {
            if (i < Tokens.size() - 1 && Tokens[i + 1] == L"N")
            {
                if (Tokens[i] == L"e")
                    ph_seq_str += L"an ";
                else if (Tokens[i] == L"ie")
                    ph_seq_str += L"en ";
                else if (Tokens[i] == L"o")
                    ph_seq_str += L"ong ";
                else
                    ph_seq_str += Tokens[i] + L"n ";
                ph_dur_str += std::to_wstring(ph_dur[i] + ph_dur[i + 1]) + L' ';
                ++i;
            }
            else if (Tokens[i] == L"I")
            {
                ph_seq_str += L"i ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"U")
            {
                ph_seq_str += L"u ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"br")
            {
                continue;
            }
            else if (Tokens[i] == L"cl")
            {
                continue;
            }
            else if (Tokens[i] == L"U")
            {
                ph_seq_str += L"u ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"v")
            {
                ph_seq_str += L"w ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i] == L"e")
            {
                ph_seq_str += L"ai ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else if (Tokens[i].length() == 2)
            {
                if (Tokens[i] == L"ts")
                {
                    ph_seq_str += L"c ";
                    ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
                }
                else if (Tokens[i] == L"sh" && Tokens[i + 1] == L"i")
                {
                    ph_seq_str += L"x ";
                    ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
                }
                else if (Tokens[i] == L"ch" && Tokens[i + 1] == L"i")
                {
                    ph_seq_str += L"q ";
                    ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
                }
                else
                {
                    ph_seq_str += Tokens[i] + L' ';
                    ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
                }
            }
            else if (Tokens[i] == L"r")
            {
                ph_seq_str += L"l ";
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
            else
            {
                ph_seq_str += Tokens[i] + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
        }
        /*
        Tokens.clear();
        text = ph_seq_str;
        while (std::regex_search(text, amatch_results, tokenReg))
        {
            Tokens.push_back(amatch_results[0]);
            text = amatch_results.suffix();
        }
        ph_dur.clear();
        ph_dur.reserve(Tokens.size());
        phstr = ph_dur_str;
        while (std::regex_search(phstr, amatch_results, numReg))
        {
            ph_dur.push_back(_wtof(amatch_results[0].str().c_str()));
            phstr = amatch_results.suffix();
        }
        if (ph_dur.size() != Tokens.size())
            throw std::exception("error at tran func");
        ph_seq_str.clear();
        ph_dur_str.clear();
        for (size_t i = 0; i < Tokens.size(); ++i)
        {
            if (i < Tokens.size() - 1)
            {
                if (Tokens[i] == Tokens[i + 1])
                {
                    ph_seq_str += Tokens[i] + L' ';
                    ph_dur_str += std::to_wstring(ph_dur[i] + ph_dur[i + 1]) + L' ';
                    ++i;
                }
                else if (Tokens[i].length() == 2 && Tokens[i][1] == Tokens[i + 1][0])
                {
                    ph_seq_str += Tokens[i] + L' ';
                    ph_dur_str += std::to_wstring(ph_dur[i] + ph_dur[i + 1]) + L' ';
                    ++i;
                }
                else if (Tokens[i] == L"i" && (Tokens[i + 1] == L"a" || Tokens[i + 1] == L"e" || Tokens[i + 1] == L"u"))
                {
                    ph_seq_str += Tokens[i] + L' ';
                    ph_dur_str += std::to_wstring(ph_dur[i] + ph_dur[i + 1]) + L' ';
                    ++i;
                }
                else
                {
                    ph_seq_str += Tokens[i] + L' ';
                    ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
                }
            }
            else
            {
                ph_seq_str += Tokens[i] + L' ';
                ph_dur_str += std::to_wstring(ph_dur[i]) + L' ';
            }
        }
         */
        return ph_seq_str + L"<<<<DURATION>>>>" + ph_dur_str;
    }
}
