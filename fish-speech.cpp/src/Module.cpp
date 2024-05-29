#include "Module.h"

LibTTSBegin

Conv1D::Conv1D(Module* _Parent, const std::wstring& _Name, const Conv1DParam& _Params, ggml_context* _Ctx) :
	Module(_Parent, _Name, _Ctx),
	RegisterLayer(weight),
	RegisterLayer(bias),
	params(_Params)
{
}

void Conv1D::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} (in_channel[{}], out_channel[{}], kernel_size[{}])",
		RegName_,
		params.InChannels,
		params.OutChannels,
		params.KernelSize
	);
}

ggml_tensor* Conv1D::operator()(ggml_tensor* _Tensor)
{
	auto Tmp = ggml_conv_1d(GGMLCtx_, _Tensor, weight, params.Stride, params.Padding, params.Dilation);
	return ggml_add(GGMLCtx_, Tmp, bias);
}

LibTTSEnd	