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

void Tensor::loadData(WeightDict& _Dict)
{
	const auto res = _Dict.find(RegName_);
	if (res != _Dict.end())
	{
		Shape_ = res->second.Shape;
		size_t TotalSize = 1;
		for (const auto i : Shape_)
			TotalSize *= i;
		if (TotalSize * sizeof(DType) != res->second.Size)
			LibDLVoiceCodecThrow("Expected size does not match actual size!");
		Data_ = std::move(res->second.Data);
	}
}

void Tensor::saveData(FileWrapper& _File)
{

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

LibDLVoiceCodecEnd