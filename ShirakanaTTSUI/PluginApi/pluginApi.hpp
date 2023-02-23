#pragma once

#ifndef MOESS_PLUGIN
#define MOESS_PLUGIN

#include <string>
#include <Windows.h>
class MoeSSPluginAPI
{
public:
	using funTy = const wchar_t* (*)(const wchar_t*);
	using freTy = void (*)();
	MoeSSPluginAPI() = default;
	~MoeSSPluginAPI() {
		func = nullptr;
		if (m_hDynLib)
			FreeLibrary(m_hDynLib);
		m_hDynLib = nullptr;
	}
	char Load(const std::wstring& PluginName);
	void unLoad();
	std::wstring functionAPI(const std::wstring& inputLen) const;
	MoeSSPluginAPI(const MoeSSPluginAPI&) = delete;
	MoeSSPluginAPI(MoeSSPluginAPI&&) = delete;
	MoeSSPluginAPI& operator=(MoeSSPluginAPI&& move) noexcept{
		func = move.func;
		m_hDynLib = move.m_hDynLib;
		move.func = nullptr;
		move.m_hDynLib = nullptr;
		return *this;
	}
	MoeSSPluginAPI& operator=(const MoeSSPluginAPI&) = delete;
private:
	const wchar_t*(*func)(const wchar_t*) = nullptr;
	void (*frel)() = nullptr;
	HINSTANCE m_hDynLib = nullptr;
};
#endif
