#pragma once
#include "../BaseF0Extractor/BaseF0Extractor.hpp"

MOEVSFOEXTRACTORHEADER
class DioF0Extractor : public BaseF0Extractor
{
public:
	DioF0Extractor(int sampling_rate, int hop_size, int n_f0_bins = 256, double max_f0 = 1100.0, double min_f0 = 50.0);

	~DioF0Extractor() override = default;

	void compute_f0(const double* PCMData, size_t PCMLen);

	void InterPf0(size_t TargetLength);

	std::vector<float> ExtractF0(const std::vector<double>& PCMData, size_t TargetLength) override;

	static std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0);

private:
	std::vector<double> refined_f0;
};
MOEVSFOEXTRACTOREND