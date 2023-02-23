/*
* file: Helper.h
* info: 应用程序WINAPI助手
*
* Author: Maplespe(mapleshr@icloud.com)
*
* date: 2022-9-19 Create
*/
#pragma once
#include "..\framework.h"


//获取当前应用程序所在目录
extern std::wstring GetCurrentFolder();

//获取当前窗口所在显示器DPI
extern bool GetWNdMonitorDPI(HWND hWNd, UINT& dpiX, UINT& dpiY);