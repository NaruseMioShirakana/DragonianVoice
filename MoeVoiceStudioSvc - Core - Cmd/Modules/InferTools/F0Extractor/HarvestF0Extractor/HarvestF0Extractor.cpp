#include "HarvestF0Extractor.hpp"
#include "matlabfunctions.h"
#include "harvest.h"
#include "stonemask.h"

MOEVSFOEXTRACTORHEADER
	HarvestF0Extractor::HarvestF0Extractor(int sampling_rate, int hop_size, int n_f0_bins, double max_f0, double min_f0):
	BaseF0Extractor(sampling_rate, hop_size, n_f0_bins, max_f0, min_f0)
{
}

void HarvestF0Extractor::InterPf0(size_t TargetLength)
{
    const auto f0Len = refined_f0.size();
    if (abs((int64_t)TargetLength - (int64_t)f0Len) < 3)
    {
        refined_f0.resize(TargetLength, 0.0);
	    return;
    }
    for (size_t i = 0; i < f0Len; ++i) if (refined_f0[i] < 0.001) refined_f0[i] = NAN;

    auto xi = arange(0.0, (double)f0Len * (double)TargetLength, (double)f0Len, (double)TargetLength);
    while (xi.size() < TargetLength) xi.emplace_back(*(xi.end() - 1) + ((double)f0Len / (double)TargetLength));
    while (xi.size() > TargetLength) xi.pop_back();

	auto x0 = arange(0, (double)f0Len);
    while (x0.size() < f0Len) x0.emplace_back(*(x0.end() - 1) + 1.);
    while (x0.size() > f0Len) x0.pop_back();

    auto raw_f0 = std::vector<double>(xi.size());
    interp1(x0.data(), refined_f0.data(), static_cast<int>(x0.size()), xi.data(), (int)xi.size(), raw_f0.data());

    for (size_t i = 0; i < xi.size(); i++) if (isnan(raw_f0[i])) raw_f0[i] = 0.0;
    refined_f0 = std::move(raw_f0);
}

std::vector<float> HarvestF0Extractor::ExtractF0(const std::vector<double>& PCMData, size_t TargetLength)
{
    compute_f0(PCMData.data(), PCMData.size());
    InterPf0(TargetLength);
    std::vector<float> f0(refined_f0.size());
    for (size_t i = 0; i < refined_f0.size(); ++i) f0[i] = (float)refined_f0[i];
    return f0;
}

void HarvestF0Extractor::compute_f0(const double* PCMData, size_t PCMLen)
{
    HarvestOption Doption;
    InitializeHarvestOption(&Doption);
    Doption.f0_ceil = f0_max;
    Doption.f0_floor = f0_min;
    Doption.frame_period = 1000.0 * hop / fs;

    const size_t f0Length = GetSamplesForHarvest(int(fs), (int)PCMLen, Doption.frame_period);
	auto temporal_positions = std::vector<double>(f0Length);
	auto raw_f0 = std::vector<double>(f0Length);
    refined_f0 = std::vector<double>(f0Length);
    Harvest(PCMData, (int)PCMLen, int(fs), &Doption, temporal_positions.data(), raw_f0.data());
    StoneMask(PCMData, (int)PCMLen, int(fs), temporal_positions.data(), raw_f0.data(), (int)f0Length, refined_f0.data());
}
MOEVSFOEXTRACTOREND