#include "value.h"
#include <random>

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
		return { std::move(NewShape)  ,DataPtr_ + index * (step() * DTypeAligBytes_) };
	}
	if (index > Shape_[0])
		LibDLVoiceCodecThrow("Index Out Of Range");
	std::vector<int64> NewShape{Shape_.begin() + 1, Shape_.end()};
	if (NewShape.empty())
		NewShape.emplace_back(1);
	return { std::move(NewShape)  ,DataPtr_ + index * (step() * DTypeAligBytes_) };
}

template <typename _TypeName>
TensorData& TensorData::operator=(const _TypeName& _Val)
{
	assert(sizeof(_TypeName) == DTypeAligBytes_);
	//TODO
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

void Tensor::loadData(WeightDict& _Dict)
{
	const auto res = _Dict.find(RegName_);
	if (res != _Dict.end())
	{
		Shape_ = res->second.Shape;
		size_t TotalSize = 1;
		for (const auto i : Shape_)
			TotalSize *= i;
		if (TotalSize * DTypeAligBytes_ != res->second.Size)
			LibDLVoiceCodecThrow("Expected size does not match actual size!");
		Data_ = std::move(res->second.Data);
		DataPtr_ = Data_.data();
	}
}

void Tensor::saveData(FileWrapper& _File)
{

}

Tensor::Tensor(const std::initializer_list<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name)
{
	if (__Dtype.find(_Dtype) == __Dtype.end())
		LibDLVoiceCodecThrow("DType Not Recognized");
	DTypeAligBytes_ = __Dtype.at(Type_);
	Shape_ = _Shape;
	Step_ = std::vector(Shape_.size(), 0ui64);
	Step_.back() = DTypeAligBytes_;
	for (size_t i = Step_.size() - 2; i != size_t(-1); --i)
		Step_[i] = Step_[i + 1] * Shape_[i + 1];
	Transopse_ = std::vector(Shape_.size(), 0i64);
	for (size_t i = 1; i < Transopse_.size(); ++i)
		Transopse_[i] = Transopse_[i - 1] + 1;

	size_t TotalSize = 1;
	for (const auto i : Shape_)
		TotalSize *= i;
	if (TotalSize)
		Data_ = MResource<byte>(DTypeAligBytes_ * TotalSize);
	RegName_ = _Name;
	MemOwner_ = true;
	DataPtr_ = Data_.data();
	ThisPtr_ = Data_.data();
	Type_ = _Dtype;
}

Tensor::Tensor(const std::vector<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name)
{
	if (__Dtype.find(_Dtype) == __Dtype.end())
		LibDLVoiceCodecThrow("DType Not Recognized");
	DTypeAligBytes_ = __Dtype.at(Type_);
	Shape_ = _Shape;
	Step_ = std::vector(Shape_.size(), 0ui64);
	Step_.back() = DTypeAligBytes_;
	for (size_t i = Step_.size() - 2; i != size_t(-1); --i)
		Step_[i] = Step_[i + 1] * Shape_[i + 1];
	Transopse_ = std::vector(Shape_.size(), 0i64);
	for (size_t i = 1; i < Transopse_.size(); ++i)
		Transopse_[i] = Transopse_[i - 1] + 1;

	size_t TotalSize = 1;
	for (const auto i : Shape_)
		TotalSize *= i;
	if (TotalSize)
		Data_ = MResource<byte>(DTypeAligBytes_ * TotalSize);
	RegName_ = _Name;
	MemOwner_ = true;
	DataPtr_ = Data_.data();
	ThisPtr_ = Data_.data();
	Type_ = _Dtype;
}

Tensor::Tensor(const Tensor& _Left)
{
	Shape_ = _Left.Shape_;
	Data_ = _Left.Data_;
	Step_ = _Left.Step_;
	Transopse_ = _Left.Transopse_;
	RegName_ = _Left.RegName_;
	MemOwner_ = true;
	DataPtr_ = Data_.data();
	ThisPtr_ = Data_.data();
	Type_ = _Left.Type_;
	DTypeAligBytes_ = __Dtype.at(Type_);
}

Tensor::Tensor(Tensor&& _Right) noexcept
{
	Shape_ = _Right.Shape_;
	Data_ = std::move(_Right.Data_);
	Step_ = std::move(_Right.Step_);
	Transopse_ = std::move(_Right.Transopse_);
	RegName_ = _Right.RegName_;
	MemOwner_ = _Right.MemOwner_;
	DataPtr_ = Data_.data();
	ThisPtr_ = Data_.data();
	Type_ = _Right.Type_;
	DTypeAligBytes_ = __Dtype.at(Type_);
}

Tensor& Tensor::operator=(const Tensor& _Left)
{
	if (&_Left == this)
		return *this;
	Shape_ = _Left.Shape_;
	Data_ = _Left.Data_;
	Step_ = _Left.Step_;
	Transopse_ = _Left.Transopse_;
	RegName_ = _Left.RegName_;
	MemOwner_ = true;
	DataPtr_ = Data_.data();
	ThisPtr_ = Data_.data();
	Type_ = _Left.Type_;
	DTypeAligBytes_ = __Dtype.at(Type_);
	return *this;
}

Tensor& Tensor::operator=(Tensor&& _Right) noexcept
{
	Shape_ = _Right.Shape_;
	Data_ = std::move(_Right.Data_);
	Step_ = std::move(_Right.Step_);
	Transopse_ = std::move(_Right.Transopse_);
	RegName_ = _Right.RegName_;
	MemOwner_ = _Right.MemOwner_;
	DataPtr_ = Data_.data();
	ThisPtr_ = Data_.data();
	Type_ = _Right.Type_;
	DTypeAligBytes_ = __Dtype.at(Type_);
	return *this;
}

template<typename __TY>
void AssignPtr(__TY* Begin, __TY* End, __TY Val)
{
	while (Begin != End)
		*(Begin++) = Val;
}

template<typename __TY>
void AssignPtr(__TY* Begin, __TY* End, __TY(*Val)())
{
	while (Begin != End)
		*(Begin++) = Val();
}

Tensor Tensor::zeros(const std::vector<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name)
{
	Tensor Output{ _Shape ,_Dtype, _Name };
	memset(Output.Data_.data(), 0, Output.Data_.size());
	return Output;
}

Tensor Tensor::zeros_like(const Tensor& _O)
{
	Tensor Output{ _O.shape() ,_O.dtype(), _O.RegName_ };
	memset(Output.Data_.data(), 0, Output.Data_.size());
	return Output;
}

Tensor Tensor::ones(const std::vector<int64_t>& _Shape, const std::string& _Dtype, const std::string& _Name)
{
	Tensor Output{ _Shape ,_Dtype, _Name };
	if (Output.Type_ == "int8")
		AssignPtr(Output.buf_begin<int8>(), Output.buf_end<int8>(), int8(1));
	else if (Output.Type_ == "int16")
		AssignPtr(Output.buf_begin<int16>(), Output.buf_end<int16>(), 1i16);
	else if (Output.Type_ == "int32")
		AssignPtr(Output.buf_begin<int32>(), Output.buf_end<int32>(), 1i32);
	else if (Output.Type_ == "int64")
		AssignPtr(Output.buf_begin<int64>(), Output.buf_end<int64>(), 1i64);
	else if (Output.Type_ == "float8")
		;
	else if (Output.Type_ == "float16")
		;
	else if (Output.Type_ == "bfloat16")
		;
	else if (Output.Type_ == "float32")
		AssignPtr(Output.buf_begin<float32>(), Output.buf_end<float32>(), 1.f);
	else if (Output.Type_ == "float64")
		AssignPtr(Output.buf_begin<float64>(), Output.buf_end<float64>(), 1.);
	else
		AssignPtr(Output.buf_begin<bool>(), Output.buf_end<bool>(), true);
	return Output;
}

Tensor Tensor::ones_like(const Tensor& _O)
{
	return ones(_O.Shape_, _O.Type_, _O.RegName_);
}

Tensor Tensor::rand(const std::vector<int64_t>& _Shape, int _Seed, const std::string& _Dtype, const std::string& _Name)
{
	return {};
}

Tensor Tensor::rand_like(const Tensor& _O, int _Seed)
{
	return rand(_O.Shape_, _Seed, _O.Type_, _O.RegName_);
}

Tensor Tensor::randn(const std::vector<int64_t>& _Shape, int _Seed, const std::string& _Dtype, const std::string& _Name)
{
	return {};
}

Tensor Tensor::randn_like(const Tensor& _O, int _Seed)
{
	return randn(_O.Shape_, _Seed, _O.Type_, _O.RegName_);
}
LibDLVoiceCodecEnd