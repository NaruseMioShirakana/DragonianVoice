// AudioRecording.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "Recorder.h"
#include <Render/Sound/Mui_AudioWave.h>
#include <Render/Sound/Mui_DirectSound.h>

MRecorder* m_recorder = nullptr;
MDS_AudioPlayer* m_player = nullptr;
std::thread* m_playThread = nullptr;
MAudioStream* m_stream = nullptr;
bool m_play = true;
bool recordFiles = false;

class MainWindow : public UIWindowsWnd
{
public:
	MainWindow(MRender* pRender) : UIWindowsWnd(pRender) {}

	bool AfterCreated();

	bool EventProc(UINotifyEvent event, UIControl* control, _m_param param) override
	{
		return false;
	}

	_m_lpcwstr m_title = L"录音测试demo";
	UISize m_size = { 870, 620 };

private:
};


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	//注册Mui所有控件
	CtrlMgr::RegisterMuiControl();

	std::wstring error;
	if (!InitDirect2D(&error, -1))
	{
		MessageBoxW(nullptr, (L"初始化渲染引擎失败！错误信息：" + error).c_str(), L"ERROR", MB_ICONERROR);
		return -1;
	}

	MainWindow window(new MRender_D2D());
	auto func = [ObjectPtr = &window] { return ObjectPtr->AfterCreated(); };
	if (window.Create(0, window.m_title, UIRect(0, 0, window.m_size.width, window.m_size.height), func, 0))
	{
		window.SetMainWindow(true);

		HDC hdc = GetDC(nullptr);
		UINT dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(nullptr, hdc);
		float scale = (float)dpiX / 96.f;
		window.ScaleWindow(int((float)window.m_size.width * scale), int((float)window.m_size.height * scale));

		window.CenterWindow();
		window.ShowWindow(true);

		UIMessageLoop();
	}

	if (m_playThread)
	{
		m_play = false;
		m_playThread->join();
		delete m_playThread;
	}
	if (m_player)
		m_player->StopTrack(m_stream);
	if (m_recorder)
		m_recorder->Stop();

	delete m_recorder;
	delete m_player;

	UninitDirect2D();
	return 0;
}

//wav头
struct WaveHeader {
	char chunk_id[4] = { 'R', 'I', 'F', 'F' };
	uint32_t chunk_size;
	char format[4] = { 'W', 'A', 'V', 'E' };
	char subchunk1_id[4] = { 'f', 'm', 't', ' ' };
	uint32_t subchunk1_size = 16;
	uint16_t audio_format = 1;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	char subchunk2_id[4] = { 'd', 'a', 't', 'a' };
	uint32_t subchunk2_size;
};

void SaveToWaveFile(std::wstring path)
{
	auto datasize = m_recorder->GetDataSize() * m_recorder->GetBlockAlign();

	WaveHeader header;
	header.chunk_size = 36 + datasize;
	header.num_channels = m_recorder->GetChannels();
	header.sample_rate = m_recorder->GetSampleRate();
	header.subchunk2_size = datasize;
	header.block_align = m_recorder->GetBlockAlign();
	header.bits_per_sample = m_recorder->GetBitRate();
	header.byte_rate = (header.sample_rate * header.bits_per_sample * header.num_channels) / 8;

	FILE* out = _wfsopen(path.c_str(), L"wb", _SH_DENYNO);
	fwrite(&header, sizeof(header), 1, out);

	while (true)
	{
		auto data = m_recorder->GetStreamData();
		if (!data) break;
		fwrite(data->data, data->size, 1, out);
		delete data;
	}
	fclose(out);
}


bool MainWindow::AfterCreated()
{
	UIControl* root = new UIControl();
	AddRootControl(root);
	UIBkgndStyle bgColor;
	bgColor.background = Color::M_White;
	root->SetBackground(bgColor);

	//创建录音器
	m_recorder = new MRecorder(this);

	std::wstring error;
	if(!m_recorder->InitRecorder(error))
	{
		MessageBoxW(NULL, (L"录音器初始化失败！error:" + error).c_str(), L"error", MB_ICONERROR);
		return false;
	}

	recordFiles = MessageBoxW(NULL, L"是否录制到文件? 是(录制10秒录音到 '测试.wav' 否则实时播放)", L"录音", MB_ICONQUESTION | MB_YESNO) == IDYES;

	//如果不录制到文件 实时播放 创建一个播放器
	if (!recordFiles)
	{

		//创建播放器
		m_player = new MDS_AudioPlayer(this);
		if (!m_player->InitAudioPlayer(error))
		{
			MessageBoxW(NULL, (L"播放初始化失败！error:" + error).c_str(), L"error", MB_ICONERROR);
			return false;
		}

		//创建PCM流
		m_stream = m_player->CreateStreamPCM(
			m_recorder->GetFrameSize(),
			m_recorder->GetSampleRate(),
			m_recorder->GetChannels(),
			m_recorder->GetBitRate(),
			m_recorder->GetBlockAlign()
		);

		auto PlayerThread = []
		{
			while (m_play)
			{
				auto data = m_recorder->GetStreamData();
				if (!data) continue;
				m_player->WriteStreamPCM(m_stream, data->data, data->size);
				delete data;
			}
		};

		m_playThread = new std::thread(PlayerThread);
		m_player->PlayTrack(m_stream);
	}
	m_recorder->Start();

	if (recordFiles)
	{
		Sleep(10000);

		m_recorder->Stop();

		SaveToWaveFile(L"测试.wav");
	}
	return true;
}