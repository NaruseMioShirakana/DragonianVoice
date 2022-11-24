/*
* file: Helper.h
* info: 应用程序WINAPI助手
*
* Author: Maplespe(mapleshr@icloud.com)
*
* date: 2022-9-19 Create
*/

#include "Helper.h"
#include <ShellScalingApi.h>

std::wstring GetCurrentFolder()
{
	wchar_t _path[MAX_PATH];
	GetModuleFileNameW(NULL, _path, MAX_PATH);
	std::wstring ret =_path;
	ret = ret.substr(0, ret.rfind(L"\\"));
	return ret;
}

bool GetWNdMonitorDPI(HWND hWNd, UINT& dpiX, UINT& dpiY)
{
	//Windows 8.1及以上可用
	HINSTANCE _dll = LoadLibraryW(L"SHCore.dll");
	if (_dll == nullptr) return false;

	typedef HRESULT(WINAPI* _fun)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

	_fun _GetDpiForMonitor_ = (_fun)GetProcAddress(_dll, "GetDpiForMonitor");
	if (_GetDpiForMonitor_)
	{
		if (FAILED(_GetDpiForMonitor_(MonitorFromWindow(hWNd, MONITOR_DEFAULTTONEAREST),
			MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
			goto gdiget;
		else
			return true;
	}
	//旧版系统 使用gdi获取主桌面DPI
	gdiget:
	HDC hdc = GetDC(NULL);
	if (hdc)
	{
		dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
		dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
		ReleaseDC(NULL, hdc);
		return true;
	}
	return false;
}