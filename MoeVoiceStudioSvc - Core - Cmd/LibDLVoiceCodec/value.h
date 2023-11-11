#pragma once
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
	bool TensorLayer_ = false;
	std::string Type_ = "float32";

public:
	void loadData(WeightDict& _Dict) override;
	void saveData(FileWrapper& _File) override;

protected:
	MResource<byte> Data_;

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
		return total_size() * __Dtype.at(Type_);
	}
	LIBDVCND size_t step() const {
		if (Shape_.empty()) return 0;
		return total_size() / Shape_[0];
	}
	LIBDVCND byte* data() const { return DataPtr_; }
	template<typename _ValueType>
	LIBDVCND _ValueType& item()
	{
		assert(sizeof(_ValueType) == __Dtype.at(Type_));
		return *(_ValueType*)(DataPtr_);
	}
	template<typename _ValueType>
	LIBDVCND _ValueType* begin()
	{
		assert(sizeof(_ValueType) == __Dtype.at(Type_));
		return (_ValueType*)(DataPtr_);
	}
	template<typename _ValueType>
	LIBDVCND _ValueType* end()
	{
		assert(sizeof(_ValueType) == __Dtype.at(Type_));
		return (_ValueType*)(DataPtr_)+total_size();
	}

protected:
	byte* DataPtr_ = nullptr;

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
	Tensor(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor", bool _TensorLayer = false);
	Tensor(const std::vector<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor", bool _TensorLayer = false);
	Tensor(const Tensor& _Left);
	Tensor(Tensor&& _Right) noexcept;
	~Tensor() override = default;
	Tensor& operator=(const Tensor& _Left);
	Tensor& operator=(Tensor&& _Right) noexcept;

	static Tensor zeros(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor", bool _TensorLayer = false);
	static Tensor zeros_like(const Tensor& _O, bool _TensorLayer = false);
	static Tensor ones(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor", bool _TensorLayer = false);
	static Tensor ones_like(const Tensor& _O, bool _TensorLayer = false);
	static Tensor rand(const std::initializer_list<int64_t>& _Shape, int _Seed = 114514, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor", bool _TensorLayer = false);
	static Tensor rand_like(const Tensor& _O, int _Seed = 114514, bool _TensorLayer = false);
	static Tensor randn(const std::initializer_list<int64_t>& _Shape, int _Seed = 114514, const std::string& _Dtype = "float32", const std::string& _Name = "Tensor", bool _TensorLayer = false);
	static Tensor randn_like(const Tensor& _O, int _Seed = 114514, bool _TensorLayer = false);
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