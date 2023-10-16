#ifdef MOEVSONNX
#include <deque>
#include <mutex>
#include <iostream>
#include "Modules/Modules.hpp"
#include "Modules/AvCodec/AvCodeResample.h"

#include <windows.h>
#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib") 

#ifdef _IOSTREAM_
std::ostream& operator<<(std::ostream& stream, const std::wstring& str)
{
	return stream << to_byte_string(str);
}
#include <vector>
template<typename T>
std::ostream& operator<<(std::ostream& stream, std::vector<T>& vec)
{
	stream << "[ ";
	for (size_t i = 0; i < vec.size(); ++i)
	{
		stream << vec[i];
		if (i != vec.size() - 1)
			stream << ", ";
	}
	stream << " ]";
	return stream;
}
#endif
#ifdef _VECTOR_
template <typename T>
std::vector<T>& operator-=(std::vector<T>& left, const std::vector<T>& right)
{
	for (size_t i = 0; i < left.size() && i < right.size(); ++i)
		left[i] -= right[i];
	return left;
}
#endif

namespace RtInferenceSpace
{
	class MRecorder
	{
	public:
		MRecorder() = default;
		~MRecorder()
		{
			if (!hWaveIn)
				return;
			Stop();
			waveInClose(hWaveIn);
		}
		void initRecorder(DWORD SamplingRate = 44100)
		{
			waveform.nSamplesPerSec = SamplingRate;
			waveform.wBitsPerSample = 16;
			waveform.nChannels = 1;
			waveform.cbSize = 0;
			waveform.wFormatTag = WAVE_FORMAT_PCM;
			waveform.nBlockAlign = (waveform.wBitsPerSample * waveform.nChannels) / 8;
			waveform.nAvgBytesPerSec = waveform.nBlockAlign * waveform.nSamplesPerSec;
			SamplingRateSrc = SamplingRate;
			WaitEvent = CreateEvent(nullptr, 0, 0, nullptr);
			waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD_PTR)WaitEvent, 0L, CALLBACK_EVENT);
		}

		void setStreamBufferSize(double time)
		{
			Stop();
			StreamSize = size_t(time * SamplingRateSrc);
			timems = DWORD(time * 1000);
			timems -= 50;
			if (timems < 50) timems = 50;
			pcmVector = std::vector<int16_t>(StreamSize * 2);
			whdri.lpData = (LPSTR)pcmVector.data();
			whdri.dwBufferLength = DWORD(StreamSize * 2);
			whdri.dwBytesRecorded = 0;
			whdri.dwUser = 0;
			whdri.dwFlags = 0;
			whdri.dwLoops = 1;
		}

		[[nodiscard]] size_t GetFrameSize() const
		{
			return StreamSize;
		}

		void Start()
		{
			if (isBegin)
				return;
			isBegin = true;
			std::thread RecoderThread([&]()
				{
					while(isBegin)
					{
						whdri.lpData = (LPSTR)pcmVector.data();
						whdri.dwBufferLength = DWORD(StreamSize * 2);
						whdri.dwBytesRecorded = 0;
						whdri.dwUser = 0;
						whdri.dwFlags = 0;
						whdri.dwLoops = 1;
						waveInPrepareHeader(hWaveIn, &whdri, sizeof(WAVEHDR));
						waveInAddBuffer(hWaveIn, &whdri, sizeof(WAVEHDR));
						waveInStart(hWaveIn);
						Sleep(timems);
						const size_t nSamples = (size_t)whdri.dwBytesRecorded / 2;
						waveInReset(hWaveIn);
						std::lock_guard lock(mx);
						if(pcmQueue.empty() || pcmQueue.back().size() == StreamSize)
							pcmQueue.emplace_back(pcmVector.data(), pcmVector.data() + nSamples);
						else
						{
							auto& BackData = pcmQueue.back();
							if(BackData.size() + nSamples > StreamSize)
							{
								const auto RealSize = StreamSize - BackData.size();
								BackData.insert(BackData.end(), pcmVector.data(), pcmVector.data() + RealSize);
								pcmQueue.emplace_back(pcmVector.data() + RealSize, pcmVector.data() + nSamples);
							}
							else
								BackData.insert(BackData.end(), pcmVector.data(), pcmVector.data() + nSamples);
						}
					}
				});
			RecoderThread.detach();
		}

		void Stop() const
		{
			if(isBegin)
			{
				waveInStop(hWaveIn);
				waveInReset(hWaveIn);
			}
		}

		std::vector<int16_t> GetStreamData()
		{
			std::lock_guard lock(mx);
			if (pcmQueue.empty() || pcmQueue[0].size() != StreamSize)
				return {};
			auto Stream = std::move(pcmQueue[0]);
			pcmQueue.pop_front();
			return Stream;
		}
	private:
		DWORD SamplingRateSrc = 44100;
		std::vector<int16_t> pcmVector;
		std::deque<std::vector<int16_t>> pcmQueue;
		size_t StreamSize = 0;
		DWORD timems = 0;
		HWAVEIN hWaveIn = nullptr;
		WAVEFORMATEX waveform{ WAVE_FORMAT_PCM,1,44100,88200,2,16,0 };
		WAVEHDR whdri{ nullptr,0,0,0,0,0,nullptr,0 };
		HANDLE WaitEvent = nullptr;
		bool isBegin = false;
		std::mutex mx;
	};
	class MPCMPlayer
	{
	public:
		MPCMPlayer() = default;
		~MPCMPlayer()
		{
			if (!hWaveOut)
				return;
			waveOutClose(hWaveOut);
		}
		void initPlayer(DWORD SamplingRate = 44100)
		{
			waveform.nSamplesPerSec = SamplingRate;
			waveform.wBitsPerSample = 16;
			waveform.nChannels = 1;
			waveform.cbSize = 0;
			waveform.wFormatTag = WAVE_FORMAT_PCM;
			waveform.nBlockAlign = (waveform.wBitsPerSample * waveform.nChannels) / 8;
			waveform.nAvgBytesPerSec = waveform.nBlockAlign * waveform.nSamplesPerSec;
			WaitEvent = CreateEvent(nullptr, 0, 0, nullptr);
			waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveform, (DWORD_PTR)WaitEvent, 0L, CALLBACK_EVENT);
			SAMP = SamplingRate;
		}
		void Play(std::vector<int16_t>& data)
		{
			whdri.lpData = (LPSTR)data.data();
			whdri.dwBufferLength = DWORD(data.size() * 2);
			whdri.dwFlags = 0L;
			whdri.dwLoops = 1L;
			waveOutPrepareHeader(hWaveOut, &whdri, sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, &whdri, sizeof(WAVEHDR));
			Sleep(DWORD(data.size() * 1000 / size_t(SAMP)));
		}
	private:
		HWAVEOUT hWaveOut = nullptr;
		WAVEFORMATEX waveform{ WAVE_FORMAT_PCM,1,44100,88200,2,16,0 };
		WAVEHDR whdri{ nullptr,0,0,0,0,0,nullptr,0 };
		HANDLE WaitEvent = nullptr;
		DWORD SAMP = 44100;
	};

	MoeVSProjectSpace::MoeVSSvcParams Params;
	short Threshold = 400;
	MRecorder RTRecorder;
	MPCMPlayer RTPlayer;
	std::deque<std::vector<int16_t>> InputBuffer, OutputBuffer, rawInputBuffer, rawOutputBuffer;
	bool RTIsEnabled = false;
	size_t crossfade_length = 0;
	size_t extra_length = 0;

	void EndRtInference()
	{
		RTRecorder.Stop();
		RTIsEnabled = false;
		InputBuffer.clear();
		OutputBuffer.clear();
		rawInputBuffer.clear();
		rawOutputBuffer.clear();
	}

	void RTInference()
	{
		if (RTIsEnabled)
		{
			EndRtInference();
			return;
		}
		std::wstring error;
		RTIsEnabled = true;
		crossfade_length = Params.CrossFadeLength;
		extra_length = crossfade_length / 4;
		std::thread RT_RECORD_THREAD = std::thread([&]()
			{
				logger.log(L"[RTInference] Recording Thread Start!");
				while (RTIsEnabled)
				{
					auto PCM = RTRecorder.GetStreamData();
					if(PCM.empty())
						continue;
					rawInputBuffer.emplace_back(std::move(PCM));

					if (rawInputBuffer.size() > 2)
					{
						std::vector<int16_t> pBuffer;
						pBuffer.reserve(rawInputBuffer[1].size() + 4 * crossfade_length);
						pBuffer.insert(pBuffer.end(),
							rawInputBuffer[0].end() - int64_t(crossfade_length + extra_length),
							rawInputBuffer[0].end());
						pBuffer.insert(pBuffer.end(), rawInputBuffer[1].begin(), rawInputBuffer[1].end());
						pBuffer.insert(pBuffer.end(),
							rawInputBuffer[2].begin(),
							rawInputBuffer[2].begin() + int64_t(crossfade_length + extra_length) + 1000);
						InputBuffer.emplace_back(std::move(pBuffer));
						rawInputBuffer.pop_front();
					}
					if (rawInputBuffer.size() > 100)
						rawInputBuffer.pop_front();
				}
				logger.log(L"[RTInference] Recording Thread End!");
			});

		std::thread RT_INFERENCE_THREAD = std::thread([&]()
			{
				logger.log(L"[RTInference] Inferencing Thread Start!");
				while (RTIsEnabled)
				{
					if (!InputBuffer.empty())
					{
						try
						{
							if (MoeVSModuleManager::GetCurSvcModel())
							{
								bool zeroVector = true;
								for (const auto& i16data : InputBuffer[0])
								{
									if (i16data > Threshold * 10)
									{
										zeroVector = false;
										break;
									}
								}
								if (zeroVector)
									rawOutputBuffer.emplace_back(std::vector<int16_t>(InputBuffer[0].size(), 0));
								else
									rawOutputBuffer.emplace_back(MoeVSModuleManager::GetCurSvcModel()->InferPCMData(InputBuffer[0], (long)MoeVSModuleManager::SamplingRate, Params));
							}
							else
								rawOutputBuffer.emplace_back(std::move(InputBuffer[0]));
							InputBuffer.pop_front();
						}
						catch (std::exception& e)
						{
							logger.error(e.what());
							EndRtInference();
						}
					}
					if (InputBuffer.size() > 100)
						InputBuffer.pop_front();
				}
				logger.log(L"[RTInference] Inferencing Thread End!");
			});

		std::thread RT_OUTPUT_THREAD = std::thread([&]()
			{
				logger.log(L"[RTInference] OutPut Thread Start!");
				while (RTIsEnabled)
				{
					if (rawOutputBuffer.size() > 2)
					{
						std::vector pBuffer(
							rawOutputBuffer[1].begin() + (int64_t)(crossfade_length + extra_length),
							rawOutputBuffer[1].end()
						);
						pBuffer.resize(RTRecorder.GetFrameSize());

						const auto dataBufr = pBuffer.size() - crossfade_length;
						const auto crossBufl = crossfade_length + extra_length + RTRecorder.GetFrameSize();
						const auto crossBufr = extra_length;

						for (size_t i = 0; i < crossfade_length; ++i)
						{
							const auto crosf1 = (double(i) / double(crossfade_length));
							const auto crosf2 = (1. - (double(i) / double(crossfade_length)));

							pBuffer[i] = (int16_t)(
								double(pBuffer[i]) * crosf1 +
								(double)rawOutputBuffer[0][i + crossBufl] * crosf2
								);

							pBuffer[i + dataBufr] = (int16_t)(
								double(pBuffer[i + dataBufr]) * crosf2 +
								(double)rawOutputBuffer[2][i + crossBufr] * crosf1
								);
						}
						OutputBuffer.emplace_back(std::move(pBuffer));
						rawOutputBuffer.pop_front();
					}
					if (!OutputBuffer.empty())
					{
						RTPlayer.Play(OutputBuffer.front());
						OutputBuffer.pop_front();
					}
				}
				logger.log(L"[RTInference] OutPut Thread End!");
			});
		RTRecorder.Start();
		logger.log(L"[RTInference] Start RTInference!");
		RT_RECORD_THREAD.detach();
		RT_INFERENCE_THREAD.detach();
		RT_OUTPUT_THREAD.detach();
	}
}

int main()
{
	MoeVSModuleManager::MoeVoiceStudioCoreInitSetup();

	try
	{
		MoeVSModuleManager::LoadSvcModel(
			MJson(to_byte_string(GetCurrentFolder() + L"/Models/ShirohaRVC.json").c_str()),
			[](size_t cur, size_t total)
			{
				//std::cout << (double(cur) / double(total) * 100.) << "%\n";
			},
			0,
			8,
			0
			);
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
		return 0;
	}

	RtInferenceSpace::Params.Sampler = L"DDim";
	RtInferenceSpace::Params.Step = 100;
	RtInferenceSpace::Params.Pndm = 10;
	RtInferenceSpace::Params.F0Method = L"RMVPE";
	RtInferenceSpace::Params.CrossFadeLength = 8000;
	RtInferenceSpace::Params.Keys = 8;

	RtInferenceSpace::RTRecorder.initRecorder((DWORD)MoeVSModuleManager::SamplingRate);
	RtInferenceSpace::RTRecorder.setStreamBufferSize(0.5);
	RtInferenceSpace::RTRecorder.Start();
	RtInferenceSpace::RTPlayer.initPlayer((DWORD)MoeVSModuleManager::SamplingRate);

	RtInferenceSpace::RTInference();

	while (true);
	while (true)
	{
		auto PCM = RtInferenceSpace::RTRecorder.GetStreamData();
		if (!PCM.empty())
			RtInferenceSpace::RTPlayer.Play(PCM);
	}
}
#endif

#include "LibDLVoiceCodec/value.h"
class Class0 : libdlvcodec::Module
{
public:
	Class0(Module* _Parent, const std::string& _Name) : Module(_Parent, _Name) {}
};

class ClassA : libdlvcodec::Module
{
public:
	ClassA(Module* _Parent, const std::string& _Name) : Module(_Parent, _Name) {}
private:
	RegLayer(Class0, attrC0);
};

class ClassB : libdlvcodec::Module
{
public:
	ClassB() : Module(nullptr, "ClassB") {}
private:
	RegLayer(ClassA, attrCA);
};

int main()
{
	ClassB a;
	printf("%d", &a);
}