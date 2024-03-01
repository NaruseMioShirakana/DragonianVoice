#include "MJson.h"

class FileGuard
{
public:
	FileGuard() = delete;
	~FileGuard()
	{
		if (_fp) fclose(_fp);
		_fp = nullptr;
	}
	FileGuard(const char* _path)
	{
		if (_fp) fclose(_fp);
		_wfopen_s(&_fp, to_wide_string(_path).c_str(), L"rb");
	}
	FileGuard(const std::wstring& _path)
	{
		if (_fp) fclose(_fp);
		_wfopen_s(&_fp, _path.c_str(), L"rb");
	}
	operator FILE* () const
	{
		return _fp;
	}
private:
	FILE* _fp = nullptr;
	static std::wstring to_wide_string(const std::string& input)
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
};

MJson::MJson(const char* _path)
{
	const auto file = FileGuard(_path);
	_document = yyjson_read_file(_path, YYJSON_READ_NOFLAG, nullptr, nullptr);
	if (!_document)
		throw std::exception("Json Parse Error !");
	root = yyjson_doc_get_root(_document);
}

MJson::MJson(const std::wstring& _path)
{
	const FileGuard fp(_path);
	_document = yyjson_read_fp(fp, YYJSON_READ_NOFLAG, nullptr, nullptr);
	if (!_document)
		throw std::exception("File Not Exists!");
	root = yyjson_doc_get_root(_document);
}

MJson::MJson(const std::string& _data, bool _read_from_string)
{
	if (_read_from_string)
		_document = yyjson_read(_data.c_str(), _data.length(), YYJSON_READ_NOFLAG);
	else
	{
		const auto file = FileGuard(_data.c_str());
		_document = yyjson_read_fp(file, YYJSON_READ_NOFLAG, nullptr, nullptr);
	}
	if (!_document)
		throw std::exception("Json Parse Error !");
	root = yyjson_doc_get_root(_document);
}
