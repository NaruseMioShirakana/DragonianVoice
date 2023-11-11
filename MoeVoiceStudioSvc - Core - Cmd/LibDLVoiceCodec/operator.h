#pragma once
#include "base.h"

LibDLVoiceCodecBegin
Tensor equal(const Tensor& _A, const Tensor& _B);
Tensor add(const Tensor& _A, const Tensor& _B);
Tensor sub(const Tensor& _A, const Tensor& _B);
Tensor mul(const Tensor& _A, const Tensor& _B);
Tensor div(const Tensor& _A, const Tensor& _B);
void selfAdd(Tensor& _Self, const Tensor& _O);
void selfSub(Tensor& _Self, const Tensor& _O);
void selfMul(Tensor& _Self, const Tensor& _O);
void selfDiv(Tensor& _Self, const Tensor& _O);
Tensor matmul(const Tensor& _A, const Tensor& _B);
Tensor conv1d(const Tensor& _Input, const Tensor& _Weight, const Tensor& _Bias,
	int64 _Stride = 1, int64 _Padding = 0, int64 _Dilation = 1, int64 _Groups = 1);
Tensor conv2d(const Tensor& _Input, const Tensor& _Weight, const Tensor& _Bias,
	int64 _Stride = 1, int64 _Padding = 0, int64 _Dilation = 1, int64 _Groups = 1);
Tensor conv3d(const Tensor& _Input, const Tensor& _Weight, const Tensor& _Bias,
	int64 _Stride = 1, int64 _Padding = 0, int64 _Dilation = 1, int64 _Groups = 1);
Tensor conv_transpose1d(const Tensor& _Input, const Tensor& _Weight, const Tensor& _Bias,
	int64 _Stride = 1, int64 _Padding = 0, int64 _OutputPadding = 0, int64 _Dilation = 1, int64 _Groups = 1);
Tensor conv_transpose2d(const Tensor& _Input, const Tensor& _Weight, const Tensor& _Bias,
	int64 _Stride = 1, int64 _Padding = 0, int64 _OutputPadding = 0, int64 _Dilation = 1, int64 _Groups = 1);
Tensor conv_transpose3d(const Tensor& _Input, const Tensor& _Weight, const Tensor& _Bias,
	int64 _Stride = 1, int64 _Padding = 0, int64 _OutputPadding = 0, int64 _Dilation = 1, int64 _Groups = 1);
LibDLVoiceCodecEnd