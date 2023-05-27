#ifndef InferTool
#define InferTool
#include <Mui_Base.h>
#include <fstream>
#pragma comment(lib,"Lib/World/World.lib")

constexpr int HEAD_LENGTH = 1024;

struct cutResult
{
	std::vector<unsigned long long>	cutOffset;
	std::vector<bool> cutTag;
	cutResult(std::vector<unsigned long long>&& O, std::vector<bool>&& T) :cutOffset(O), cutTag(T) {}
};

class Wav {
public:

	struct WAV_HEADER {
		char             RIFF[4] = { 'R','I','F','F' };              //RIFF��ʶ
		unsigned long    ChunkSize;                                  //�ļ���С-8
		char             WAVE[4] = { 'W','A','V','E' };              //WAVE��
		char             fmt[4] = { 'f','m','t',' ' };               //fmt��
		unsigned long    Subchunk1Size;                              //fmt���С
		unsigned short   AudioFormat;                                //�����ʽ
		unsigned short   NumOfChan;                                  //������
		unsigned long    SamplesPerSec;                              //������
		unsigned long    bytesPerSec;                                //ÿ�����ֽ���
		unsigned short   blockAlign;                                 //�������ֽ�
		unsigned short   bitsPerSample;                              //������λ��
		char             Subchunk2ID[4] = { 'd','a','t','a' };       //���ݿ�
		unsigned long    Subchunk2Size;                              //���ݿ��С
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
	bool isEmpty() const { return this->header.Subchunk2Size == 0; }
	const char* getData() const { return Data; }
	char* getData() { return Data; }
	WAV_HEADER getHeader() const { return header; }
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
	iterator begin() const
	{
		return reinterpret_cast<int16_t*>(Data);
	}
	iterator end() const
	{
		return reinterpret_cast<int16_t*>(Data + header.Subchunk2Size);
	}
	Mui::UIResource getRes() const
	{
		Mui::UIResource out;
		out.size = header.ChunkSize + 8;
		out.data = new Mui::_m_byte[out.size + 44];
		int64_t pos = 0;
		memcpy(out.data + pos, header.RIFF, 4);
		pos += 4;
		memcpy(out.data + pos, &header.ChunkSize, 4);
		pos += 4;
		memcpy(out.data + pos, header.WAVE, 4);
		pos += 4;
		memcpy(out.data + pos, header.fmt, 4);
		pos += 4;
		memcpy(out.data + pos, &header.Subchunk1Size, 4);
		pos += 4;
		memcpy(out.data + pos, &header.AudioFormat, 2);
		pos += 2;
		memcpy(out.data + pos, &header.NumOfChan, 2);
		pos += 2;
		memcpy(out.data + pos, &header.SamplesPerSec, 4);
		pos += 4;
		memcpy(out.data + pos, &header.bytesPerSec, 4);
		pos += 4;
		memcpy(out.data + pos, &header.blockAlign, 2);
		pos += 2;
		memcpy(out.data + pos, &header.bitsPerSample, 2);
		pos += 2;
		memcpy(out.data + pos, header.Subchunk2ID, 4);
		pos += 4;
		memcpy(out.data + pos, &header.Subchunk2Size, 4);
		pos += 4;
		memcpy(out.data + pos, Data, header.Subchunk2Size);
		return out;
	}
	int64_t getDataLen()const
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
private:
	WAV_HEADER header;
	char* Data;
	int16_t* SData;
	size_t dataSize;
	int StartPos;
};

cutResult cutWav(Wav& input, double threshold = 30, unsigned long minLen = 5, unsigned short frame_len = 4 * 1024, unsigned short frame_shift = 512);

class F0PreProcess
{
public:
	struct SvcReturn
	{
		std::vector<float> f0;
		std::vector<long long> melph;
		SvcReturn(std::vector<float>&& rF0, std::vector<long long>&& melp) :f0(rF0), melph(melp) {}
	};
	struct V4Return
	{
		std::vector<float> f0;
		std::vector<float> uv;
		V4Return() = default;
		V4Return(std::vector<float>&& rF0, std::vector<float>&& ruv) :f0(rF0), uv(ruv) {}
	};
	struct RVCR
	{
		std::vector<float> f0f;
		std::vector<long long> f0;
		RVCR(std::vector<float>&& rF0, std::vector<long long>&& melp) :f0f(rF0), f0(melp) {}
	};
	int fs;
	short hop;
	const int f0_bin = 256;
	const double f0_max = 1100.0;
	const double f0_min = 50.0;
	const double f0_mel_min = 1127.0 * log(1.0 + f0_min / 700.0);
	const double f0_mel_max = 1127.0 * log(1.0 + f0_max / 700.0);
	F0PreProcess(int sr = 16000, short h = 160) :fs(sr), hop(h) {}
	~F0PreProcess()
	{
		delete[] rf0;
		rf0 = nullptr;
	}
	void compute_f0(const double* audio, int64_t len);
	void InterPf0(int64_t len);
	long long* f0Log();
	std::vector<long long> GetF0AndOtherInput(const double* audio, int64_t audioLen, int64_t hubLen, int64_t tran);
	RVCR GetF0AndOtherInputR(const double* audio, int64_t audioLen, int64_t hubLen, int64_t tran);
	std::vector<float> GetF0AndOtherInput3(const double* audio, int64_t audioLen, int64_t hubLen, int64_t tran);
	V4Return GetF0AndOtherInput4(const double* audio, int64_t audioLen, int64_t tran);
	std::vector<float> GetF0AndOtherInputF0(const double* audio, int64_t audioLen, int64_t tran);
	std::vector<float> GetOrgF0(const double* audio, int64_t audioLen, int64_t hubLen, int64_t tran);
	int64_t getLen()const { return f0Len; }
private:
	double* rf0 = nullptr;
	int64_t f0Len = 0;
};

std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0);

std::vector<std::vector<bool>> generatePath(float* duration, size_t durationSize, size_t maskSize);

std::vector<long long> getAligments(size_t specLen, size_t hubertLen);

namespace tranTokens
{
	std::wstring ChineseToJapanese(std::wstring);
	std::wstring JapaneseToChinese(std::wstring);
}

std::vector<float> mean_filter(const std::vector<float>& vec, size_t window_size);

template<typename T>
double getAvg(const T* start, const T* end)
{
	const auto size = end - start + 1;
	auto avg = (double)(*start);
	for (auto i = 1; i < size; i++)
		avg = avg + (abs((double)start[i]) - avg) / (double)(i + 1ull);
	return avg;
}
#endif