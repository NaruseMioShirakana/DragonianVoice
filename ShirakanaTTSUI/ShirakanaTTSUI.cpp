/*
* file: MoeSSUI.cpp
* info: MoeSS的UI界面实现 应用程序主类
* 
* Author: Maplespe(mapleshr@icloud.com)
* AuthorInfo: NaruseMioYumemiShirakana的老婆
* date: 2022-9-19 Create
*/
#include "framework.h"
#include "ShirakanaTTSUI.h"
#define val const auto
int __stdcall wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    //初始化MiaoUI界面库
    std::wstring _error;
    if (!ttsUI::MiaoUI_Initialize(_error))
    {
        MessageBoxW(NULL, _error.c_str(), L"MiaoUI 初始化失败", MB_ICONERROR);
        return -1;
    }

    //创建主窗口和消息循环
    ttsUI::MainWindow_CreateLoop();

    //反初始化界面库
    ttsUI::MiaoUI_Uninitialize();

    return 0;
}
