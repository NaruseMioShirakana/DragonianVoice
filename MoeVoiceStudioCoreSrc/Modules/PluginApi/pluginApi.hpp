#pragma once
#include <string>
#ifdef WIN32
#ifndef UNICODE
#define UNICODE
#endif
#include <Windows.h>
#endif
class MoeSSPluginAPI
{
public:
	using funTy = const wchar_t* (*)(const wchar_t*);
	using freTy = void (*)();
	MoeSSPluginAPI() = default;
	~MoeSSPluginAPI();
	char Load(const std::wstring& PluginName);
	void unLoad();
	[[nodiscard]] std::wstring functionAPI(const std::wstring& inputLen) const;
	MoeSSPluginAPI(const MoeSSPluginAPI&) = delete;
	MoeSSPluginAPI(MoeSSPluginAPI&&) = delete;
	MoeSSPluginAPI& operator=(MoeSSPluginAPI&& move) noexcept;
	[[nodiscard]] bool enabled() const;
	MoeSSPluginAPI& operator=(const MoeSSPluginAPI&) = delete;
private:
#ifdef WIN32
	const wchar_t*(*func)(const wchar_t*) = nullptr;
	void (*frel)() = nullptr;
	HINSTANCE m_hDynLib = nullptr;
#endif
};
