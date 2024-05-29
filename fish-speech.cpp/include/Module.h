#pragma once
#include "Base.h"

LibTTSBegin

enum PaddingType
{
	Zeros,
	Constant,
	Reflect,
	Replicate,
	Cycle
};

class Conv1D : public Module
{
public:
	struct Conv1DParam
	{
		SizeType InChannels;
		SizeType OutChannels;
		SizeType KernelSize;
		int Stride = 1;
		int Padding = 0;
		int Dilation = 1;
		int Groups = 1;
		bool Bias = true;
		PaddingType PaddingMode = Zeros;
	};

	Conv1D(Module* _Parent, const std::wstring& _Name, const Conv1DParam& _Params, ggml_context* _Ctx);
	ggml_tensor* operator()(ggml_tensor* _Tensor) override;
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	Parameter bias;
	Conv1DParam params;
};

class Linear : public Module
{
public:
	Linear(Module* _Parent, const std::wstring& _Name) :
		Module(_Parent, _Name),
		RegisterLayer(weight),
		RegisterLayer(bias)
	{}
private:
	Parameter weight;
	Parameter bias;
};

LibTTSEnd
