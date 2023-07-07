#pragma once
#include <cstdint>
#include <vector>
#include <string>
#define MOEVSFOEXTRACTORHEADER namespace MoeVSF0Extractor{
#define MOEVSFOEXTRACTOREND }

MOEVSFOEXTRACTORHEADER
#define __NAME__MOEVS(x) std::wstring ClassName = (x)
class BaseF0Extractor
{
public:
	__NAME__MOEVS(L"MoeVSF0Extractor");

	BaseF0Extractor() = delete;

	BaseF0Extractor(int sampling_rate, int hop_size, int n_f0_bins = 256, double max_f0 = 1100.0, double min_f0 = 50.0);

	virtual ~BaseF0Extractor() = default;

	virtual std::vector<float> ExtractF0(const std::vector<double>& PCMData, size_t TargetLength);

	std::vector<float> ExtractF0(const std::vector<float>& PCMData, size_t TargetLength);

	std::vector<float> ExtractF0(const std::vector<int16_t>& PCMData, size_t TargetLength);
protected:
	const uint32_t fs;
	const uint32_t hop;
	const uint32_t f0_bin;
	const double f0_max;
	const double f0_min;
	double f0_mel_min;
	double f0_mel_max;
};
#undef __NAME__MOEVS
MOEVSFOEXTRACTOREND