#pragma once
#include <dsound.h>
#include <deque>
#include <Mui_Base.h>

#pragma comment(lib, "dsound.lib")

class MRecorder
{
public:
	struct PCMData
	{
		Mui::_m_lpbyte data = nullptr;
		Mui::_m_uint size = 0;

		~PCMData() { delete data; }
	};

	MRecorder() = default;
	~MRecorder();

	/*初始化录音器
	* @param out error - 错误信息输出
	* @param sampleRate - 采样率 为0则默认 44100 (最大支持44100)
	* @param channels - 声道数 为0则默认 2 (最大2声道立体声)
	* @param bit - 位数 为0则默认 16 (最大支持16位)
	*/
	bool InitRecorder(std::wstring& error, Mui::_m_uint sampleRate = 0, Mui::_m_byte channels = 0, Mui::_m_byte bit = 0, Mui::_m_uint fs = 0);

	bool Start();

	bool Stop();

	//获取一段已录制的PCM音频数据 nullptr则没有数据
	PCMData* GetStreamData();

	//获取总dataSize
	Mui::_m_size GetDataSize();

	//获取采样率
	Mui::_m_uint GetSampleRate() const { return m_sampleRate; }

	//获取声道数
	Mui::_m_byte GetChannels() const { return m_channels; }

	//获取位数
	Mui::_m_byte GetBitRate() const { return m_bit; }

	//获取音频帧尺寸
	Mui::_m_uint GetFrameSize() const { return m_frameSize; }

	//获取BlockAlign
	Mui::_m_uint GetBlockAlign() const { return m_blockAlign; }

private:
	//音频参数设置
	Mui::_m_uint m_sampleRate = 44100;
	Mui::_m_byte m_channels = 2;
	Mui::_m_byte m_bit = 16;

	Mui::_m_uint m_frameSize = 0;

	Mui::_m_uint m_blockAlign = 0;

	HANDLE m_event[2] = { nullptr };

	IDirectSoundCapture* m_captureDriver = nullptr;
	IDirectSoundCaptureBuffer* m_captureBuffer = nullptr;

	//录音线程相关
	std::mutex m_lock;
	std::condition_variable m_signal;
	std::atomic_bool m_runing;
	std::atomic_bool m_iscapture;
	std::thread* m_thread = nullptr;

	Mui::_m_uint m_offsetNum = 0;

	void CaptureThread();

	//录音数据
	template <typename T>
	class Queue
	{
		std::deque<T> m_queue;
		std::mutex m_mutex;
	public:
		Queue() = default;

		bool empty()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			return m_queue.empty();
		}
		auto size()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			return m_queue.size();
		}
		void enqueue(T& t)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_queue.push_back(t);
		}
		bool dequeue(T& t)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_queue.empty())
				return false;
			t = std::move(m_queue.front());
			m_queue.pop_front();
			return true;
		}
		void clear()
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_queue.clear();
		}
	};

	Queue<PCMData*> m_queue;
};