#include "pluginApi.hpp"
#ifdef WIN32
MoeSSPluginAPI::~MoeSSPluginAPI()
{
	func = nullptr;
	if (m_hDynLib)
		FreeLibrary(m_hDynLib);
	m_hDynLib = nullptr;
}

MoeSSPluginAPI& MoeSSPluginAPI::operator=(MoeSSPluginAPI&& move) noexcept
{
	func = move.func;
	m_hDynLib = move.m_hDynLib;
	move.func = nullptr;
	move.m_hDynLib = nullptr;
	return *this;
}

bool MoeSSPluginAPI::enabled() const
{
	return m_hDynLib != nullptr;
}

char MoeSSPluginAPI::Load(const std::wstring& PluginName)
{
	func = nullptr;
	frel = nullptr;
	if (m_hDynLib)
	{
		FreeLibrary(m_hDynLib);
		m_hDynLib = nullptr;
	}
	m_hDynLib = LoadLibrary((L"cleaners\\" + PluginName + L".dll").c_str());
	if (m_hDynLib == nullptr)
		return -1;
	func = reinterpret_cast<funTy>(
		reinterpret_cast<void*>(
			GetProcAddress(m_hDynLib, "PluginMain")
			)
		);
	frel = reinterpret_cast<freTy>(
		reinterpret_cast<void*>(
			GetProcAddress(m_hDynLib, "Release")
			)
		);
	if (func == nullptr)
		return 1;
	return 0;
}

std::wstring MoeSSPluginAPI::functionAPI(const std::wstring& inputLen) const
{
	const auto tmp = func(inputLen.c_str());
	std::wstring ret = tmp;
	return ret;
}

void MoeSSPluginAPI::unLoad()
{
	if (frel)
		frel();
	func = nullptr;
	frel = nullptr;
	if (m_hDynLib)
		FreeLibrary(m_hDynLib);
	m_hDynLib = nullptr;
}
#endif