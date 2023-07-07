#include "Recorder.h"

MRecorder::~MRecorder()
{
    //释放录制线程
    m_runing = false;
    m_iscapture = true;
    if (m_thread)
    {
        m_signal.notify_all();
        m_thread->join();
        delete m_thread;
    }
    //释放设备资源
    if (m_event[0])
    {
        for (Mui::_m_byte i = 0; i < 2; i++)
        {
            SetEvent(m_event[i]);
            CloseHandle(m_event[i]);
        }
    }
    if (m_captureBuffer) m_captureBuffer->Release();
    if (m_captureDriver) m_captureDriver->Release();
    //释放队列数据
    while (true)
    {
        const auto data = GetStreamData();
        delete data;
        if (!data)
            break;
    }
}

bool MRecorder::InitRecorder(std::wstring& error, Mui::_m_uint sampleRate, Mui::_m_byte channels, Mui::_m_byte bit, Mui::_m_uint fs)
{
    if (m_captureDriver && m_captureBuffer)
    {
        error = L"Initialized";
        return false;
    }

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    //创建录音设备
    HRESULT hr = DirectSoundCaptureCreate(nullptr, &m_captureDriver, nullptr);
    if (FAILED(hr))
    {
        error = L"DirectSoundCaptureCreate failed!";
        return false;
    }

    if(sampleRate != 0)
		m_sampleRate = sampleRate;
    if(channels != 0)
		m_channels = channels;
    if(bit != 0)
		m_bit = bit;

    //创建录音缓冲区
    m_blockAlign = 2 * m_channels;

    WAVEFORMATEX wavFormatEx;
    wavFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    wavFormatEx.nChannels = m_channels;
    wavFormatEx.nSamplesPerSec = m_sampleRate;
    wavFormatEx.nAvgBytesPerSec = m_sampleRate * m_blockAlign;
    wavFormatEx.nBlockAlign = WORD(m_blockAlign);
    wavFormatEx.wBitsPerSample = m_bit;

    m_frameSize = fs*2;

    DSCBUFFERDESC dsBufferDesc = { sizeof(DSCBUFFERDESC) };
    dsBufferDesc.lpwfxFormat = &wavFormatEx;
    dsBufferDesc.dwBufferBytes = m_frameSize * 2;
    //dsBufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;

    hr = m_captureDriver->CreateCaptureBuffer(&dsBufferDesc, &m_captureBuffer, nullptr);
    if (FAILED(hr))
    {
        error = L"DirectSoundCaptureCreate failed!";
        m_captureDriver->Release(); m_captureDriver = nullptr;
        return false;
    }
    
    //创建通知事件
    IDirectSoundNotify* Notify = nullptr;
    m_captureBuffer->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&Notify);

    DSBPOSITIONNOTIFY notify[2] = { 0 };
    int offset = int(m_frameSize - 1);
    for (int i = 0; i < 2; i++) {
        notify[i].dwOffset = offset;
        m_event[i] = CreateEventW(nullptr, false, false, nullptr);
        notify[i].hEventNotify = m_event[i];
        offset += (int)m_frameSize;
    }

    Notify->SetNotificationPositions(2, notify);
    Notify->Release();

    //创建录制线程
    m_runing = true;
    m_iscapture = false;

    m_thread = new std::thread(&MRecorder::CaptureThread, this);

	return true;
}

bool MRecorder::Start()
{
    if(m_captureBuffer)
    {
        if (m_iscapture) 
            return true;

        m_iscapture = true;
        m_signal.notify_all();
        return SUCCEEDED(m_captureBuffer->Start(DSBPLAY_LOOPING));
    }
    return false;
}

bool MRecorder::Stop()
{
    if(m_captureBuffer)
    {
        if (!m_iscapture)
            return true;

        m_iscapture = false;
        return SUCCEEDED(m_captureBuffer->Stop());
    }
    return false;
}

MRecorder::PCMData* MRecorder::GetStreamData()
{
    PCMData* ret = nullptr;
    m_queue.dequeue(ret);
    return ret;
}

Mui::_m_size MRecorder::GetDataSize()
{
    return Mui::_m_size(m_queue.size() * m_frameSize);
}

void MRecorder::CaptureThread()
{
    while (m_runing)
    {
        //休眠线程
        if (!m_iscapture) {
            std::unique_lock<std::mutex> lock(m_lock);
            while (!m_iscapture) m_signal.wait(lock);
        }

        //等待通知事件
        const DWORD res = WaitForMultipleObjects(2, m_event, FALSE, INFINITE);

        if (!m_iscapture) continue;

        if (res >= WAIT_OBJECT_0 && res <= WAIT_OBJECT_0 + 1) 
        {
            auto data = new PCMData();
            UCHAR* buffer = nullptr;
            DWORD dwBufferSize = 0;
            if (FAILED(m_captureBuffer->Lock(m_offsetNum * m_frameSize, m_frameSize, (LPVOID*)&buffer, &dwBufferSize, nullptr, nullptr, 0)))
                continue;

            data->data = new Mui::_m_byte[dwBufferSize];
            data->size = dwBufferSize;
            memcpy(data->data, buffer, dwBufferSize);
            m_queue.enqueue(data);

            m_captureBuffer->Unlock(buffer, dwBufferSize, nullptr, 0);

            m_offsetNum++;
            if (m_offsetNum == 2)
                m_offsetNum = 0;
        }
    }
}
