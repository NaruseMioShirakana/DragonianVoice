#pragma once
#include <cstdint>
#include <filesystem>
#include <ggml-alloc.h>
#include <unordered_map>
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#define LibTTSBegin namespace libtts {
#define LibTTSEnd }
#define LIBTTSND [[nodiscard]]

#define LibTTSThrowImpl(message, exception_type) {\
	const std::string __LibTTS__Message__ = message;\
	const std::string __LibTTS__Message__Prefix__ =\
	std::string("[In File: \"") + std::filesystem::path(__FILE__).filename().string() + "\", " +\
	"Function: \"" + __FUNCSIG__ + "\", " +\
	"Line: " + std::to_string(__LINE__) + " ] ";\
	if (__LibTTS__Message__.substr(0, __LibTTS__Message__Prefix__.length()) != __LibTTS__Message__Prefix__)\
		throw exception_type((__LibTTS__Message__Prefix__ + __LibTTS__Message__).c_str());\
	throw exception_type(__LibTTS__Message__.c_str());\
}

#define LibTTSThrow(message) LibTTSThrowImpl(message, std::exception)

#define LibTTSNotImplementedError LibTTSThrow("NotImplementedError!")

#define RegisterLayer(MemberName, ...) MemberName(this, L#MemberName, __VA_ARGS__)

#define LayerItem(TypeName, ...) new TypeName(nullptr, L"", __VA_ARGS__)

#define LogMessage(message) LibTTS::GetLogger().log(message)

#define ErrorMessage(message) LibTTS::GetLogger().error(message)

LibTTSBegin

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;
using byte = unsigned char;
using lpvoid = void*;
using cpvoid = const void*;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
struct NoneType {};
static constexpr NoneType None;
using DictType = ggml_context*;

std::string UnicodeToByte(const std::wstring& input);
std::string UnicodeToAnsi(const std::wstring& input);
std::wstring ByteToUnicode(const std::string& input);

class FileGuard
{
public:
	FileGuard() = default;
	~FileGuard();
	FileGuard(const FileGuard& _Left) = delete;
	FileGuard& operator=(const FileGuard& _Left) = delete;
	FileGuard(FileGuard&& _Right) noexcept;
	FileGuard& operator=(FileGuard&& _Right) noexcept;
	void Open(const std::wstring& _Path, const std::wstring& _Mode);
	void Close();
	operator FILE* () const;
	LIBTTSND bool Enabled() const;
private:
	FILE* file_ = nullptr;
};

class Value
{
public:
	Value() = default;
	Value(const Value& _Left) = delete;
	virtual ~Value();

protected:
	std::wstring RegName_;

public:
	virtual Value& load(const std::wstring& _Path, bool _Strict = false);
	virtual Value& save(const std::wstring& _Path);
	virtual void loadData(const DictType& _WeightDict, bool _Strict = false);
	virtual void saveData(FileGuard& _File);
private:
	ggml_context* DictCtx = nullptr;
	gguf_context* DictGuf = nullptr;
};

class Module : public Value
{
public:
	Module(Module* _Parent, const std::wstring& _Name);
	~Module() override = default;
	virtual ggml_tensor* operator()(ggml_tensor*);
	std::wstring& Name() { return RegName_; }
	std::wstring DumpLayerNameInfo();

private:
	void DumpLayerNameInfoImpl(std::wstring& _Tmp, int TabCount);

protected:
	std::unordered_map<std::wstring, Module*> Layers_;

public:
	void loadData(const DictType& _WeightDict, bool _Strict) override;
	void saveData(FileGuard& _File) override;
	void ReCalcLayerName(const std::wstring& _Parent);
};

class Parameter : public Module
{
public:
	using Ty = float;
	Parameter(Module* _Parent, const std::wstring& _Name);
	~Parameter() override = default;
	operator ggml_tensor* () const;

protected:
	ggml_tensor* _Weight = nullptr;
public:
	void loadData(const DictType& _WeightDict, bool _Strict) override;
	void saveData(FileGuard& _File) override;
};

class Sequential : public Module
{
public:
	Sequential(Module* _Parent, const std::wstring& _Name);
	~Sequential() override;
	ggml_tensor* operator()(ggml_tensor* _Input) override;
	void Append(Module* _Module);
	Sequential& operator=(const std::initializer_list<Module*>& _Input);
	Module** begin() { return _Items.data(); }
	Module** end() { return _Items.data() + _Items.size(); }
	Module& operator[](int64_t _Index) const { return **(_Items.data() + _Index); }

private:
	std::vector<Module*> _Items;
};

LibTTSEnd
