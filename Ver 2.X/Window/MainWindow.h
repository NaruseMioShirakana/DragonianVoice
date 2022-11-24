/*
* file: MainWindow.h
* info: 应用程序主窗口定义 使用MiaoUI
*
* Author: Maplespe(mapleshr@icloud.com)
* 
* date: 2022-9-19 Create
*/
#pragma once
#include "..\framework.h"
#include <onnxruntime_cxx_api.h>
#include <Render\Sound\Mui_DirectSound.h>
#include <Render\Sound\Mui_AudioOgg.h>
namespace ttsUI
{
	class MioTTSControl;
	using namespace Mui;
	//主窗口尺寸
	extern const UISize m_wndSize;
	//主窗口标题
	extern std::wstring m_wndTitle;

	//初始化and反初始化
	extern bool MiaoUI_Initialize(std::wstring& error);
	extern void MiaoUI_Uninitialize();

	//应用程序主窗口 WIndows平台
	class MainWindow : public Window::UIWindowsWnd
	{
	public:
		MainWindow(MRender* _render_) : UIWindowsWnd(_render_) {}
		~MainWindow();

		//界面事件回调
		virtual bool EventProc(UINotifyEvent, UIControl*, _m_param) override;
		//窗口事件源 WIndows窗口事件回调
		virtual _m_result EventSource(_m_param, _m_param) override;

		void setTTSCon(MioTTSControl*&& _ttsCon)
		{
			ttsCon = _ttsCon;
		}
	private:
		bool AfterCreated();
		bool CreateControls(UIControl* root);
		MioTTSControl* ttsCon = nullptr;
		MHelper::MuiXML* m_uiXML = nullptr;
		MFontLoader* m_fontLoader = nullptr;
		friend void MainWindow_CreateLoop();
	};

	struct modelInfo
	{
		std::wstring Folder;
		std::wstring Name;
		std::wstring Type;
		std::wstring Symbol;
		std::wstring Cleaner;
		std::vector<std::wstring> Speakers;
	};

	enum class modelType
	{
		Taco,
		Vits,
		Vitm
	};

	//控件控制
	class MioTTSControl
	{
	public:
		MioTTSControl() = delete;
		MioTTSControl(ttsUI::MainWindow*&& _main)
		{
			_mainWindow = _main;
			_uiControl = _main->GetRootControl();
			hWnd = (HWND)(_main->GetWindowHandle());
		}
		UIProgressBar* process = nullptr;      //转换进度
		UIBlurLayer* titlebar = nullptr;       //主题栏
		UIButton* title_close = nullptr;       //关闭按钮
		UIButton* title_minisize = nullptr;    //最小化按钮
		UILabel* title_label = nullptr;        //标题
		IconButton* tts_begin = nullptr;       //开始转换按钮
		UIEditBox* wave_editbox = nullptr;     //输入字符串
		UIComBox* modlist_main = nullptr;      //模型列表
		UIComBox* modlist_child = nullptr;     //说话人
		IconButton* modlist_refresh = nullptr; //刷新列表
		IconButton* modlist_folder = nullptr;  //打开模型文件夹
		UILabel* voice_timecur = nullptr;      //当前播放时间
		UILabel* voice_timeall = nullptr;      //音频时长
		UIButton* voice_play = nullptr;        //播放按钮
		IconButton* voice_save = nullptr;      //保存按钮
		UISlider* voice_volume = nullptr;      //音量滑块
		UILabel* volume_text = nullptr;        //音量
		UIImgBox* voice_imgbox = nullptr;      //模型人物
		UILabel* voice_imgname = nullptr;      //模型名
		UISlider* voice_prog = nullptr;        //播放进度条
		IconButton* clean_begin = nullptr;     //清理输入文本
		Render::MDS_AudioPlayer* wavPlayer = nullptr;
		Render::MAudioTrack* wavTrack = nullptr;
		friend class MainWindow;
		//加载模型
		void loadmodel();
		//注册左键点击事件
		void RegisterLeftClickEvent(std::wstring&& _control, std::function<bool()>&& _fun) {
			_levents.insert(std::pair<std::wstring, std::function<bool()>>(_control, _fun));
		}
		//注册数值事件
		void RegisterValEvent(std::wstring&& _control, std::function<bool(uint64_t)>&& _fun) {
			_sevents.insert(std::pair<std::wstring, std::function<bool(uint64_t)>>(_control, _fun));
		}
		//注册控件
		void RegisterControl();
		//注册事件
		void RegisterEvent();
		//初始化控件
		void setConsDefalut() const;
		//清除模型
		void ClearModel();
		//初始化
		void InitControl();
		//初始化
		void InferQuit() const;
		//Tacotron2
		void InferTaco2(const std::wstring& _input);
		//InferVits
		void InferVits(const std::vector<std::wstring>& _input);
		//InferVitsM
		void InferVitsM(const std::vector<std::wstring>& _input,int chara);
		//Message
		void MessageCreate(const std::wstring& _input) const
		{
			MessageBox(hWnd, _input.c_str(), L"", MB_OK);
		}
		//cleaner
		std::wstring getCleanerStr(std::wstring& _input) const;

		void setSoundModule(bool Switch) const
		{
			voice_play->SetEnabled(Switch);
			voice_save->SetEnabled(Switch);
			voice_prog->SetEnabled(Switch);
		}
	protected:
		std::map <std::wstring, std::function<bool()>> _levents;
		std::map <std::wstring, std::function<bool(uint64_t)>> _sevents;
		std::vector<modelInfo> _models;
		std::vector<short> _wavData;
		std::wstring _curCleaner;
		MainWindow* _mainWindow = nullptr;
		UIControl* _uiControl = nullptr;
		HWND hWnd = nullptr;
		modelType _modelType = modelType::Taco;
		std::map<wchar_t, int64_t> _symbol;
		Ort::Session* sessionEncoder = nullptr;
		Ort::Session* sessionDecoderIter = nullptr;
		Ort::Session* sessionPostNet = nullptr;
		Ort::Session* sessionGan = nullptr;
		/*
		Ort::Session* sessionDec = nullptr;
		Ort::Session* sessionDp = nullptr;
		Ort::Session* sessionEnc_p = nullptr;
		Ort::Session* sessionFlow = nullptr;
		Ort::Session* sessionEmb = nullptr;
		*/
		Ort::Env* env = nullptr;
		Ort::SessionOptions* session_options = nullptr;
		Ort::MemoryInfo* memory_info = nullptr;
		bool playstat = false;
	};
	
	//创建主窗口
	extern void MainWindow_CreateLoop();
}


