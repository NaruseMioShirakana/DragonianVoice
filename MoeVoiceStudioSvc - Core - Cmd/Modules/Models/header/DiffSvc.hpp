/**
 * FileName: DiffSvc.hpp
 * Note: MoeVoiceStudioCore Onnx Diffusion系Svc 模型定义
 *
 * Copyright (C) 2022-2023 NaruseMioShirakana (shirakanamio@foxmail.com)
 *
 * This file is part of MoeVoiceStudioCore library.
 * MoeVoiceStudioCore library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License as published by the Free Software Foundation, either version 3
 * of the License, or any later version.
 *
 * MoeVoiceStudioCore library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
 *
 * date: 2022-10-17 Create
*/

#pragma once
#include "SVC.hpp"

MoeVoiceStudioCoreHeader

class DiffusionSvc : public SingingVoiceConversion
{
public:
    DiffusionSvc(const MJson& _Config, const ProgressCallback& _ProgressCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

	~DiffusionSvc() override;

    void Destory();

    [[nodiscard]] std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice,
        const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

    [[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Paths, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
        const InferTools::SlicerSettings& _SlicerSettings) const override;

    [[nodiscard]] std::vector<int16_t> InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

    [[nodiscard]] std::vector<int16_t> ShallowDiffusionInference(
		std::vector<float>& _16KAudioHubert,
        const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
        Ort::Value&& _Mel,
        const std::vector<float>& _SrcF0,
        const std::vector<float>& _SrcVolume,
        const std::vector<std::vector<float>>& _SrcSpeakerMap
    ) const;

private:
    Ort::Session* nsfHifigan = nullptr;

    Ort::Session* encoder = nullptr;
    Ort::Session* denoise = nullptr;
    Ort::Session* pred = nullptr;
    Ort::Session* after = nullptr;
    Ort::Session* alpha = nullptr;
    Ort::Session* naive = nullptr;

    Ort::Session* diffSvc = nullptr;

    int64_t melBins = 128;
    int64_t Pndms = 100;
    int64_t MaxStep = 1000;

    std::wstring DiffSvcVersion = L"DiffSvc";

    const std::vector<const char*> nsfInput = { "c", "f0" };
    const std::vector<const char*> nsfOutput = { "audio" };
    const std::vector<const char*> DiffInput = { "hubert", "mel2ph", "spk_embed", "f0", "initial_noise", "speedup" };
    const std::vector<const char*> DiffOutput = { "mel_pred", "f0_pred" };
    const std::vector<const char*> afterInput = { "x" };
    const std::vector<const char*> afterOutput = { "mel_out" };
    const std::vector<const char*> naiveOutput = { "mel" };
};

MoeVoiceStudioCoreEnd