/*
* file: MainWindow.cpp
* info: 应用程序主窗口实现 使用MiaoUI
*
* Author: Maplespe(mapleshr@icloud.com) & NaruseMioYumemiShirakana
*
* date: 2022-9-19 Create.
*/

#include "MainWindow.h"
#pragma warning(disable : 4996)
#include "..\Helper\Helper.h"
#include "../resource1.h"
#define _MNAME(x) (control->GetName() == (x))
#include <Uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
namespace ttsUI
{
	using namespace Mui;

	//界面资源
	DMResources* m_uiRes = nullptr;

	auto constexpr m_resKey = L"12345678";

	//主窗口尺寸
	const UISize m_wndSize = { 1200, 776 };
	//主窗口标题
	std::wstring m_wndTitle = L"MoeSS - a speech synthesis app";

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
				const float scaleX = (float)dpiX / 96.f;
				const float scaleY = (float)dpiY / 96.f;
				window.ScaleWindow(int((float)m_wndSize.width * scaleX), int((float)m_wndSize.height * scaleY));
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
			std::wstring errTmp;
			//窗口消息循环
			UIMessageLoop();
		}
	}

	MainWindow::~MainWindow()
	{
		delete m_uiXML;
		delete m_fontLoader;
	}

	bool MainWindow::EventProc(UINotifyEvent event, UIControl* control, _m_param param)
	{
		if (tts_layer->EventProc(event, control, param)) return true;
		if (vc_layer->EventProc(event, control, param)) return true;
		if (m_player->EventProc(event, control, param)) return true;
		switch (event)
		{
		case Event_Mouse_LClick:
			{
				if (ttsCon->_levents.find(control->GetName()) != ttsCon->_levents.end())
				{
					return (ttsCon->_levents.find(control->GetName())->second)();
				}
				break;
			}
		case Event_Mouse_LDown:
			{
				if (_MNAME(L"titlebar")) {
					::SendMessageW(ttsCon->hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
					return true;
				}
				break;
			}
		case Event_Slider_Change:
			{
				if (ttsCon->_sevents.find(control->GetName()) != ttsCon->_sevents.end())
				{
					return (ttsCon->_sevents.find(control->GetName())->second)(param);
				}
				break;
			}
		case Event_ListBox_ItemChanged:
			{
				if (ttsCon->_sevents.find(control->GetName()) != ttsCon->_sevents.end())
				{
					return (ttsCon->_sevents.find(control->GetName())->second)(param);
				}
				break;
			}
		case Event_Mouse_Wheel:
			{
				if (_MNAME(L"modlist_child"))
				{
					const auto curItem = ttsCon->modlist_child->GetCurSelItem();
					if (param < 0)
					{
						if (curItem != ttsCon->n_speakers - 1)
							ttsCon->modlist_child->SetCurSelItem(curItem + 1);
					}else
					{
						if (curItem != 0)
							ttsCon->modlist_child->SetCurSelItem(curItem - 1);
					}
					return true;
				}
			}
		default:
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
				const auto pm = (std::pair<_m_param, _m_param>*)param;

				typedef void (WINAPI* PGetNTVer)(DWORD*, DWORD*, DWORD*);
				static HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
				static auto GetNTVer = (PGetNTVer)GetProcAddress(hModule, "RtlGetNtVersionNumbers");
				DWORD Major = 0;
				GetNTVer(&Major, nullptr, nullptr);

				//win10以下版本删除边框
				if (pm->first && Major < 10)
					return 1;

				if (!pm->second)
					return UIWindowBasic::EventSource(message, param);

				const HWND hWnd = (HWND)GetWindowHandle();

				UINT dpi = 96;
				GetWNdMonitorDPI(hWnd, dpi, dpi);

				const int frameX = GetThemeSysSize(nullptr, SM_CXFRAME);
				const int frameY = GetThemeSysSize(nullptr, SM_CYFRAME);
				const int padding = GetThemeSysSize(nullptr, SM_CXPADDEDBORDER);

				/*if (dpi > 131)
					padding += (dpi - 131) / 24 + 1;

				if (dpi > 143)
				{
					auto add = (dpi - 143) / 96 + 1;
					frameX += add;
					frameY += add;
				}*/
				const auto params = (NCCALCSIZE_PARAMS*)pm->second;
				RECT* rgrc = params->rgrc;

				rgrc->right -= frameX + padding;
				rgrc->left += frameX + padding;
				rgrc->bottom -= frameY + padding;

				WINDOWPLACEMENT placement = { 0 };
				placement.length = sizeof(WINDOWPLACEMENT);
				if (GetWindowPlacement(hWnd, &placement)) {
					if (placement.showCmd == SW_SHOWMAXIMIZED)
						rgrc->top += padding;
				}

				return true;
			}
			//重新计算框架 扩展到整窗
			case WM_CREATE:
			{
				const auto rect = GetWindowRect();
				SetWindowPos((HWND)GetWindowHandle(), nullptr, rect.left, rect.top,
					rect.GetWidth(), rect.GetHeight(), SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
				return UIWindowBasic::EventSource(message, param);
			}
			//限定窗口大小
			case WM_GETMINMAXINFO:
			{
				const auto pm = (std::pair<_m_param, _m_param>*)param;
				const auto p = (MINMAXINFO*)pm->second;
				p->ptMaxSize.x = m_wndSize.width;
				p->ptMaxSize.y = m_wndSize.height;
				return true;
			}
			//响应系统DPI更改
			case WM_DPICHANGED:
			{
				const auto pm = (std::pair<_m_param, _m_param>*)param;
				const UINT dpi = M_LOWORD((_m_long)pm->first);
				const float scale = (float)dpi / 96.f;
				ScaleWindow(int(static_cast<float>(m_wndSize.width) * scale), int(static_cast<float>(m_wndSize.height) * scale));
				return 0;
			}
			//窗口即将关闭
			case WM_CLOSE:
			{
				//先执行界面库内部释放方法
				const auto ret = UIWindowBasic::EventSource(message, param);
				//最后再销毁XML类
				if (m_uiXML) {
					delete m_uiXML;
					m_uiXML = nullptr;
				}
				return ret;
			}
		default:
			return UIWindowBasic::EventSource(message, param);
		}
	}

	bool MainWindow::AfterCreated()
	{
		//窗口必须有一个根控件
		//
		const auto root = new UIControl();
		AddRootControl(root);
		//设置窗口背景色为白色
		UIBkgndStyle bgColor;
		bgColor.background = Color::M_RGBA(255, 255, 255, 255);
		root->SetBackground(bgColor);
		const HWND hWnd = (HWND)GetWindowHandle();
		HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1));
		SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

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

		const std::wstring xml = LR"(
		<root>
	    <!-- 组合框样式组 -->
	    <PropGroup textAlign="4" skin="skin_combox" skin1="skin_scroll" listSkin="skin_comlist" resIcon="icon_comlist"
	    itemSkin="skin_comlistitem" menuHeight="150" itemFontStyle="comList" barWidth="6" id="comstyle" />
	
	    <!-- 模块样式组 -->
	    <PropGroup blur="5" frameWidth="1" frameColor="165,165,165,255" frameRound="15" frameRound="15"
	    bgColor="255,255,255,180" clipRound="15" id="group" />
	
	    <!-- 图标按钮样式组 -->
	    <PropGroup skin="skin_button" fontStyle="style" autoSize="false" textAlign="5" fontSize="12" offset="14,6,5,5"
	    iconSize="16,16" animate="true" id="iconbutton" />
	
	    <!-- 默认界面背景 -->
	    <UIImgBox align="5" img="img_defaultbg" imgStyle="3" />

		<PropGroup fontStyle="style" skin="skin_checkbox" animate="true" aniAlphaType="true" textAlign="4" id="checkbox" />
	
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
	        <IconButton prop="iconbutton" frame="406,20,115,25" resIcon="icon_begin" text="开始转换语音" name="tts_begin" />
	        <IconButton prop="iconbutton" frame="300,20,95,25" resIcon="icon_plugin" text="启用插件" name="clean_begin" />
	        <UILabel fontStyle="style" fontSize="12" pos="150,24" text="种子：" />
	        <UIEditBox frame="r+5,22,102,20" skin="skin_editbox" name="tran_seeds" limitText="10" text="52468" number="true" />
	        <UILabel fontStyle="style" fontSize="12" pos="20,61" text="噪声规模：" />
	        <UIEditBox frame="r+5,58,60,20" skin="skin_editbox" name="noise_scale_box" limitText="7" text="0.667" />
	        <UILabel fontStyle="style" fontSize="12" pos="r+10,61" text="DP噪声规模：" />
	        <UIEditBox frame="r+5,58,60,20" skin="skin_editbox" name="noise_scale_w_box" limitText="7" text="0.800" />
	        <UILabel fontStyle="style" fontSize="12" pos="r+10,61" text="长度规模：" />
	        <UIEditBox frame="r+5,58,60,20" skin="skin_editbox" name="length_scale_box" limitText="7" text="1.000" />
			<UICheckBox prop="checkbox" fontSize="12" name="customPathStat" frame="r+10,60,100,18" text="自定义DP" />
	        <UIEditBox frame="20,96,502,375" skin="skin_editbox" skin1="skin_scroll" wordWrap="true" multiline="true"
			 scroll="true" inset="10,10,10,10" name="wave_editbox" />
	    </UIBlurLayer>
	
	    <!-- 模型列表组 -->
	    <UIBlurLayer frame="40,75,542,126" prop="group">
	        <UIImgBox frame="20,20,22,22" img="icon_modlist" />
	        <UILabel fontStyle="style" pos="51,22" text="模型列表" />
	        <UIComBox prop="comstyle" frame="20,50,246,25" text=" 模型选择" name="modlist_main" />
	        <UILabel fontStyle="style" pos="=,b+12" text="角色选择:" />
	        <UIComBox prop="comstyle" frame="280,50,242,25" text=" 串联模型选择（仅限VITS多角色模型）" name="modlist_child2" />
	        <UIComBox prop="comstyle" frame="90,83,176,25" text=" 角色选择" name="modlist_child" />
	        <IconButton prop="iconbutton" frame="280,=,115,25" resIcon="icon_refresh" text="刷新模型列表" name="modlist_refresh" />
	        <IconButton prop="iconbutton" frame="r+11,=,=,=" resIcon="icon_folder" text="打开模型目录" name="modlist_folder" />
	    </UIBlurLayer>
	
	    <!-- 音频输出组 -->
	    <!-- frame不能设置为"r+32,="的相对布局位置 因为上面的Combox会插入隐藏的List导致控件树顺序改变而使用错误的坐标 -->
	    <!-- 因此这个模块先使用绝对位置 这是一个界面库设计问题 暂时先不解决 但标记一下 -->
	    <UIBlurLayer frame="614,75,541,650" prop="group">
	        <UIImgBox frame="20,20,22,22" img="icon_micro" />
	        <UILabel fontStyle="style" pos="51,22" text="音频输出" />
	
	        <UILabel fontStyle="style" fontSize="12" pos="22,66" text="0:00" name="voice_timecur" />
	        <UISlider frame="54,65,325,18" skin="skin_slider" btnSkin="skin_sliderbtn" max="100" value="0" trackInset="10,0,10,0" name="voice_prog" />
	        <UILabel fontStyle="style" fontSize="12" pos="r+12,66" text="0:00" name="voice_timeall" />
	
	        <UIButton frame="431,60,85,25" skin="skin_button" fontStyle="style" autoSize="false" textAlign="5" fontSize="12"
	        animate="true" name="voice_play" text="播放or暂停" />
	        <IconButton prop="iconbutton" frame="r_126,b+8,126,=" resIcon="icon_save" text="保存音频文件" name="voice_save" />
	
	        <UIImgBox frame="26,100,18,18" img="icon_volume" />
	        <UISlider frame="r+10,=,100,18" skin="skin_slider" btnSkin="skin_sliderbtn" max="100" value="80" trackInset="10,0,10,0" name="voice_volume" />
	        <UILabel pos="r+14,+2" fontStyle="style" fontSize="12" text="80%" name="volume_text" />
	
	        <UIImgBox frame="19,137,504,496" frameRound="15" frameWidth="1" frameColor="189,189,189,255" imgStyle="2" name="voice_imgbox" img="voice_default" />
	        <UILabel pos="+13,+13" fontSize="14" fontColor="109,120,201,255" text="Shiroha.png" name="voice_imgname" />
	    </UIBlurLayer>
		</root>
		)";
		
		if (!m_uiXML->CreateUIFromXML(root, xml))
		{
			throw std::exception("xml error");
		}
		m_player = new UIAudioPlayer(this);
		tts_layer = new UISliderLayer(m_uiXML, root);
		vc_layer = new VcConfigLayer(m_uiXML, root);
		return true;
	}
}
#undef val