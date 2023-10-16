#pragma once
#include <map>
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
	virtual ~Value() = default;
	using WeightDict = std::map<std::string, WeightData>;

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

private:
	std::map<std::string, Value*> Layers_;

public:
	void loadData(WeightDict& _Dict) override;
	void saveData(FileWrapper& _File) override;
};

class Tensor : Value
{
public:
	using DType = float;
	Tensor(const std::string& _Name = "Tensor")
	{
		RegName_ = _Name;
		TensorLayer_ = false;
	}

protected:
	std::vector<int64_t> Shape_;
	MResource<byte> Data_;
	bool TensorLayer_ = false;
public:
	void loadData(WeightDict& _Dict) override;
	void saveData(FileWrapper& _File) override;
};

LibDLVoiceCodecEnd