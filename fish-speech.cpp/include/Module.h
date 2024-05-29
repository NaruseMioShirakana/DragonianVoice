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
	struct ConvParam
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
	Conv1D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params);
	ggml_tensor* operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(const ConvParam& _Params);
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	Parameter bias;
	ConvParam params;
};

class Conv2D : public Module
{
public:
	struct ConvParam
	{
		SizeType InChannels;
		SizeType OutChannels;
		SizeType KernelSize[2];
		int Stride[2] = { 1, 1 };
		int Padding[2] = { 0, 0 };
		int Dilation[2] = { 1, 1 };
		int Groups = 1;
		bool Bias = true;
		PaddingType PaddingMode = Zeros;
	};
	Conv2D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params);
	ggml_tensor* operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(const ConvParam& _Params);
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	Parameter bias;
	ConvParam params;
};

class ConvTranspose1D : public Module
{
public:
	struct ConvParam
	{
		SizeType InChannels;
		SizeType OutChannels;
		SizeType KernelSize;
		int Stride = 1;
		int Padding = 0;
		int OutPutPadding = 0;
		int Dilation = 1;
		int Groups = 1;
		bool Bias = true;
		PaddingType PaddingMode = Zeros;
	};
	ConvTranspose1D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params);
	ggml_tensor* operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(const ConvParam& _Params);
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	Parameter bias;
	ConvParam params;
};

class ConvTranspose2D : public Module
{
public:
	struct ConvParam
	{
		SizeType InChannels;
		SizeType OutChannels;
		SizeType KernelSize[2];
		int Stride[2] = { 1, 1 };
		int Padding[2] = { 0, 0 };
		int OutPutPadding[2] = { 0, 0 };
		int Dilation[2] = { 1, 1 };
		int Groups = 1;
		bool Bias = true;
		PaddingType PaddingMode = Zeros;
	};
	ConvTranspose2D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params);
	ggml_tensor* operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(const ConvParam& _Params);
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	Parameter bias;
	ConvParam params;
};

class Linear : public Module
{
public:
	struct LinearParam
	{
		SizeType InFeatures;
		SizeType OutFeatures;
		bool Bias = true;
	};
	Linear(Module* _Parent, const std::wstring& _Name, const LinearParam& _Params);
	ggml_tensor* operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(const LinearParam& _Params);
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	Parameter bias;
	LinearParam params;
};

class Embedding : public Module
{
public:
	struct EmbeddingParam
	{
		int NumEmbeddings;
		int EmbeddingDim;
		int PaddingIdx = INT_MAX;
		float MaxNorm = NAN;
		float NormType = 2.f;
		bool ScaleGradByFreq = false;
		bool Sparse = false;
		bool Freeze = false;
	};
	Embedding(Module* _Parent, const std::wstring& _Name, const EmbeddingParam& _Params);
	ggml_tensor* operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(const EmbeddingParam& _Params);
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	//Parameter bias;
	EmbeddingParam params;
};

LibTTSEnd
