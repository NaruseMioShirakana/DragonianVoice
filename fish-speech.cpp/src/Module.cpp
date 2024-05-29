#include "Module.h"

LibTTSBegin

Conv1D::Conv1D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params) :
	Module(_Parent, _Name),
	RegisterLayer(weight, { _Params.OutChannels, _Params.InChannels / _Params.Groups, _Params.KernelSize }),
	RegisterLayer(bias, { _Params.OutChannels }),
	params(_Params)
{
	if (!_Params.Bias)
		Layers_.erase(bias.Name());
}

void Conv1D::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} Conv1D(in_channel[{}], out_channel[{}], kernel_size[{}])",
		RegName_,
		params.InChannels,
		params.OutChannels,
		params.KernelSize
	);
}

ggml_tensor* Conv1D::operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace)
{
	if (params.Bias)
		return ggml_add(
			_Ctx,
			ggml_conv_1d(
				_Ctx,
				_Tensor,
				weight,
				params.Stride,
				params.Padding,
				params.Dilation
			),
			bias
		);

	return ggml_conv_1d(
		_Ctx,
		_Tensor,
		weight,
		params.Stride,
		params.Padding,
		params.Dilation
	);
}

void Conv1D::ChangeParam(const ConvParam& _Params)
{
	params = _Params;
	weight.ChangeShape({ _Params.OutChannels, _Params.InChannels / _Params.Groups, _Params.KernelSize });
	bias.ChangeShape({ _Params.OutChannels });
}

Conv2D::Conv2D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params) :
	Module(_Parent, _Name),
	RegisterLayer(
		weight,
		{ _Params.OutChannels, _Params.InChannels / _Params.Groups, _Params.KernelSize[0], _Params.KernelSize[1] }
	),
	RegisterLayer(bias, { _Params.OutChannels }),
	params(_Params)
{
	if (!_Params.Bias)
		Layers_.erase(bias.Name());
}

void Conv2D::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} Conv2D(in_channel[{}], out_channel[{}], kernel_size[{}, {}])",
		RegName_,
		params.InChannels,
		params.OutChannels,
		params.KernelSize[0],
		params.KernelSize[1]
	);
}

ggml_tensor* Conv2D::operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace)
{
	if (params.Bias)
		return ggml_add(
			_Ctx,
			ggml_conv_2d(
				_Ctx,
				_Tensor,
				weight,
				params.Stride[0],
				params.Stride[1],
				params.Padding[0],
				params.Padding[1],
				params.Dilation[0],
				params.Dilation[1]
			),
			bias
		);

	return ggml_conv_2d(
		_Ctx,
		_Tensor,
		weight,
		params.Stride[0],
		params.Stride[1],
		params.Padding[0],
		params.Padding[1],
		params.Dilation[0],
		params.Dilation[1]
	);
}

void Conv2D::ChangeParam(const ConvParam& _Params)
{
	params = _Params;
	weight.ChangeShape({ _Params.OutChannels, _Params.InChannels / _Params.Groups, _Params.KernelSize[0], _Params.KernelSize[1] });
	bias.ChangeShape({ _Params.OutChannels });
}

ConvTranspose1D::ConvTranspose1D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params) :
	Module(_Parent, _Name),
	RegisterLayer(weight, { _Params.InChannels, _Params.OutChannels / _Params.Groups, _Params.KernelSize }),
	RegisterLayer(bias, { _Params.OutChannels }),
	params(_Params)
{
	if (_Params.PaddingMode != Zeros)
		LibTTSThrow("Only `Zeros` Padding Mode Is Supported For ConvTranspose1D");
	if (!_Params.Bias)
		Layers_.erase(bias.Name());
}

void ConvTranspose1D::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} ConvTranspose1D(in_channel[{}], out_channel[{}], kernel_size[{}])",
		RegName_,
		params.InChannels,
		params.OutChannels,
		params.KernelSize
	);
}

ggml_tensor* ConvTranspose1D::operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace)
{
	if(params.Bias)
	{
		if (params.OutPutPadding)
			return ggml_add(
				_Ctx,
				ggml_pad(
					_Ctx,
					ggml_conv_transpose_1d(
						_Ctx,
						_Tensor,
						weight,
						params.Stride,
						params.Padding,
						params.Dilation
					),
					params.OutPutPadding,
					0,
					0,
					0
				),
				bias
			);

		return ggml_add(
			_Ctx,
			ggml_conv_transpose_1d(
				_Ctx,
				_Tensor,
				weight,
				params.Stride,
				params.Padding,
				params.Dilation
			),
			bias
		);
	}

	if (params.OutPutPadding)
		return ggml_pad(
			_Ctx,
			ggml_conv_transpose_1d(
				_Ctx,
				_Tensor,
				weight,
				params.Stride,
				params.Padding,
				params.Dilation
			),
			params.OutPutPadding,
			0,
			0,
			0
		);

	return ggml_conv_transpose_1d(
		_Ctx,
		_Tensor,
		weight,
		params.Stride,
		params.Padding,
		params.Dilation
	);
}

void ConvTranspose1D::ChangeParam(const ConvParam& _Params)
{
	params = _Params;
	weight.ChangeShape({ _Params.InChannels, _Params.OutChannels / _Params.Groups, _Params.KernelSize });
	bias.ChangeShape({ _Params.OutChannels });
}

ConvTranspose2D::ConvTranspose2D(Module* _Parent, const std::wstring& _Name, const ConvParam& _Params) :
	Module(_Parent, _Name),
	RegisterLayer(
		weight,
		{ _Params.InChannels, _Params.OutChannels / _Params.Groups, _Params.KernelSize[0], _Params.KernelSize[1] }
	),
	RegisterLayer(bias, { _Params.OutChannels }),
	params(_Params)
{
	if (!_Params.Bias)
		Layers_.erase(bias.Name());
}

void ConvTranspose2D::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} ConvTranspose2D(in_channel[{}], out_channel[{}], kernel_size[{}, {}])",
		RegName_,
		params.InChannels,
		params.OutChannels,
		params.KernelSize[0],
		params.KernelSize[1]
	);
}

ggml_tensor* ConvTranspose2D::operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace)
{
	LibTTSNotImplementedError; //TODO
}

void ConvTranspose2D::ChangeParam(const ConvParam& _Params)
{
	params = _Params;
	weight.ChangeShape({ _Params.InChannels, _Params.OutChannels / _Params.Groups, _Params.KernelSize[0], _Params.KernelSize[1] });
	bias.ChangeShape({ _Params.OutChannels });
}

Linear::Linear(Module* _Parent, const std::wstring& _Name, const LinearParam& _Params):
	Module(_Parent, _Name),
	RegisterLayer(weight, { _Params.OutFeatures, _Params.InFeatures }),
	RegisterLayer(bias, { _Params.OutFeatures }),
	params(_Params)
{
	if (!_Params.Bias)
		Layers_.erase(bias.Name());
}

void Linear::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} Linear(in_feature[{}], out_feature[{}])",
		RegName_,
		params.InFeatures,
		params.OutFeatures
	);
}

ggml_tensor* Linear::operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace)
{
	if (params.Bias)
		return ggml_add(_Ctx, ggml_mul_mat(_Ctx, _Tensor, weight), bias);

	return ggml_mul_mat(_Ctx, _Tensor, weight);
}

void Linear::ChangeParam(const LinearParam& _Params)
{
	params = _Params;
	weight.ChangeShape({ _Params.OutFeatures, _Params.InFeatures });
	bias.ChangeShape({ _Params.OutFeatures });
}

Embedding::Embedding(Module* _Parent, const std::wstring& _Name, const EmbeddingParam& _Params) :
	Module(_Parent, _Name),
	RegisterLayer(weight, { _Params.NumEmbeddings, _Params.EmbeddingDim }),
	params(_Params)
{
	
}

void Embedding::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} Embedding(num_embeddings[{}], embedding_dim[{}])",
		RegName_,
		params.NumEmbeddings,
		params.EmbeddingDim
	);
}

ggml_tensor* Embedding::operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace)
{
	return ggml_get_rows(_Ctx, weight, _Tensor);
}

void Embedding::ChangeParam(const EmbeddingParam& _Params)
{
	params = _Params;
	weight.ChangeShape({ _Params.NumEmbeddings, _Params.EmbeddingDim });
}

LibTTSEnd	