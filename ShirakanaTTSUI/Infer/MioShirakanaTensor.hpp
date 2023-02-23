#pragma once
#ifndef SHIRAKANAMIOTENSOR
#define SHIRAKANAMIOTENSOR
#include <onnxruntime_cxx_api.h>
#include <cmath>
#include <fstream>
typedef int64_t int64;
typedef Ort::Value MTensor;
constexpr float gateThreshold = 0.666f;
constexpr int64 maxDecoderSteps = 3000i64;

#endif