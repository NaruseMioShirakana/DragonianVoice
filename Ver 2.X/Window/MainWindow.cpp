/*
* file: MainWindow.cpp
* info: 应用程序主窗口实现 使用MiaoUI
*
* Author: Maplespe(mapleshr@icloud.com)
*
* date: 2022-9-19 Create.
*/

#include "MainWindow.h"
#pragma warning(disable : 4996)
#include <codecvt>
#include "..\Helper\Helper.h"
#include "MioShirakanaTensor.hpp"
#include <fstream>
#include <io.h>
#include <commdlg.h>
#define _MNAME(x) (control->GetName() == (x))
namespace ttsUI
{
	std::wstring charaSets = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	inline std::wstring to_wide_string(const std::string& input)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.from_bytes(input);
	}
	inline std::string to_byte_string(const std::wstring& input)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(input);
	}

	using namespace Mui;

	//主窗口尺寸
	const UISize m_wndSize = { 1200, 776 };
	//主窗口标题
	std::wstring m_wndTitle = L"MioTTS - Shirakana says: Cxx is the best lang in the world";

	//界面资源
	DMResources* m_uiRes = nullptr;
	auto constexpr m_resKey = L"12345678";

	bool MiaoUI_Initialize(std::wstring& error)
	{
		//注册MiaoUI系统控件
		CtrlMgr::RegisterMuiControl();
		//注册自定义控件
		IconButton::Register();

		//初始化渲染器
		if (!InitDirect2D(&error, -1))
			return false;
		//加载资源文件
		const std::wstring cfgpath = GetCurrentFolder() + L"\\ShirakanaUI.dmres";
		m_uiRes = new DMResources();
		if (!m_uiRes->LoadResource(cfgpath, false))
		{
			error = L"加载文件失败：" + cfgpath;
			return false;
		}

		return true;
	}

	void MiaoUI_Uninitialize()
	{
		if (m_uiRes) {
			m_uiRes->CloseResource();
			delete m_uiRes;
		}
		UninitDirect2D();
	}

	void MainWindow_CreateLoop()
	{
		MainWindow window(new MRender_D2D());
		auto func = std::bind(&MainWindow::AfterCreated, &window);
		if (window.Create(0, m_wndTitle, UIRect(0, 0, m_wndSize.width, m_wndSize.height), func, WS_CAPTION | WS_MINIMIZEBOX, 0))
		{
			window.SetMainWindow(true);

			//DPI缩放
			UINT dpiX, dpiY = 0;
			if (GetWNdMonitorDPI((HWND)window.GetWindowHandle(), dpiX, dpiY))
			{
				float scaleX = (float)dpiX / 96.f;
				float scaleY = (float)dpiY / 96.f;
				window.ScaleWindow(int(m_wndSize.width * scaleX), int(m_wndSize.height * scaleY));
			}

			//垂直同步
			window.SetVerticalSync(true);
			//开启内存资源缓存
			window.SetResMode(true);
			
			//居中和显示窗口
			window.CenterWindow();
			window.ShowWindow(true);

			//注册控件
			MioTTSControl _ttsCon{ &window };
			window.setTTSCon(&_ttsCon);

			//初始化
			_ttsCon.InitControl();
			_ttsCon.wavPlayer = new MDS_AudioPlayer(&window);
			std::wstring errTmp;
			_ttsCon.wavPlayer->InitAudioPlayer(errTmp);

			//窗口消息循环
			UIMessageLoop();
		}
	}

	MainWindow::~MainWindow()
	{
		if (m_uiXML) delete m_uiXML;
		if (m_fontLoader) delete m_fontLoader;
	}

	bool MainWindow::EventProc(UINotifyEvent event, UIControl* control, _m_param param)
	{
		switch (event)
		{
		case Event_Mouse_LClick:
			if (ttsCon->_levents.find(control->GetName()) != ttsCon->_levents.end())
			{
				return (ttsCon->_levents.find(control->GetName())->second)();
			}
			break;
		case Event_Mouse_LDown:
			if (_MNAME(L"titlebar")) {
				::SendMessageW(ttsCon->hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
				return true;
			}
			break;
		case Event_Slider_Change:
			if (ttsCon->_sevents.find(control->GetName()) != ttsCon->_sevents.end())
			{
				return (ttsCon->_sevents.find(control->GetName())->second)(param);
			}
			break;
		case Event_ListBox_ItemChanged:
			if (ttsCon->_sevents.find(control->GetName()) != ttsCon->_sevents.end())
			{
				return (ttsCon->_sevents.find(control->GetName())->second)(param);
			}
			break;
		}
		return false;
	}

	_m_result MainWindow::EventSource(_m_param message, _m_param param)
	{
		switch (message)
		{
			//自绘标题栏 扩展窗口客户区
			case WM_NCCALCSIZE:
			{
				auto pm = (std::pair<_m_param, _m_param>*)param;
				if (!pm->second)
					return UIWindowBasic::EventSource(message, param);

				HWND hWnd = (HWND)GetWindowHandle();

				UINT dpi = GetDpiForWindow(hWnd);

				int frameX = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
				int frameY = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
				int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

				NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)pm->second;
				RECT* rgrc = params->rgrc;

				rgrc->right -= frameX + padding;
				rgrc->left += frameX + padding;
				rgrc->bottom -= frameY + padding;

				WINDOWPLACEMENT placement = { 0 };
				placement.length = sizeof(WINDOWPLACEMENT);
				if (GetWindowPlacement(hWnd, &placement)) {
					if(placement.showCmd == SW_SHOWMAXIMIZED)
						rgrc->top += padding;
				}

				return true;
			}
			break;
			//重新计算框架 扩展到整窗
			case WM_CREATE:
			{
				auto rect = GetWindowRect();
				SetWindowPos((HWND)GetWindowHandle(), NULL, rect.left, rect.top,
					rect.GetWidth(), rect.GetHeight(), SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
				return UIWindowBasic::EventSource(message, param);
			}
			break;
			//限定窗口大小
			case WM_GETMINMAXINFO:
			{
				auto pm = (std::pair<_m_param, _m_param>*)param;
				MINMAXINFO* p = (MINMAXINFO*)pm->second;
				p->ptMaxSize.x = m_wndSize.width;
				p->ptMaxSize.y = m_wndSize.height;
				return true;
			}
			break;
			//响应系统DPI更改
			case WM_DPICHANGED:
			{
				auto pm = (std::pair<_m_param, _m_param>*)param;
				UINT dpi = M_LOWORD((_m_long)pm->first);
				float scale = (float)dpi / 96.f;
				ScaleWindow(int(m_wndSize.width * scale), int(m_wndSize.height * scale));
				return 0;
			}
			break;
			//窗口即将关闭
			case WM_CLOSE:
			{
				//先执行界面库内部释放方法
				auto ret = UIWindowBasic::EventSource(message, param);
				//最后再销毁XML类
				if (m_uiXML) {
					delete m_uiXML;
					m_uiXML = nullptr;
				}
				return ret;
			}
			break;
		default:
			return UIWindowBasic::EventSource(message, param);
			break;
		}
	}

	bool MainWindow::AfterCreated()
	{
		//窗口必须有一个根控件
		//
		UIControl* root = new UIControl();
		AddRootControl(root);
		//设置窗口背景色为白色
		UIBkgndStyle bgColor;
		bgColor.background = Color::M_RGBA(255, 255, 255, 255);
		root->SetBackground(bgColor);
		//创建界面
		return CreateControls(root);
	}

	bool MainWindow::CreateControls(UIControl* root)
	{
		m_uiXML = new MHelper::MuiXML(this);
		m_uiXML->LoadResource(m_uiRes, m_resKey);
		m_uiXML->LoadSkinList();

		//加载内存字体
		m_fontLoader = new MFontLoader(GetRender());
		UIResource font = m_uiRes->ReadResource(L"font_TSYuMo", m_resKey, DataRes).res;
		m_fontLoader->AddFontResources(font);
		font.Release();

		//导入内存字体样式
		UILabel::Property style;
		style.Font = L"TsangerYuMo";
		style.FontSize = 14;
		style.AutoSize = true;
		style.FontCustom = m_fontLoader->CreateLoader();

		m_uiXML->AddFontStyle(L"style", style);

		//组合框列表字体样式
		style.AutoSize = false;
		style.FontSize = 12;
		style.Font = L"Microsoft YaHei UI";
		style.FontCustom = 0;
		style.Alignment = TextAlign_VCenter;

		m_uiXML->AddFontStyle(L"comList", style);

		std::wstring xml = LR"(
		<root>

		<!-- 组合框样式组 -->
		<PropGroup textAlign="4" skin="skin_combox" skin1="skin_scroll" listSkin="skin_comlist" resIcon="icon_comlist"
		itemSkin="skin_comlistitem" menuHeight="150" itemFontStyle="comList" barWidth="6" id="comstyle" />

		<!-- 模块样式组 -->
		<PropGroup blur="5" frameWidth="1" frameColor="189,189,189,255" frameRound="15" frameRound="15"
		bgColor="255,255,255,180" clipRound="15" id="group" />

		<!-- 图标按钮样式组 -->
		<PropGroup skin="skin_button" fontStyle="style" autoSize="false" textAlign="5" fontSize="12" offset="14,6,5,5"
		iconSize="16,16" animate="true" id="iconbutton" />

		<!-- 默认界面背景 -->
		<UIImgBox align="5" img="img_defaultbg" imgStyle="3" />

		<UIProgressBar skin="skin_process" frame="0,b_8,=,8" max="100" value="0" name="process" visible="false" />

		<!-- 标题栏 -->
		<UIBlurLayer frame="0,0,=,30" blur="5" bgColor="255,255,255,200" name="titlebar">
			<UIButton frame="r_32,0,33,30" skin="skin_btnclose" animate="true" aniAlphaType="true" name="title_close" />
			<UIButton frame="_32,0,33,30" skin="skin_btnmini" animate="true" aniAlphaType="true" name="title_minisize" />
			<UILabel pos="8,8" text=")" + m_wndTitle + LR"(" name="title_label" />
		</UIBlurLayer>
		<UIControl bgColor="150,150,150,255" frame="0,30,=,1" />

		<!-- 语音转换组 -->
		<UIBlurLayer frame="40,235,542,491" prop="group">
			<UIImgBox frame="20,20,22,22" img="icon_wave" />
			<UILabel fontStyle="style" pos="51,22" text="语音转换" />
			<IconButton prop="iconbutton" frame="406,18,115,25" resIcon="icon_begin" text="开始转换语音" name="tts_begin" />
			<IconButton prop="iconbutton" frame="330,18,70,25" resIcon="icon_begin" text="清理" name="clean_begin" />
			<UIEditBox frame="22,54,498,416" skin="skin_editbox" skin1="skin_scroll" wordWrap="true" multiline="true"
			scroll="true" inset="10,10,10,10" name="wave_editbox" />
		</UIBlurLayer>

		<!-- 模型列表组 -->
		<UIBlurLayer frame="40,75,542,126" prop="group">
			<UIImgBox frame="20,20,22,22" img="icon_modlist" />
			<UILabel fontStyle="style" pos="51,22" text="模型列表" />
			<UIComBox prop="comstyle" frame="20,50,241,25" text=" 模型选择" name="modlist_main" />
			<UILabel fontStyle="style" pos="=,b+12" text="角色选择:" />
			<UIComBox prop="comstyle" frame="80,83,181,25" text=" 角色选择（仅VITS）" name="modlist_child" />
			<IconButton prop="iconbutton" frame="r+14,=,115,25" resIcon="icon_refresh" text="刷新模型列表" name="modlist_refresh" />
			<IconButton prop="iconbutton" frame="r+14,=,=,=" resIcon="icon_folder" text="打开模型目录" name="modlist_folder" />
		</UIBlurLayer>

		<!-- 音频输出组 -->
		<!-- frame不能设置为"r+32,="的相对布局位置 因为上面的Combox会插入隐藏的List导致控件树顺序改变而使用错误的坐标 -->
		<!-- 因此这个模块先使用绝对位置 这是一个界面库设计问题 暂时先不解决 但标记一下 -->
		<UIBlurLayer frame="614,75,541,650" prop="group">
			<UIImgBox frame="20,20,22,22" img="icon_micro" />
			<UILabel fontStyle="style" pos="51,22" text="音频输出" />

			<UILabel fontStyle="style" fontSize="12" pos="22,66" text="0:00" name="voice_timecur" />
			<UISlider frame="54,65,325,18" skin="skin_slider" btnSkin="skin_sliderbtn" max="100" value="0" trackInset="10,0,0,0" name="voice_prog" />
			<UILabel fontStyle="style" fontSize="12" pos="r+12,66" text="0:00" name="voice_timeall" />

			<UIButton frame="431,60,85,25" skin="skin_button" fontStyle="style" autoSize="false" textAlign="5" fontSize="12"
			animate="true" name="voice_play" text="播放or暂停" />
			<IconButton prop="iconbutton" frame="r_126,b+8,126,=" resIcon="icon_save" text="保存音频文件" name="voice_save" />

			<UIImgBox frame="26,100,18,18" img="icon_volume" />
			<UISlider frame="r+10,=,100,18" skin="skin_slider" btnSkin="skin_sliderbtn" max="100" value="80" trackInset="10,0,0,0" name="voice_volume" />
			<UILabel pos="r+14,+2" fontStyle="style" fontSize="12" text="80%" name="volume_text" />

			<UIImgBox frame="19,137,504,496" frameRound="15" frameWidth="1" frameColor="189,189,189,255" imgStyle="2" name="voice_imgbox" img="voice_default" />
			<UILabel pos="+13,+13" fontSize="14" fontColor="109,120,201,255" text="Shiroha.png" name="voice_imgname" />
		</UIBlurLayer>
		</root>
		)";
		if (!m_uiXML->CreateUIFromXML(root, xml))
		{
			throw "界面创建失败 XML代码有误";
			__debugbreak();

			return false;
		}

		return true;
	}

	void MioTTSControl::RegisterControl()
	{
		process = _uiControl->Child<UIProgressBar>(L"process");
		titlebar = _uiControl->Child<UIBlurLayer>(L"titlebar");
		title_close = _uiControl->Child<UIButton>(L"title_close");
		title_minisize = _uiControl->Child<UIButton>(L"title_minisize");
		title_label = _uiControl->Child<UILabel>(L"title_label");
		tts_begin = _uiControl->Child<IconButton>(L"tts_begin");
		wave_editbox = _uiControl->Child<UIEditBox>(L"wave_editbox");
		modlist_main = _uiControl->Child<UIComBox>(L"modlist_main");
		modlist_child = _uiControl->Child<UIComBox>(L"modlist_child");
		modlist_refresh = _uiControl->Child<IconButton>(L"modlist_refresh");
		modlist_folder = _uiControl->Child<IconButton>(L"modlist_folder");
		voice_timecur = _uiControl->Child<UILabel>(L"voice_timecur");
		voice_timeall = _uiControl->Child<UILabel>(L"voice_timeall");
		voice_play = _uiControl->Child<UIButton>(L"voice_play");
		voice_save = _uiControl->Child<IconButton>(L"voice_save");
		voice_volume = _uiControl->Child<UISlider>(L"voice_volume");
		volume_text = _uiControl->Child<UILabel>(L"volume_text");
		voice_imgbox = _uiControl->Child<UIImgBox>(L"voice_imgbox");
		voice_imgname = _uiControl->Child<UILabel>(L"voice_imgname");
		voice_prog = _uiControl->Child<UISlider>(L"voice_prog");
		clean_begin = _uiControl->Child<IconButton>(L"clean_begin");
	}

	void MioTTSControl::RegisterEvent()
	{
		RegisterLeftClickEvent
		(
			L"title_close",
			[&]()
			{
				::SendMessageW((HWND)(_mainWindow->GetWindowHandle()), M_WND_CLOSE, 0, 0);
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"title_minisize",
			[&]()
			{
				::ShowWindow(hWnd, SW_MINIMIZE);
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"modlist_refresh",
			[&]()
			{
				setConsDefalut();
				loadmodel();
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"tts_begin",
			[&]()
			{
				playstat = false;
				wavPlayer->StopTrack(wavTrack);
				wavPlayer->DeleteTrack(wavTrack);
				voice_prog->SetCurValue(0);
				voice_timecur->SetAttribute(L"text", L"00:00");
				setSoundModule(false);
				std::thread inferThread(
					[&]()
					{
						clean_begin->SetEnabled(false);
						tts_begin->SetEnabled(false);
						modlist_main->SetEnabled(false);
						modlist_refresh->SetEnabled(false);
						process->SetVisible(true);
						auto _inputLens = wave_editbox->GetCurText();
						if (_inputLens.length() == 0)
						{
							MessageBox(hWnd, L"请输入要转换的字符串", L"未找到输入", MB_OK);
							InferQuit();
							return;
						}
						std::vector<std::wstring> _Lens;
						while (_inputLens[_inputLens.length() - 1] == L'\n' || _inputLens[_inputLens.length() - 1] == L'\r') {
							_inputLens.pop_back();
						}
						_inputLens += L"\n";
						std::wstring _tmpLen = L"";
						for (size_t i = 0; i < _inputLens.length(); i++) {
							if (_symbol.find(_inputLens[i]) == _symbol.end() && _inputLens[i] != L'\n' && _inputLens[i] != L'\r') {
								MessageBox(hWnd, (L"字符位置：Index[" + std::to_wstring(i + 1) + L"]").c_str(), L"不支持的字符", MB_OK);
								InferQuit();
								return;
							}
							if ((_inputLens[i] == L'\n') || (_inputLens[i] == L'\r')) {
								if (_tmpLen != L"") {
									if (charaSets.find(_tmpLen[_tmpLen.length() - 1]) != std::wstring::npos) {
										_tmpLen += L'.';
									}
									if (_tmpLen.length() > 101 && _modelType == modelType::Taco) {
										MessageBox(hWnd, (L"单句超出最大长度（100Byte）\n位于Len[" + std::to_wstring(_Lens.size()) + L"]位置").c_str(), L"转换失败", MB_OK);
										InferQuit();
										return;
									}
									else if (_tmpLen.length() > 600) {
										MessageBox(hWnd, (L"单句超出最大长度（600Byte）\n位于Len[" + std::to_wstring(_Lens.size()) + L"]位置").c_str(), L"转换失败", MB_OK);
										InferQuit();
										return;
									}
									_Lens.push_back(_tmpLen);
									_tmpLen = L"";
								}
							}
							else {
								_tmpLen += _inputLens[i];
							}
						}
						process->SetAttribute(L"max", std::to_wstring(_Lens.size()));
						_wavData.clear();
						int proc = 0;
						const auto _chara = modlist_child->GetCurSelItem();
						char tmpBuffer[20] = "";
						try
						{
							switch (_modelType)
							{
							case modelType::Taco:
								for (const std::wstring& _it : _Lens)
								{
									InferTaco2(_it);
									process->SetAttribute(L"value", std::to_wstring(proc++));
									conArr2Wav(_wavData.size(), _wavData.data(), "temp\\tmp.wav");
								}
								break;
							case modelType::Vits:
								InferVits(_Lens);
								break;
							case modelType::Vitm:
								if (_chara == -1)
								{
									MessageBox(hWnd, L"请指定一个角色", L"未指定角色", MB_OK);
									InferQuit();
									return;
								}
								InferVitsM(_Lens, modlist_child->GetCurSelItem());
								break;
							default:
								return;
							}
						}catch(std::exception& e)
						{
							MessageBox(hWnd, to_wide_string(e.what()).c_str(), L"错误", MB_ICONERROR | MB_OK);
							InferQuit();
							return;
						}
						process->SetAttribute(L"value", std::to_wstring(proc));
						Sleep(500);
						InferQuit();
						MAudio* audio = MCreateAudioWaveFromFile(GetCurrentFolder() + L"\\temp\\tmp.wav");
						wavTrack = wavPlayer->CreateTrack();
						wavPlayer->SetTrackSound(wavTrack, audio, 0);
						wavPlayer->SetTrackVolume(wavTrack, voice_volume->GetCurValue());
						voice_prog->SetMaxValue((_m_uint)(audio->GetDuration() * 1000.0));
						voice_timeall->SetAttribute(L"text", std::to_wstring((int)audio->GetDuration() / 60) + L":" + std::to_wstring((int)audio->GetDuration() % 60));
						setSoundModule(true);
					}
				);
				inferThread.detach();
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"clean_begin",
			[&]()
			{
				std::wstring Temp = wave_editbox->GetCurText();
				wave_editbox->SetCurText(getCleanerStr(Temp));
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"voice_play",
			[&]()
			{
				if(playstat)
				{
					Sleep(100);
					playstat = false;
					wavPlayer->PauseTrack(wavTrack);
				}else
				{
					wavPlayer->PlayTrack(wavTrack);
					playstat = true;
					std::thread playThread([&]()
						{
							_m_uint max = voice_prog->GetMaxValue() - 100;
							while (playstat)
							{
								Sleep(100);
								auto pos = (_m_uint)(wavPlayer->GetTrackPlaybackPos(wavTrack) * 1000.0f);
								if (pos > max)
								{
									playstat = false;
									wavPlayer->StopTrack(wavTrack);
									voice_prog->SetCurValue(0);
									voice_timecur->SetAttribute(L"text", L"00:00");
									break;
								}
								voice_prog->SetCurValue(pos);
								voice_timecur->SetAttribute(L"text", std::to_wstring(pos / 60000) + L":" + std::to_wstring(pos / 1000 % 60));
							}
						});
					playThread.detach();
				}
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"voice_save",
			[&]()
			{
				TCHAR  szFileName[MAX_PATH] = { 0 };
				TCHAR szTitleName[MAX_PATH] = { 0 };
				TCHAR szFilter[] = TEXT("波形音频 (*.wav)\0*.wav\0") \
					TEXT("所有文件 (*.*)\0*.*\0\0");
				OPENFILENAME  ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFileTitle = szTitleName;
				ofn.nMaxFileTitle = MAX_PATH;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrDefExt = TEXT("txt");
				ofn.lpstrTitle = NULL;
				ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = NULL;
				if (GetOpenFileName(&ofn))
				{
					CopyFile((GetCurrentFolder() + L"\\temp\\tmp.wav").c_str(), szFileName, true);
				}
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"modlist_folder",
			[]()
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(STARTUPINFO));
				ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
				si.cb = sizeof(STARTUPINFO);
				CreateProcess(nullptr,
					const_cast<LPWSTR>((L"Explorer.exe " + GetCurrentFolder() + L"\\Mods").c_str()),
					nullptr,
					nullptr,
					TRUE,
					CREATE_NO_WINDOW,
					nullptr,
					nullptr,
					&si,
					&pi);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				return true;
			}
		);
		RegisterValEvent
		(
			L"voice_volume",
			[&](uint64_t _val)
			{
				volume_text->SetAttribute(L"text", std::to_wstring(_val) + L"%");
				wavPlayer->SetTrackVolume(wavTrack, voice_volume->GetCurValue());
				return true;
			}
		);
		RegisterValEvent
		(
			L"modlist_main",
			[&](uint64_t _index)
			{
				ClearModel();
				if (_index == modlist_main->GetItemListCount() - 1)
				{
					voice_imgname->SetAttribute(L"text", L"当前未选中模型");
					voice_imgbox->SetImage(m_uiRes->ReadResource(L"voice_default", m_resKey, DataRes).res);
					return true;
				}
				std::wstring _path = GetCurrentFolder() + L"\\Mods\\" + _models[_index].Folder + L"\\" + _models[_index].Folder;
				if (_models[_index].Type == L"Tacotron2")
				{
					try
					{
						env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
						session_options = new Ort::SessionOptions;
						session_options->SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency() / 2) + 1);
						session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
						sessionEncoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
						sessionDecoderIter = new Ort::Session(*env, (_path + L"_decoder_iter.onnx").c_str(), *session_options);
						sessionPostNet = new Ort::Session(*env, (_path + L"_postnet.onnx").c_str(), *session_options);
						sessionGan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan.onnx").c_str(), *session_options);
						memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, L"加载模型失败，可能是配置文件有误或模型有误", L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					_modelType = modelType::Taco;
				}
				else if (_models[_index].Type == L"VITS_LJS")
				{
					/*
					try
					{
						env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
						session_options = new Ort::SessionOptions;
						session_options->SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency() / 2) + 1);
						session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
						memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
						sessionDec = new Ort::Session(*env, (_path + L"_dec.onnx").c_str(), *session_options);
						sessionDp = new Ort::Session(*env, (_path + L"_dp.onnx").c_str(), *session_options);
						sessionEnc_p = new Ort::Session(*env, (_path + L"_enc_p.onnx").c_str(), *session_options);
						sessionFlow = new Ort::Session(*env, (_path + L"_flow.onnx").c_str(), *session_options);
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, L"加载模型失败，可能是配置文件有误或模型有误", L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					*/
					FILE* test = nullptr;
					test = _wfopen((_path + L"_solo.pt").c_str(), L"r");
					if (!test)
					{
						MessageBox(hWnd, L"未找到模型文件", L"错误", MB_OK);
						return true;
					}
					fclose(test);
					_modelType = modelType::Vits;
				}
				else if (_models[_index].Type == L"VITS_VCTK")
				{
					/*
					try
					{
						env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
						session_options = new Ort::SessionOptions;
						session_options->SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency() / 2) + 1);
						session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
						memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
						sessionDec = new Ort::Session(*env, (_path + L"_dec.onnx").c_str(), *session_options);
						sessionDp = new Ort::Session(*env, (_path + L"_dp.onnx").c_str(), *session_options);
						sessionEnc_p = new Ort::Session(*env, (_path + L"_enc_p.onnx").c_str(), *session_options);
						sessionFlow = new Ort::Session(*env, (_path + L"_flow.onnx").c_str(), *session_options);
						sessionEmb = new Ort::Session(*env, (_path + L"_emb.onnx").c_str(), *session_options);
						for (auto it : _models[_index].Speakers)
						{
							auto _tmpItem = new ListItem;
							_tmpItem->SetText(L"  " + it);
							modlist_child->AddItem(_tmpItem, -1);
						}
						modlist_child->SetEnabled(true);
						modlist_child->SetCurSelItem(0);
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, L"加载模型失败，可能是配置文件有误或模型有误", L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					_modelType = modelType::Vitm;
					*/
					FILE* test = nullptr;
					test = _wfopen((_path + L"_mul.pt").c_str(), L"r");
					if (!test)
					{
						MessageBox(hWnd, L"未找到模型文件", L"错误", MB_OK);
						return true;
					}
					fclose(test);
					for (auto it : _models[_index].Speakers)
					{
						auto _tmpItem = new ListItem;
						_tmpItem->SetText(L"  " + it);
						modlist_child->AddItem(_tmpItem, -1);
					}
					modlist_child->SetEnabled(true);
					modlist_child->SetCurSelItem(0);
					_modelType = modelType::Vitm;
				}
				else
				{
					modlist_main->DeleteItem(static_cast<int>(_index));
					MessageBox(hWnd, L"模型配置文件有误", L"错误！", MB_OK);
					_models.erase(_models.begin() + static_cast<int>(_index));
					return true;
				}
				voice_imgname->SetAttribute(L"text", L"当前模型："+ _models[_index].Name);
				voice_imgbox->SetImage(m_uiRes->ReadFiles(_path + L".png"));
				int64 indexS = 0;
				for (auto it : _models[_index].Symbol)
					_symbol.insert(std::pair<wchar_t, int64>(it, indexS++));
				tts_begin->SetEnabled(true);
				clean_begin->SetEnabled(true);
				_curCleaner = _models[_index].Cleaner;
				return true;
			}
		);
		/*
		RegisterValEvent
		(
			L"voice_prog",
			[&](uint64_t _index)
			{
				voice_timecur->SetAttribute(L"text", std::to_wstring(_index / 60000) + L":" + std::to_wstring(_index/ 1000 % 60));
				return true;
			}
		);
		 */
	}

	void MioTTSControl::setConsDefalut() const
	{
		voice_imgname->SetAttribute(L"text", L"当前未选中模型");
		voice_imgbox->SetImage(m_uiRes->ReadResource(L"voice_default", m_resKey, DataRes).res);
		tts_begin->SetEnabled(false);
		clean_begin->SetEnabled(false);
		voice_play->SetEnabled(false);
		voice_save->SetEnabled(false);
		voice_prog->SetEnabled(false);
		modlist_child->SetEnabled(false);
	}

	void MioTTSControl::loadmodel()
	{
		ClearModel();
		std::wstring prefix = L"Cleaners";
		if (_waccess(prefix.c_str(), 0) == -1)
			int returnValueMkdir = _wmkdir(prefix.c_str());

		_wfinddata_t file_info;
		std::wstring current_path = GetCurrentFolder() + L"\\Mods";
		intptr_t handle = _wfindfirst((current_path + L"\\*.mod").c_str(), &file_info);
		if (-1 == handle) {
			MessageBox(hWnd, L"未找到有效的模型定义文件", L"载入失败", MB_OK);
			return;
		}
		voice_imgname->SetAttribute(L"text", L"当前未选中模型");
		voice_imgbox->SetImage(m_uiRes->ReadResource(L"voice_default", m_resKey, DataRes).res);
		std::string modInfo;
		modelInfo _temp;
		if (modlist_main->GetCurSelItem() != -1)
			modlist_main->SetCurSelItem(-1);
		modlist_main->DeleteAllItem();
		_models.clear();
		do
		{
			std::ifstream modfile((current_path + L"\\" + file_info.name).c_str());
			_temp.Speakers.clear();
			if (std::getline(modfile, modInfo) && modInfo.find("Folder:") == 0)
				_temp.Folder = to_wide_string(modInfo.substr(7));
			else 
				continue;
			if (std::getline(modfile, modInfo) && modInfo.find("Name:") == 0)
				_temp.Name = to_wide_string(modInfo.substr(5));
			else 
				continue;
			if (std::getline(modfile, modInfo) && modInfo.find("Type:") == 0)
				_temp.Type = to_wide_string(modInfo.substr(5));
			else
				continue;
			if (std::getline(modfile, modInfo) && modInfo.find("Symbol:") == 0)
				_temp.Symbol = to_wide_string(modInfo.substr(7));
			else 
				continue;
			if (std::getline(modfile, modInfo) && modInfo.find("Cleaner:") == 0)
				_temp.Cleaner = to_wide_string(modInfo.substr(8));
			else
				continue;
			while (std::getline(modfile, modInfo))
			{
				_temp.Speakers.push_back(to_wide_string(modInfo));
			}
			auto _tmpItem = new ListItem;
			auto _tmpText = L"  " + _temp.Type + L":" + _temp.Name;
			if (_tmpText.size() > 30)
				_tmpText = _tmpText.substr(0, 30)+L"...";
			_tmpItem->SetText(_tmpText);
			modlist_main->AddItem(_tmpItem, -1);
			_models.push_back(_temp);
		} while (!_wfindnext(handle, &file_info));
		auto _tmpItem = new ListItem;
		_tmpItem->SetText(L"  未选中");
		modlist_main->AddItem(_tmpItem, -1);
	}

	void MioTTSControl::InitControl()
	{
		RegisterControl();
		RegisterEvent();
		setConsDefalut();
		loadmodel();
	}

	void MioTTSControl::ClearModel()
	{
		tts_begin->SetEnabled(false);
		clean_begin->SetEnabled(false);
		_symbol.clear();
		modlist_child->DeleteAllItem();
		modlist_child->SetEnabled(false);
		delete sessionEncoder;
		delete sessionDecoderIter;
		delete sessionPostNet;
		delete sessionGan;
		delete env;
		delete session_options;
		delete memory_info;
		sessionEncoder = nullptr;
		sessionDecoderIter = nullptr;
		sessionPostNet = nullptr;
		sessionGan = nullptr;
		/*
		delete sessionDec;
		delete sessionDp;
		delete sessionEnc_p;
		delete sessionFlow;
		delete sessionEmb;
		sessionDec = nullptr;
		sessionDp = nullptr;
		sessionEnc_p = nullptr;
		sessionFlow = nullptr;
		sessionEmb = nullptr;
		*/
		env = nullptr;
		session_options = nullptr;
		memory_info = nullptr;
	}

	void MioTTSControl::InferQuit() const
	{
		modlist_refresh->SetEnabled(true);
		modlist_main->SetEnabled(true);
		tts_begin->SetEnabled(true);
		clean_begin->SetEnabled(true);
		process->SetVisible(false);
		process->SetAttribute(L"value", std::to_wstring(0));
	}

	void MioTTSControl::InferTaco2(const std::wstring& _input)
	{
		std::vector<void*> _ptrs;
		auto text = (int64*)malloc(_input.length() * sizeof(int64));
		if (text == nullptr) return;
		for (size_t i = 0; i < _input.length(); i++)
			*(text + i) =_symbol[_input[i]];
		int64 textLength[] = { static_cast<int64>(_input.length()) };
		std::vector<MTensor> outputTensors;
		std::vector<MTensor> inputTensors;
		int64 inputShape1[2] = { 1,textLength[0] };
		int64 inputShape2[1] = { 1 };
		inputTensors.push_back(MTensor::CreateTensor<int64>(
			*memory_info, text, textLength[0], inputShape1, 2));
		inputTensors.push_back(MTensor::CreateTensor<int64>(
			*memory_info, textLength, 1, inputShape2, 1));
		_ptrs.push_back(text);
		outputTensors = sessionEncoder->Run(Ort::RunOptions{ nullptr },
		inputNodeNamesSessionEncoder.data(),
		inputTensors.data(),
		inputTensors.size(),
		outputNodeNamesSessionEncoder.data(),
		outputNodeNamesSessionEncoder.size());
		
		inputTensors = initDecoderInputs(outputTensors, *memory_info, _ptrs);
		int32_t notFinished = 1;
		int32_t melLengths = 0;
		std::vector<MTensor> melGateAlig;
		melGateAlig.push_back(STensor::getZero<float>({ 1i64 }, *memory_info, _ptrs));
		melGateAlig.push_back(STensor::getZero<float>({ 1i64 }, *memory_info, _ptrs));
		melGateAlig.push_back(STensor::getZero<float>({ 1i64 }, *memory_info, _ptrs));
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
			}
			if (firstIter) {
				melGateAlig[0] = STensor::unsqueezeCopy<float>(outputTensors[0], 2, 1, *memory_info, _ptrs);
				melGateAlig[1] = STensor::unsqueezeCopy<float>(outputTensors[1], 2, 1, *memory_info, _ptrs);
				melGateAlig[2] = STensor::unsqueezeCopy<float>(outputTensors[6], 2, 1, *memory_info, _ptrs);
				firstIter = false;
			}
			else {
				melGateAlig[0] = STensor::cats(melGateAlig[0], STensor::unsqueezeNoCopy<float>(outputTensors[0], 2, 1, *memory_info), *memory_info, _ptrs);
				melGateAlig[1] = STensor::cats(melGateAlig[1], STensor::unsqueezeNoCopy<float>(outputTensors[1], 2, 1, *memory_info), *memory_info, _ptrs);
				melGateAlig[2] = STensor::cats(melGateAlig[2], STensor::unsqueezeNoCopy<float>(outputTensors[6], 2, 1, *memory_info), *memory_info, _ptrs);
			}
			MTensor decTmp = STensor::unsqueezeNoCopy<float>(outputTensors[1], 2, 1, *memory_info);
			MTensor dec = STensor::leCpp(STensor::sigmoid<float>(decTmp, *memory_info, _ptrs), gateThreshold, *memory_info, _ptrs);
			bool decInt = dec.GetTensorData<bool>()[0];
			notFinished = decInt * notFinished;
			melLengths += notFinished;
			if (!notFinished) {
				break;
			}
			else if (melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape()[2] >= maxDecoderSteps) {
				for (auto it : _ptrs)
					free(it);
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
		std::vector<int64> melInputShape = melGateAlig[0].GetTensorTypeAndShapeInfo().GetShape();
		std::vector<MTensor> melInput;
		melInput.push_back(MTensor::CreateTensor<float>(
			*memory_info, melGateAlig[0].GetTensorMutableData<float>(), melInputShape[0] * melInputShape[1] * melInputShape[2], melInputShape.data(), melInputShape.size()));
		outputTensors = sessionPostNet->Run(Ort::RunOptions{ nullptr },
			inputNodeNamesSessionPostNet.data(),
			melInput.data(),
			melInput.size(),
			outputNodeNamesSessionPostNet.data(),
			outputNodeNamesSessionPostNet.size());
		std::vector<MTensor> wavOuts;
		wavOuts = sessionGan->Run(Ort::RunOptions{ nullptr },
			ganIn.data(),
			outputTensors.data(),
			outputTensors.size(),
			ganOut.data(),
			ganOut.size());
		std::vector<int64> wavOutsSharp = wavOuts[0].GetTensorTypeAndShapeInfo().GetShape();
		auto TempVecWav = static_cast<int16_t*>(malloc(sizeof(int16_t) * wavOutsSharp[2]));
		if (TempVecWav == nullptr) return;
		for (int i = 0; i < wavOutsSharp[2]; i++) {
			*(TempVecWav + i) = static_cast<int16_t>(wavOuts[0].GetTensorData<float>()[i] * 32768.0f);
		}
		_wavData.insert(_wavData.end(), TempVecWav, TempVecWav + (wavOutsSharp[2]));
		free(TempVecWav);
		for (auto it : _ptrs)
			free(it);
	}

	void MioTTSControl::InferVits(const std::vector<std::wstring>& _input)
	{
		std::wstring commandLine = L"model " + GetCurrentFolder() + L"\\Mods\\" + _models[modlist_main->GetCurSelItem()].Folder + L"\\" + _models[modlist_main->GetCurSelItem()].Folder + L"_solo.pt";
		for(auto& it : _input)
		{
			commandLine += L" line";
			for (auto cha : it)
				commandLine += L" 0 " + std::to_wstring(_symbol[cha]);
			commandLine += L" 0 endline";
		}
		commandLine += L" endinfer end end";

		SECURITY_ATTRIBUTES saAttr;
		HANDLE g_hChildStd_OUT_Rd, g_hChildStd_OUT_Wr, g_hChildStd_IN_Rd, g_hChildStd_IN_Wr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		{
			throw std::exception("error code 1");
			return;
		}
		if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		{
			throw std::exception("error code 1");
			return;
		}
		if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		{
			throw std::exception("error code 1");
			return;
		}
		if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		{
			throw std::exception("error code 1");
			return;
		}

		PROCESS_INFORMATION piProcInfo;
		STARTUPINFO siStartInfo;
		bool bSuccess = false;
		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = g_hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		siStartInfo.hStdInput = g_hChildStd_IN_Rd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
		std::wstring commandStr = L"\"bins\\Vits.exe\"";

		bSuccess = CreateProcess(NULL,
			const_cast<LPWSTR>(commandStr.c_str()),
			NULL,
			NULL,
			TRUE,
			CREATE_NO_WINDOW,//CREATE_NO_WINDOW
			NULL,
			NULL,
			&siStartInfo,
			&piProcInfo);
		if (!bSuccess) {
			CloseHandle(g_hChildStd_OUT_Rd);
			CloseHandle(g_hChildStd_OUT_Wr);
			CloseHandle(g_hChildStd_IN_Rd);
			CloseHandle(g_hChildStd_IN_Wr);
			return;
		}
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
		std::string Temp = to_byte_string(commandLine);
		DWORD dwWritten;
		bSuccess = WriteFile(g_hChildStd_IN_Wr, Temp.c_str(), Temp.length() * 2 + 1, &dwWritten, NULL);
		if (!bSuccess) {
			throw std::exception("error code 0");
			return;
		}

		char ReadBuff[10] = "";
		DWORD bytesRead;
		int proc = 1;
		while (ReadFile(g_hChildStd_OUT_Rd, ReadBuff, 9, &bytesRead, NULL))
		{
			ReadBuff[bytesRead] = '\0';
			if(std::string(ReadBuff)=="endofline")
			{
				process->SetAttribute(L"value", std::to_wstring(proc++));
				continue;
			}
			if (std::string(ReadBuff) == "endofinfe")
			{
				break;
			}
		}
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_IN_Wr);
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}

	void MioTTSControl::InferVitsM(const std::vector<std::wstring>& _input,int chara)
	{
		/*
		std::vector<void*> _ptrs;
		std::vector<int64> text;
		for (auto it : _input)
		{
			text.push_back(0);
			text.push_back(_symbol[it]);
		}
		text.push_back(0);
		int64 textLength[] = { (int64)text.size() };
		std::vector<MTensor> outputTensors;
		std::vector<MTensor> inputTensors;
		int64 inputShape1[2] = { 1,textLength[0] };
		int64 inputShape2[1] = { 1 };
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
			MessageBoxA(hWnd, e.what(), "", MB_OK);
			return;
		}
		MTensor x = STensor::copy<float>(outputTensors[0], *memory_info, _ptrs), m_p = STensor::copy<float>(outputTensors[1], *memory_info, _ptrs), logs_p = STensor::copy<float>(outputTensors[2], *memory_info, _ptrs), x_mask = STensor::copy<float>(outputTensors[3], *memory_info, _ptrs);
		std::vector<MTensor> inputSid;
		std::vector<MTensor> outputG;
		int64 Character[1] = { chara };
		inputSid.push_back(MTensor::CreateTensor<int64>(
			*memory_info, Character, 1, inputShape2, 1));
		outputG = sessionEmb->Run(Ort::RunOptions{ nullptr },
			EMBInput.data(),
			inputSid.data(),
			inputSid.size(),
			EMBOutput.data(),
			EMBOutput.size());
		inputTensors.clear();
		MessageBoxA(hWnd, std::to_string(x.GetTensorData<float>()[0]).c_str(), "", MB_OK);
		inputTensors.push_back(std::move(outputTensors[0]));
		inputTensors.push_back(std::move(outputTensors[3]));
		MTensor g = STensor::unsqueezeCopy<float>(outputG[0], 2, 1, *memory_info, _ptrs);
		inputTensors.push_back(STensor::unsqueezeCopy<float>(outputG[0], 2, 1, *memory_info, _ptrs));
		MessageBoxA(hWnd, std::to_string(g.GetTensorData<float>()[0]).c_str(), "", MB_OK);
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
			MessageBoxA(hWnd, e.what(), "", MB_OK);
			return;
		}
		MessageBox(hWnd, std::to_wstring(outputTensors[0].GetTensorMutableData<float>()[0]).c_str(), L"", MB_OK);
		//MessageBox(hWnd, std::to_wstring(outputG[0].GetTensorMutableData<float>()[0]).c_str(), L"", MB_OK);
		//MessageBox(hWnd, std::to_wstring(outputTensors[0].GetTensorMutableData<float>()[0]).c_str(), L"", MB_OK);
		for (auto it : _ptrs)
			free(it);
		*/
		//以上为失败的Onnx移植（DP始终有不支持的东西）
		std::wstring commandLine = L"model " + GetCurrentFolder() + L"\\Mods\\" + _models[modlist_main->GetCurSelItem()].Folder + L"\\" + _models[modlist_main->GetCurSelItem()].Folder + L"_mul.pt";
		for (auto& it : _input)
		{
			commandLine += L" line";
			for (auto cha : it)
				commandLine += L" 0 " + std::to_wstring(_symbol[cha]);
			commandLine += L" 0 endline" + std::to_wstring(chara);
		}
		commandLine += L" endinfer end end";

		SECURITY_ATTRIBUTES saAttr;
		HANDLE g_hChildStd_OUT_Rd, g_hChildStd_OUT_Wr, g_hChildStd_IN_Rd, g_hChildStd_IN_Wr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		{
			MessageBox(hWnd, L"创建管道失败", L"错误", MB_OK);
			return;
		}
		if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		{
			MessageBox(hWnd, L"创建管道失败", L"错误", MB_OK);
			return;
		}
		if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		{
			MessageBox(hWnd, L"创建管道失败", L"错误", MB_OK);
			return;
		}
		if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		{
			MessageBox(hWnd, L"创建管道失败", L"错误", MB_OK);
			return;
		}

		PROCESS_INFORMATION piProcInfo;
		STARTUPINFO siStartInfo;
		bool bSuccess = false;
		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = g_hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		siStartInfo.hStdInput = g_hChildStd_IN_Rd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
		std::wstring commandStr = L"\"bins\\Vits.exe\"";

		bSuccess = CreateProcess(NULL,
			const_cast<LPWSTR>(commandStr.c_str()),
			NULL,
			NULL,
			TRUE,
			CREATE_NO_WINDOW,//CREATE_NO_WINDOW
			NULL,
			NULL,
			&siStartInfo,
			&piProcInfo);
		if (!bSuccess) {
			CloseHandle(g_hChildStd_OUT_Rd);
			CloseHandle(g_hChildStd_OUT_Wr);
			CloseHandle(g_hChildStd_IN_Rd);
			CloseHandle(g_hChildStd_IN_Wr);
			return;
		}
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
		std::string Temp = to_byte_string(commandLine);
		DWORD dwWritten;
		bSuccess = WriteFile(g_hChildStd_IN_Wr, Temp.c_str(), Temp.length() * 2 + 1, &dwWritten, NULL);
		if (!bSuccess) {
			MessageBox(hWnd, L"错误码0x00000000", L"错误", MB_OK);
			return;
		}

		char ReadBuff[10] = "";
		DWORD bytesRead;
		int proc = 1;
		while (ReadFile(g_hChildStd_OUT_Rd, ReadBuff, 9, &bytesRead, NULL))
		{
			ReadBuff[bytesRead] = '\0';
			if (std::string(ReadBuff) == "endofline")
			{
				process->SetAttribute(L"value", std::to_wstring(proc++));
				continue;
			}
			if (std::string(ReadBuff) == "endofinfe")
			{
				break;
			}
		}
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_IN_Wr);
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}

	std::wstring MioTTSControl::getCleanerStr(std::wstring& input) const {
		std::wstring output = L"";
		SECURITY_ATTRIBUTES sa;
		HANDLE hRead, hWrite;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		if (!CreatePipe(&hRead, &hWrite, &sa, 0) || !SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
		{
			return input;
		}
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		si.cb = sizeof(STARTUPINFO);
		si.hStdError = hWrite;
		si.hStdOutput = hWrite;
		si.dwFlags |= STARTF_USESTDHANDLES;
		std::wstring commandStr = L"";
		if (_models[modlist_main->GetCurSelItem()].Cleaner == L"japanese_g2p.exe") {
			commandStr = L"\"cleaners\\"
				+ _models[modlist_main->GetCurSelItem()].Cleaner
				+ L"\" -rsa \""
				+ input
				+ L"\"";
		}
		else {
			commandStr = L"\"cleaners\\"
				+ _models[modlist_main->GetCurSelItem()].Cleaner
				+ L"\" \""
				+ input
				+ L"\"";
		}

		auto ProcessSta = CreateProcess(NULL,
			const_cast<LPWSTR>(commandStr.c_str()),
			NULL,
			NULL,
			TRUE,
			CREATE_NO_WINDOW,
			NULL,
			NULL,
			&si,
			&pi);
		if (!ProcessSta) {
			CloseHandle(hWrite);
			CloseHandle(hRead);
			return input;
		}
		CloseHandle(hWrite);
		char ReadBuff[2049] = "";
		DWORD bytesRead;
		while (ReadFile(hRead, ReadBuff, 2048, &bytesRead, NULL))
		{
			ReadBuff[bytesRead] = '\0';
			output += to_wide_string(ReadBuff);
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(hRead);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return output;
	}
}
#undef val