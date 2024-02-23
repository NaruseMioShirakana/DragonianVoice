#include "../header/StringPreprocess.hpp"
#ifdef _WIN32
#include <Windows.h>
#else
#error
#endif

std::string to_byte_string(const std::wstring& input)
{
	std::vector<char> ByteString(input.length() * 6);
	WideCharToMultiByte(
		CP_UTF8,
		0,
		input.c_str(),
		int(input.length()),
		ByteString.data(),
		int(ByteString.size()),
		nullptr,
		nullptr
	);
	return ByteString.data();
}

std::string to_ansi_string(const std::wstring& input)
{
	std::vector<char> ByteString(input.length() * 6);
	WideCharToMultiByte(
		CP_ACP,
		0,
		input.c_str(),
		int(input.length()),
		ByteString.data(),
		int(ByteString.size()),
		nullptr,
		nullptr
	);
	return ByteString.data();
}

std::wstring to_wide_string(const std::string& input)
{
	std::vector<wchar_t> WideString(input.length() * 2);
	MultiByteToWideChar(
		CP_UTF8,
		0,
		input.c_str(),
		int(input.length()),
		WideString.data(),
		int(WideString.size())
	);
	return WideString.data();
}

std::wstring string_vector_to_string(const std::vector<std::string>& vector)
{
	std::wstring vecstr = L"[";
	for (const auto& it : vector)
		if (!it.empty())
			vecstr += L'\"' + to_wide_string(it) + L"\", ";
	if (vecstr.length() > 2)
		vecstr = vecstr.substr(0, vecstr.length() - 2);
	vecstr += L']';
	return vecstr;
}

std::wstring wstring_vector_to_string(const std::vector<std::wstring>& vector)
{
	std::wstring vecstr = L"[";
	for (const auto& it : vector)
		if (!it.empty())
			vecstr += L'\"' + it + L"\", ";
	if (vecstr.length() > 2)
		vecstr = vecstr.substr(0, vecstr.length() - 2);
	vecstr += L']';
	return vecstr;
}