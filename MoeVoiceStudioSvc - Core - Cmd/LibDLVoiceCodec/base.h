#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#define LibDLVoiceCodecBegin namespace libdlvcodec {
#define LibDLVoiceCodecEnd }
#define LIBDVCND [[nodiscard]]

#define LibDLVoiceCodecThrow(message) throw std::exception((std::string("[At \"") + __FILE__ + "\" Line " + std::to_string(__LINE__) + "]\n" + (message)).c_str())

LibDLVoiceCodecBegin

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;
using byte = unsigned char;
using lpvoid = void*;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

class TensorView;
class Tensor;

const std::unordered_map<std::string, size_t> __Dtype {{"int8", 1}, { "int16", 2 }, { "int32", 4 }, { "int64", 8 },
	{ "float8", 1 }, { "float16", 2 }, { "bfloat16", 2 }, { "float32", 4 }, { "float64", 8 }, { "bool", 1 } };

template <class _Ty, class _Alloc = std::allocator<_Ty>>
using MResource = std::vector<_Ty, _Alloc>;

template<typename T>
std::ostream& operator<<(std::ostream& _Stream, const std::vector<T>& _Data)
{
	_Stream << '[';
	for (const auto& i : _Data)
		_Stream << i << ", ";
	_Stream << "]\n";
	return _Stream;
}

class FileWrapper
{
public:
	FileWrapper() = default;
	~FileWrapper()
	{
		if (file_)
			fclose(file_);
		file_ = nullptr;
	}
	FileWrapper(const FileWrapper& _Left) = delete;
	FileWrapper& operator=(const FileWrapper& _Left) = delete;
	FileWrapper(FileWrapper&& _Right) noexcept
	{
		file_ = _Right.file_;
		_Right.file_ = nullptr;
	}
	FileWrapper& operator=(FileWrapper&& _Right) noexcept
	{
		file_ = _Right.file_;
		_Right.file_ = nullptr;
		return *this;
	}
	void open(const std::wstring& _Path, const std::wstring& _Mode)
	{
#ifdef _WIN32
		_wfopen_s(&file_, _Path.c_str(), _Mode.c_str());
#else
		file_ = _wfopen(_Path.c_str(), _Mode.c_str());
#endif
	}
	operator FILE* () const
	{
		return file_;
	}
	LIBDVCND bool enabled() const
	{
		return file_;
	}
private:
	FILE* file_ = nullptr;
};

LibDLVoiceCodecEnd