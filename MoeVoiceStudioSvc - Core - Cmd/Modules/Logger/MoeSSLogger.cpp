#include "MoeSSLogger.hpp"
#include <iostream>
#include "../StringPreprocess.hpp"
#include <windows.h>

namespace MoeSSLogger
{
	static Logger MoeVsLogger;

	static std::wstring GetCurrentFolder(const std::wstring& defualt = L"")
	{
		wchar_t path[1024];
#ifdef _WIN32
		GetModuleFileName(nullptr, path, 1024);
		std::wstring _curPath = path;
		_curPath = _curPath.substr(0, _curPath.rfind(L'\\'));
		return _curPath;
#else
		//TODO Other System
#error Other System ToDO
#endif
	}

	void RemoveDir(const std::filesystem::directory_entry& dir)
	{
		if (!dir.exists())
			return;
		if (dir.is_directory() && !is_empty(dir))
		{
			const std::filesystem::directory_iterator dirs(dir);
			for (const auto& i : dirs)
				RemoveDir(i);
		}
		remove(dir);
	}

	Logger::~Logger()
	{
		if (log_file)
			fclose(log_file);
		if (error_file)
			fclose(error_file);
		log_file = nullptr;
		error_file = nullptr;
	}

	Logger::Logger()
	{
		const std::wstring LogPath = GetCurrentFolder() + L"/log";
		const std::filesystem::path logger_path(LogPath);
		if (!exists(logger_path))
			create_directory(logger_path);
		const std::filesystem::directory_entry logger_path_entry(LogPath);
		if (logger_path_entry.is_directory())
		{
			time_t curtime;
			time(&curtime);
			tm nowtime{ 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			localtime_s(&nowtime, &curtime);
			const std::wstring dir_name =  L"MoeVoiceStudioLog " +
				std::to_wstring(nowtime.tm_year + 1900) + L'-' +
				std::to_wstring(nowtime.tm_mon) + L'-' +
				std::to_wstring(nowtime.tm_mday) + L'-' +
				std::to_wstring(nowtime.tm_hour) + L'-' +
				std::to_wstring(nowtime.tm_min) + L'-' +
				std::to_wstring(nowtime.tm_sec);
			cur_log_dir = LogPath + L'/' + dir_name;
			const std::filesystem::directory_iterator logger_path_list(LogPath);
			std::filesystem::directory_entry remove_dir;
			size_t log_count = 0;
			for (const auto& i : logger_path_list)
				if (i.is_directory() && i.path().filename().wstring().substr(0, 17) == L"MoeVoiceStudioLog")
				{
					if (!log_count)
						remove_dir = i;
					else if (i.last_write_time() < remove_dir.last_write_time())
						remove_dir = i;
					++log_count;
				}
			if (log_count > 10)
				RemoveDir(remove_dir);
			if (!exists(cur_log_dir))
				create_directory(cur_log_dir);
			logpath = cur_log_dir;
			logpath.append("log.txt");
			errorpath = cur_log_dir;
			errorpath.append("error.txt");

			_wfopen_s(&log_file, logpath.c_str(), L"w");
			_wfopen_s(&error_file, errorpath.c_str(), L"w");
		}
	}

	void Logger::log(const std::wstring& format) const
	{
		if (log_file)
			fprintf_s(log_file, "%s\n", to_byte_string(format).c_str());
		else
			fprintf_s(stdout, "%s\n", to_byte_string(format).c_str());
	}

	void Logger::log(const char* format) const
	{
		if (log_file)
			fprintf_s(log_file, "%s\n", format);
		else
			fprintf_s(stdout, "%s\n", format);
	}

	void Logger::error(const std::wstring& format) const
	{
		if (log_file)
			fprintf(log_file, "[ERROR]%s\n", to_byte_string(format).c_str());
		if (error_file)
			fprintf(error_file, "[ERROR]%s\n", to_byte_string(format).c_str());
	}

	void Logger::error(const char* format) const
	{
		if (log_file)
			fprintf(log_file, "[ERROR]%s\n", format);
		if (error_file)
			fprintf(error_file, "[ERROR]%s\n", format);
	}

	Logger& GetLogger()
	{
		return MoeVsLogger;
	}
}
