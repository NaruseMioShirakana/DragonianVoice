#pragma once

#include <vector>
#include <string>
#include "../framework.h"

LibSvcApi std::string to_byte_string(const std::wstring& input);

LibSvcApi std::string to_ansi_string(const std::wstring& input);

LibSvcApi std::wstring to_wide_string(const std::string& input);

LibSvcApi std::wstring string_vector_to_string(const std::vector<std::string>& vector);

LibSvcApi std::wstring wstring_vector_to_string(const std::vector<std::wstring>& vector);

template <typename T>
std::wstring vector_to_string(const std::vector<T>& vector)
{
	std::wstring vecstr = L"[";
	for (const auto& it : vector)
	{
		std::wstring TmpStr = std::to_wstring(it);
		if ((std::is_same_v<T, float> || std::is_same_v<T, double>) && TmpStr.find(L'.') != std::string::npos)
		{
			while (TmpStr.back() == L'0')
				TmpStr.pop_back();
			if (TmpStr.back() == L'.')
				TmpStr += L"0";
		}
		vecstr += TmpStr + L", ";
	}
	if (vecstr.length() > 2)
		vecstr = vecstr.substr(0, vecstr.length() - 2);
	vecstr += L']';
	return vecstr;
}