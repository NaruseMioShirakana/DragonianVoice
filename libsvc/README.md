# 使用方法
### 构建
- 1、配置以下依赖：
  - [ffmpeg](https://ffmpeg.org/)
  - [onnxruntime](https://github.com/microsoft/onnxruntime)
  - [fftw](http://fftw.org/)
  - [openblas](https://github.com/OpenMathLib/OpenBLAS)
  - [faiss](https://github.com/facebookresearch/faiss)
  - [liblapack](https://netlib.org/lapack/)
- 2、编译
---
### 使用动态库
- 1、链接libsvc
- 2、#include "libsvc/Api/header/libsvc.h"
- 3、调用libsvc::Init()
- 4、调用libsvc名称空间中的函数