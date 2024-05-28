#include "../include/Base.h"
#include <ranges>
#ifdef _WIN32
#include <Windows.h>
#else
#error
#endif
LibTTSBegin

std::string UnicodeToByte(const std::wstring& input)
{
#ifdef _WIN32
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
#else
	//TODO
#endif
}

std::string UnicodeToAnsi(const std::wstring& input)
{
#ifdef _WIN32
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
#else
	//TODO
#endif
}

std::wstring ByteToUnicode(const std::string& input)
{
#ifdef _WIN32
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
#else
	//TODO
#endif
}

FileGuard::~FileGuard()
{
	Close();
}

FileGuard::FileGuard(FileGuard&& _Right) noexcept
{
	file_ = _Right.file_;
	_Right.file_ = nullptr;
}

FileGuard& FileGuard::operator=(FileGuard&& _Right) noexcept
{
	file_ = _Right.file_;
	_Right.file_ = nullptr;
	return *this;
}

void FileGuard::Open(const std::wstring& _Path, const std::wstring& _Mode)
{
#ifdef _WIN32
	_wfopen_s(&file_, _Path.c_str(), _Mode.c_str());
#else
	file_ = _wfopen(_Path.c_str(), _Mode.c_str());
#endif
}

FileGuard::operator FILE* () const
{
	return file_;
}

bool FileGuard::Enabled() const
{
	return file_;
}

void FileGuard::Close()
{
	if (file_)
		fclose(file_);
	file_ = nullptr;
}

Value::~Value()
{
	if (DictGuf)
		gguf_free(DictGuf);
	if (DictCtx)
		ggml_free(DictCtx);
	DictGuf = nullptr;
	DictCtx = nullptr;
}

Value& Value::load(const std::wstring& _Path, bool _Strict)
{
	FileGuard DictFile;
	DictFile.Open(_Path, L"rb");
	if (!DictFile.Enabled())
		LibTTSThrow("Failed To Open File!");
	DictFile.Close();
	gguf_init_params params{ false,&DictCtx };
	DictGuf = gguf_init_from_file(UnicodeToByte(_Path).c_str(), params);
	loadData(DictCtx, _Strict);
	return *this;
}

Value& Value::save(const std::wstring& _Path)
{
	FileGuard file;
	file.Open(_Path, L"rb");
	if (!file.Enabled())
		LibTTSThrow("Failed to open file!");
	saveData(file);
	return *this;
}

void Value::loadData(const DictType& _WeightDict, bool _Strict)
{
	LibTTSNotImplementedError;
}

void Value::saveData(FileGuard& _File)
{
	LibTTSNotImplementedError;
}

Module::Module(Module* _Parent, const std::wstring& _Name)
{
	if (_Parent != nullptr)
	{
		if(!_Parent->RegName_.empty())
			RegName_ = _Parent->RegName_ + L"." + _Name;
		else
			RegName_ = _Name;
		_Parent->Layers_[RegName_] = this;
	}
	else
		RegName_ = _Name;
}

void Module::loadData(const DictType& _WeightDict, bool _Strict)
{
	for (const auto& it : Layers_ | std::views::values)
		it->loadData(_WeightDict, _Strict);
}

void Module::saveData(FileGuard& _File)
{
	for (const auto& it : Layers_ | std::views::values)
		it->saveData(_File);
}

ggml_tensor* Module::operator()(ggml_tensor*)
{
	LibTTSNotImplementedError;
}

void Module::ReCalcLayerName(const std::wstring& _Parent)
{
	std::unordered_map<std::wstring, Module*> Lay;
	for (auto i : Layers_ | std::ranges::views::values)
	{
		i->ReCalcLayerName(_Parent);
		i->RegName_ = _Parent + L"." + i->RegName_;
		Lay[i->RegName_] = i;
	}
	Layers_ = std::move(Lay);
}

std::wstring Module::DumpLayerNameInfo()
{
	std::wstring Ret;
	DumpLayerNameInfoImpl(Ret, 0);
	return Ret;
}

void Module::DumpLayerNameInfoImpl(std::wstring& _Tmp, int TabCount)
{
	for (int i = 0; i < TabCount; ++i) _Tmp += L' ';
	_Tmp += RegName_;
	if(!Layers_.empty())
	{
		if(!_Tmp.empty())
			_Tmp += L" = {";
		else
			_Tmp += L"{";
		_Tmp += '\n';
		for (auto i : Layers_ | std::ranges::views::values)
			i->DumpLayerNameInfoImpl(_Tmp, TabCount + 1);
		for (int i = 0; i < TabCount; ++i) _Tmp += L' ';
		_Tmp += L'}';
		_Tmp += '\n';
	}
	else
		_Tmp += '\n';
}

Parameter::Parameter(Module* _Parent, const std::wstring& _Name) : Module(_Parent, _Name)
{
}

Parameter::operator ggml_tensor*() const
{
	return _Weight;
}

void Parameter::loadData(const DictType& _WeightDict, bool _Strict)
{
	_Weight = ggml_get_tensor(_WeightDict, UnicodeToByte(RegName_).c_str());
	if (!_Weight)
		LibTTSThrow("Missing Weight \"" + UnicodeToByte(RegName_) + '\"');
}

void Parameter::saveData(FileGuard& _File)
{
	LibTTSNotImplementedError;
}

Sequential::Sequential(Module* _Parent, const std::wstring& _Name) : Module(_Parent, _Name)
{
}

Sequential::~Sequential()
{
	for (auto i : _Items)
		delete i;
	_Items.clear();
}

ggml_tensor* Sequential::operator()(ggml_tensor* _Input)
{
	for (auto i : _Items)
		_Input = i->operator()(_Input);
	return _Input;
}

void Sequential::Append(Module* _Module)
{
	_Module->Name() = RegName_ + L"." + std::to_wstring(_Items.size());
	_Module->ReCalcLayerName(_Module->Name());
	_Items.emplace_back(_Module);
	Layers_[_Module->Name()] = _Module;
}

Sequential& Sequential::operator=(const std::initializer_list<Module*>& _Input)
{
	for (auto i : _Input)
		Append(i);
	return *this;
}

LibTTSEnd