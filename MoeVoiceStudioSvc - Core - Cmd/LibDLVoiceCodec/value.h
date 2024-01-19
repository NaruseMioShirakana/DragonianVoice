#pragma once
#include <cassert>
#include <vector>
#include "base.h"

#define RegLayer(ModuleName, MemberName, ...) ModuleName MemberName{this, #MemberName, __VA_ARGS__}

LibDLVoiceCodecBegin

struct WeightData
{
	size_t Size = 0;
	size_t ShapeSize = 0;
	std::vector<int64_t> Shape;
	MResource<byte> Data;
};

class Value
{
public:
	Value() = default;
	Value(const Value& _Left) = delete;
	virtual ~Value() = default;
	using WeightDict = std::unordered_map<std::string, WeightData>;

protected:
	std::string RegName_;

public:
	Value& load(const std::wstring& _Path);
	Value& save(const std::wstring& _Path);
	virtual void loadData(WeightDict& _Dict);
	virtual void saveData(FileWrapper& _File);
};

class Module : public Value
{
public:
	Module(Module* _Parent, const std::string& _Name)
	{
		if (_Parent != nullptr)
		{
			RegName_ = _Parent->RegName_ + "." + _Name;
			_Parent->Layers_[RegName_] = this;
		}
		else
			RegName_ = _Name;
	}
	~Module() override = default;
private:
	std::unordered_map<std::string, Value*> Layers_;

public:
	void loadData(WeightDict& _Dict) override;
	void saveData(FileWrapper& _File) override;
};

class TensorData : public Value
{
public:
	TensorData() = default;
	TensorData(const TensorData& _Left) = delete;
	TensorData(TensorData&& _Right) = delete;
	~TensorData() override = default;

protected:
	std::vector<int64_t> Shape_;
	std::vector<uint64_t> Step_;
	std::vector<int64_t> Transopse_;
	size_t DTypeAligBytes_ = 4;
	std::string Type_ = "float32";
	bool MemOwner_ = false;

public:
	LIBDVCND const std::string& dtype() const { return Type_; }
	LIBDVCND const std::vector<int64_t>& shape() const { return Shape_; }
	LIBDVCND size_t size() const {
		if (Shape_.empty()) return 0;
		return Shape_[0];
	}
	LIBDVCND size_t total_size() const {
		if (Shape_.empty()) return 0;
		size_t ttsize = 1;
		for (const auto i : Shape_)
			ttsize *= i;
		return ttsize;
	}
	LIBDVCND size_t buf_size() const {
		return total_size() * DTypeAligBytes_;
	}
	LIBDVCND size_t step() const {
		if (Step_.empty()) return 0;
		return Step_[0];
	}
	LIBDVCND byte* data() const { return DataPtr_; }
	template<typename _ValueType>
	LIBDVCND _ValueType& item()
	{
		assert(sizeof(_ValueType) == DTypeAligBytes_);
		return *(_ValueType*)(ThisPtr_);
	}

protected:
	byte* DataPtr_ = nullptr;
	byte* ThisPtr_ = nullptr;

public:
	LIBDVCND TensorView operator[](int64_t index) const;
	template <typename _TypeName>
	LIBDVCND TensorData& operator=(const _TypeName& _Val);
};

class Tensor : public TensorData
{
public:
	using DType = float;
	Tensor() = default;
	Tensor(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor");
	Tensor(const std::vector<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor");
	Tensor(const Tensor& _Left);
	Tensor(Tensor&& _Right) noexcept;
	~Tensor() override = default;
	Tensor& operator=(const Tensor& _Left);
	Tensor& operator=(Tensor&& _Right) noexcept;

protected:
	MResource<byte> Data_;

private:
	template<typename _ValueType>
	LIBDVCND _ValueType* buf_begin()
	{
		assert(sizeof(_ValueType) == DTypeAligBytes_);
		return (_ValueType*)(Data_.begin()._Ptr);
	}
	template<typename _ValueType>
	LIBDVCND _ValueType* buf_end()
	{
		assert(sizeof(_ValueType) == DTypeAligBytes_);
		return (_ValueType*)(Data_.end()._Ptr);
	}

public:
	static Tensor zeros(const std::vector<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor");
	static Tensor zeros_like(const Tensor& _O);
	static Tensor ones(const std::vector<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor");
	static Tensor ones_like(const Tensor& _O);
	static Tensor rand(const std::vector<int64_t>& _Shape, int _Seed = 114514, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor");
	static Tensor rand_like(const Tensor& _O, int _Seed = 114514);
	static Tensor randn(const std::vector<int64_t>& _Shape, int _Seed = 114514, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor");
	static Tensor randn_like(const Tensor& _O, int _Seed = 114514);
	template <typename __TY>
	static Tensor arange(__TY _Begin, __TY _End, __TY _Step, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor")
	{
		if (__Dtype.find(_Dtype) == __Dtype.end())
			LibDLVoiceCodecThrow("DType Not Recognized");
		if (sizeof(__TY) != __Dtype.at(_Dtype))
			LibDLVoiceCodecThrow("Size Of DType MisMatch");
		Tensor ret;
		ret.Type_ = _Dtype;
		ret.RegName_ = _Name;
		ret.DTypeAligBytes_ = __Dtype.at(ret.Type_);
		ret.MemOwner_ = true;
		const auto diff = _End - _Begin;
		const auto len = size_t(diff / _Step);
		if (len <= 0)
			LibDLVoiceCodecThrow("The Positive And Negative Of Both _End - _Begin And _Step Must Be The Same");

		ret.Shape_ = { len };
		ret.Data_ = MResource<byte>(len * ret.DTypeAligBytes_);
		ret.Step_ = { ret.DTypeAligBytes_ };
		ret.Transopse_ = { 0 };
		ret.DataPtr_ = ret.Data_.data();
		ret.ThisPtr_ = ret.Data_.data();

		__TY* pdata = ret.DataPtr_;
		*(pdata++) = _Begin;
		for (size_t i = 1; i < len; ++i)
		{
			*pdata = *(pdata - 1) + _Step;
			++pdata;
		}

		return ret;
	}
	template <typename __TY>
	static Tensor linspace(__TY _Begin, __TY _End, size_t _Len, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor")
	{
		if (__Dtype.find(_Dtype) == __Dtype.end())
			LibDLVoiceCodecThrow("DType Not Recognized");
		if (sizeof(__TY) != __Dtype.at(_Dtype))
			LibDLVoiceCodecThrow("Size Of DType MisMatch");
		Tensor ret;
		ret.Type_ = _Dtype;
		ret.RegName_ = _Name;
		ret.DTypeAligBytes_ = __Dtype.at(ret.Type_);
		ret.MemOwner_ = true;
		const auto diff = _End - _Begin;
		const auto step = size_t(diff / __TY(_Len));

		ret.Shape_ = { _Len };
		ret.Data_ = MResource<byte>(_Len * ret.DTypeAligBytes_);
		ret.Step_ = { ret.DTypeAligBytes_ };
		ret.Transopse_ = { 0 };
		ret.DataPtr_ = ret.Data_.data();
		ret.ThisPtr_ = ret.Data_.data();

		__TY* pdata = ret.DataPtr_;
		*(pdata++) = _Begin;
		for (size_t i = 1; i < _Len; ++i)
		{
			*pdata = *(pdata - 1) + step;
			++pdata;
		}

		return ret;
	}

protected:
	void loadData(WeightDict& _Dict) override;
	void saveData(FileWrapper& _File) override;
};

class TensorView : public TensorData
{
public:
	TensorView() = default;
	~TensorView() override = default;
	TensorView(const Tensor& _T)
	{
		Shape_ = _T.shape();
		DataPtr_ = _T.data();
	}
	TensorView(Tensor&& _T) = delete;
	TensorView(const TensorView& _T)
	{
		Shape_ = _T.shape();
		DataPtr_ = _T.data();
	}
	TensorView(TensorView&& _T) noexcept
	{
		Shape_ = _T.shape();
		DataPtr_ = _T.data();
	}
	TensorView(const std::initializer_list<int64>& _Shape, byte* _DataPtr)
	{
		Shape_ = _Shape;
		DataPtr_ = _DataPtr;
	}
	TensorView(const std::vector<int64>& _Shape, byte* _DataPtr)
	{
		Shape_ = _Shape;
		DataPtr_ = _DataPtr;
	}
	TensorView(std::vector<int64>&& _Shape, byte* _DataPtr)
	{
		Shape_ = _Shape;
		DataPtr_ = _DataPtr;
	}
	TensorView& operator=(const TensorView& _Left)
	{
		DataPtr_ = _Left.DataPtr_;
		Shape_ = _Left.Shape_;
		return *this;
	}
	TensorView& operator=(TensorView&& _Right) noexcept
	{
		DataPtr_ = _Right.DataPtr_;
		Shape_ = _Right.Shape_;
		return *this;
	}
	TensorView& operator=(const Tensor& _T)
	{
		Shape_ = _T.shape();
		DataPtr_ = _T.data();
		return *this;
	}
	TensorView& operator=(Tensor&& _T)
	{
		Shape_ = _T.shape();
		DataPtr_ = _T.data();
		return *this;
	}
};

LibDLVoiceCodecEnd