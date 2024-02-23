#include "../../../header/InferTools/F0Extractor/BaseF0Extractor.hpp"
#include <map>
#include "../../../header/Logger/MoeSSLogger.hpp"
#include "../../../header/InferTools/inferTools.hpp"

MoeVSF0Extractor::BaseF0Extractor::BaseF0Extractor(int sampling_rate, int hop_size, int n_f0_bins, double max_f0, double min_f0) :
	fs(sampling_rate),
	hop(hop_size),
	f0_bin(n_f0_bins),
	f0_max(max_f0),
	f0_min(min_f0)
{
	f0_mel_min = (1127.0 * log(1.0 + f0_min / 700.0));
	f0_mel_max = (1127.0 * log(1.0 + f0_max / 700.0));
}

std::vector<double> MoeVSF0Extractor::BaseF0Extractor::arange(double start, double end, double step, double div)
{
	std::vector<double> output;
	while (start < end)
	{
		output.push_back(start / div);
		start += step;
	}
	return output;
}

std::vector<float> MoeVSF0Extractor::BaseF0Extractor::ExtractF0(const std::vector<double>& PCMData, size_t TargetLength)
{
	LibDLVoiceCodecThrow("NotImplementedError");
}

std::vector<float> MoeVSF0Extractor::BaseF0Extractor::ExtractF0(const std::vector<float>& PCMData, size_t TargetLength)
{
	std::vector<double> PCMVector(PCMData.size());
	for (size_t i = 0; i < PCMData.size(); ++i)
		PCMVector[i] = double(PCMData[i]);
	return ExtractF0(PCMVector, TargetLength);
}

std::vector<float> MoeVSF0Extractor::BaseF0Extractor::ExtractF0(const std::vector<int16_t>& PCMData, size_t TargetLength)
{
	std::vector<double> PCMVector(PCMData.size());
	for (size_t i = 0; i < PCMData.size(); ++i)
		PCMVector[i] = double(PCMData[i]);
	return ExtractF0(PCMVector, TargetLength);
}