#include "value.h"

LibDLVoiceCodecBegin
Value& Value::load(const std::wstring& _Path)
{
	FileWrapper file;
	file.open(_Path, L"rb");
	if (!file.enabled())
		LibDLVoiceCodecThrow("Failed to open file!");
	char Header[4];
	fread(Header, 1, 4, file);
	if (Header[0] != 'L' || Header[1] != 'S' || Header[2] != 'B' || Header[3] != 'V')
		LibDLVoiceCodecThrow("File does not recognize!");
	size_t MemberNameSize = 0;
	WeightDict weight;
	char MemberName[1025];
	/*
		MemberNameSize          (size_t)
		MemberName              (char[MemberNameSize])
		_WeightData.Size        (size_t)
		_WeightData.ShapeSize   (size_t)
		_WeightData.Shape       (int64_t[_WeightData.ShapeSize])
		_WeightData.Data        (byte[_WeightData.Size])
	 */
	while (fread(&MemberNameSize, 1, sizeof(size_t), file) == sizeof(size_t))
	{
		if (MemberNameSize == 0)
			LibDLVoiceCodecThrow("Size of attrib name must higer than 0!");
		if (MemberNameSize > 1024)
			LibDLVoiceCodecThrow("Size of attrib name must lower than 1024!");
		if (fread(MemberName, 1, MemberNameSize, file) != MemberNameSize)
			LibDLVoiceCodecThrow("Unexpected EOF!");
		MemberName[MemberNameSize] = 0;
		std::string AttribName = MemberName;
		if (weight.find(AttribName) != weight.end())
			continue;

		WeightData _WeightData;

		if (fread(&_WeightData.Size, 1, sizeof(size_t), file) != sizeof(size_t))
			LibDLVoiceCodecThrow("Unexpected EOF!");
		if (_WeightData.Size == 0)
			continue;

		if (fread(&_WeightData.ShapeSize, 1, sizeof(size_t), file) != sizeof(size_t))
			LibDLVoiceCodecThrow("Unexpected EOF!");
		if (_WeightData.ShapeSize == 0)
			continue;

		size_t __SIZE = sizeof(int64_t) * _WeightData.ShapeSize;
		_WeightData.Shape = std::vector<int64_t>(_WeightData.ShapeSize);
		if (fread(_WeightData.Shape.data(), 1, __SIZE, file) != __SIZE)
			LibDLVoiceCodecThrow("Unexpected EOF!");

		__SIZE = _WeightData.Size;
		_WeightData.Data = MResource<byte>(_WeightData.Size);
		if (fread(_WeightData.Data.data(), 1, __SIZE, file) != __SIZE)
			LibDLVoiceCodecThrow("Unexpected EOF!");

		weight[AttribName] = std::move(_WeightData);
	}
	loadData(weight);
	return *this;
}

Value& Value::save(const std::wstring& _Path)
{
	FileWrapper file;
	file.open(_Path, L"rb");
	if (!file.enabled())
		LibDLVoiceCodecThrow("Failed to open file!");
	constexpr char Header[5] = "LSBV";
	fwrite(Header, 1, 4, file);
	saveData(file);
	return *this;
}

void Value::loadData(WeightDict& _Dict)
{
	LibDLVoiceCodecThrow("Not implemented error!");
}

void Value::saveData(FileWrapper& _File)
{
	LibDLVoiceCodecThrow("Not implemented error!");
}

void TensorData::loadData(WeightDict& _Dict)
{
	const auto res = _Dict.find(RegName_);
	if (res != _Dict.end())
	{
		Shape_ = res->second.Shape;
		size_t TotalSize = 1;
		for (const auto i : Shape_)
			TotalSize *= i;
		if (TotalSize * __Dtype.at(Type_) != res->second.Size)
			LibDLVoiceCodecThrow("Expected size does not match actual size!");
		Data_ = std::move(res->second.Data);
		DataPtr_ = Data_.data();
	}
}

void TensorData::saveData(FileWrapper& _File)
{

}

TensorView TensorData::operator[](int64_t index) const
{
	if (index < 0)
	{
		if (index < -Shape_[0])
			LibDLVoiceCodecThrow("Index Out Of Range");
		index += Shape_[0];
		std::vector<int64> NewShape{Shape_.begin() + 1, Shape_.end()};
		if (NewShape.empty())
			NewShape.emplace_back(1);
		return { std::move(NewShape)  ,DataPtr_ + index * (step() * __Dtype.at(Type_)) };
	}
	if (index > Shape_[0])
		LibDLVoiceCodecThrow("Index Out Of Range");
	std::vector<int64> NewShape{Shape_.begin() + 1, Shape_.end()};
	if (NewShape.empty())
		NewShape.emplace_back(1);
	return { std::move(NewShape)  ,DataPtr_ + index * (step() * __Dtype.at(Type_)) };
}

template <typename _TypeName>
TensorData& TensorData::operator=(const _TypeName& _Val)
{
	assert(sizeof(_TypeName) == __Dtype.at(Type_));
	if(Type_ == "int8")
	{
		auto it = begin<int8>();
		const auto en = end<int8>();
		while (it != en)
			*(it++) = (int8)_Val;
	}
	else if(Type_ == "int16")
	{
		auto it = begin<int16>();
		const auto en = end<int16>();
		while (it != en)
			*(it++) = (int16)_Val;
	}
	else if (Type_ == "int32")
	{
		auto it = begin<int32>();
		const auto en = end<int32>();
		while (it != en)
			*(it++) = (int32)_Val;
	}
	else if (Type_ == "int64")
	{
		auto it = begin<int64>();
		const auto en = end<int64>();
		while (it != en)
			*(it++) = (int64)_Val;
	}
	else if (Type_ == "float8")
	{
	}
	else if (Type_ == "float16")
	{
	}
	else if (Type_ == "bfloat16")
	{
	}
	else if (Type_ == "float32")
	{
		auto it = begin<float32>();
		const auto en = end<float32>();
		while (it != en)
			*(it++) = (float32)_Val;
	}
	else if (Type_ == "float64")
	{
		auto it = begin<float64>();
		const auto en = end<float64>();
		while (it != en)
			*(it++) = (float64)_Val;
	}
	else
	{
		auto it = begin<bool>();
		const auto en = end<bool>();
		while (it != en)
			*(it++) = (bool)_Val;
	}
	return *this;
}

void Module::loadData(WeightDict& _Dict)
{
	for(const auto& it : Layers_)
	{
		it.second->loadData(_Dict);
	}
}

void Module::saveData(FileWrapper& _File)
{
	for (const auto& it : Layers_)
	{
		it.second->saveData(_File);
	}
}

Tensor::Tensor(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name, bool _TensorLayer)
{
	if (__Dtype.find(_Dtype) == __Dtype.end())
		LibDLVoiceCodecThrow("DType Not Recognized");
	Shape_ = _Shape;
	size_t TotalSize = 1;
	for (const auto i : Shape_)
		TotalSize *= i;
	Data_ = MResource<byte>(__Dtype.at(_Dtype) * TotalSize);
	RegName_ = _Name;
	TensorLayer_ = _TensorLayer;
	DataPtr_ = Data_.data();
	Type_ = _Dtype;
}

Tensor::Tensor(const std::vector<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name, bool _TensorLayer)
{
	if (__Dtype.find(_Dtype) == __Dtype.end())
		LibDLVoiceCodecThrow("DType Not Recognized");
	Shape_ = _Shape;
	size_t TotalSize = 1;
	for (const auto i : Shape_)
		TotalSize *= i;
	Data_ = MResource<byte>(__Dtype.at(_Dtype) * TotalSize);
	RegName_ = _Name;
	TensorLayer_ = _TensorLayer;
	DataPtr_ = Data_.data();
	Type_ = _Dtype;
}

Tensor::Tensor(const Tensor& _Left)
{
	Shape_ = _Left.Shape_;
	size_t TotalSize = 1;
	for (const auto i : Shape_)
		TotalSize *= i;
	Data_ = MResource<byte>(__Dtype.at(_Left.Type_) * TotalSize);
	RegName_ = _Left.RegName_;
	TensorLayer_ = _Left.TensorLayer_;
	DataPtr_ = Data_.data();
	Type_ = _Left.Type_;
}

Tensor::Tensor(Tensor&& _Right) noexcept
{
	Shape_ = _Right.Shape_;
	Data_ = std::move(_Right.Data_);
	DataPtr_ = Data_.data();
	TensorLayer_ = _Right.TensorLayer_;
	Type_ = _Right.Type_;
	RegName_ = _Right.RegName_;
}

Tensor& Tensor::operator=(const Tensor& _Left)
{
	if (&_Left == this)
		return *this;
	Shape_ = _Left.Shape_;
	size_t TotalSize = 1;
	for (const auto i : Shape_)
		TotalSize *= i;
	Data_ = MResource<byte>(__Dtype.at(_Left.Type_) * TotalSize);
	RegName_ = _Left.RegName_;
	TensorLayer_ = _Left.TensorLayer_;
	DataPtr_ = Data_.data();
	Type_ = _Left.Type_;
	return *this;
}

Tensor& Tensor::operator=(Tensor&& _Right) noexcept
{
	Shape_ = _Right.Shape_;
	Data_ = std::move(_Right.Data_);
	DataPtr_ = Data_.data();
	TensorLayer_ = _Right.TensorLayer_;
	Type_ = _Right.Type_;
	RegName_ = _Right.RegName_;
	return *this;
}

Tensor Tensor::zeros(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name, bool _TensorLayer)
{
	Tensor Output{ _Shape ,_Dtype, _Name, _TensorLayer };
	memset(Output.Data_.data(), 0, Output.Data_.size());
	return Output;
}

Tensor Tensor::zeros_like(const Tensor& _O, bool _TensorLayer)
{
	Tensor Output{ _O.shape() ,_O.dtype(), _O.RegName_, _TensorLayer };
	memset(Output.Data_.data(), 0, Output.Data_.size());
	return Output;
}

Tensor Tensor::ones(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name, bool _TensorLayer)
{
	return {};
}

Tensor Tensor::ones_like(const Tensor& _O, bool _TensorLayer)
{
	return {};
}

Tensor Tensor::rand(const std::initializer_list<int64_t>& _Shape, int _Seed, const std::string& _Dtype, const std::string& _Name, bool _TensorLayer)
{
	return {};
}

Tensor Tensor::rand_like(const Tensor& _O, int _Seed, bool _TensorLayer)
{
	return {};
}

Tensor Tensor::randn(const std::initializer_list<int64_t>& _Shape, int _Seed, const std::string& _Dtype, const std::string& _Name, bool _TensorLayer)
{
	return {};
}

Tensor Tensor::randn_like(const Tensor& _O, int _Seed, bool _TensorLayer)
{
	return {};
}
LibDLVoiceCodecEnd