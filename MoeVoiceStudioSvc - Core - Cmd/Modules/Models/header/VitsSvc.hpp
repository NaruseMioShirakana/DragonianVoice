/**
 * FileName: VitsSvc.hpp
 * Note: MoeVoiceStudioCore Onnx Vits系Svc 模型定义
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
#ifdef WIN32
#ifdef MoeVSMui
#include <queue>
#include <Render/Sound/Mui_DirectSound.h>
#include "../../AvCodec/Recorder.h"
#endif
#endif
#include "DiffSvc.hpp"

MoeVoiceStudioCoreHeader

class VitsSvc : public SingingVoiceConversion
{
public:
    VitsSvc(const MJson& _Config, const ProgressCallback& _ProgressCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    VitsSvc(const std::map<std::string, std::wstring>& _PathDict, const MJson& _Config, const ProgressCallback& _ProgressCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    void load(
        const std::map<std::string, std::wstring>& _PathDict,
        const MJson& _Config, const ProgressCallback& _ProgressCallback,
        ExecutionProviders ExecutionProvider_,
        unsigned DeviceID_, unsigned ThreadCount_,
        bool MoeVoiceStudioFrontEnd = false
    );

	~VitsSvc() override;

    void Destory();

    [[nodiscard]] std::vector<int16_t> SliceInference(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice,
        const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

	[[nodiscard]] std::vector<std::wstring> Inference(std::wstring& _Paths, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams,
        const InferTools::SlicerSettings& _SlicerSettings) const override;

    //[[nodiscard]] std::vector<Ort::Value> InferSliceTensor(const MoeVSProjectSpace::MoeVSAudioSlice& _Slice, size_t SliceIdx,
    //    const MoeVSProjectSpace::MoeVSSvcParams& _InferParams, std::vector<Ort::Value>& _Tensors,
    //    std::vector<const char*>& SoVitsInput) const;

    [[nodiscard]] std::vector<int16_t> InferPCMData(const std::vector<int16_t>& PCMData, long srcSr, const MoeVSProjectSpace::MoeVSSvcParams& _InferParams) const override;

    [[nodiscard]] std::vector<Ort::Value> MelExtractor(const float* PCMAudioBegin, const float* PCMAudioEnd) const;

#ifdef WIN32
#ifdef MoeVSMui
    void StartRT(Mui::Window::UIWindowBasic* window, const MoeVSProjectSpace::MoeVSSvcParams& _Params);
    void EndRT();
#endif
#endif
private:
    Ort::Session* VitsSvcModel = nullptr;
    std::wstring VitsSvcVersion = L"SoVits4.0";

    const std::vector<const char*> soVitsOutput = { "audio" };
    const std::vector<const char*> soVitsInput = { "hidden_unit", "lengths", "pitch", "sid" };
    const std::vector<const char*> RVCInput = { "phone", "phone_lengths", "pitch", "pitchf", "ds", "rnd" };
    const std::vector<const char*> StftOutput = { "mel" };
    const std::vector<const char*> StftInput = { "waveform", "aligment"};
    DiffusionSvc* shallow_diffusion = nullptr;
    Ort::Session* stft_operator = nullptr;
#ifdef WIN32
#ifdef MoeVSMui
    bool RTSTAT = false;
    std::deque<std::vector<int16_t>> inputBuffer, outputBuffer, rawInputBuffer, rawOutputBuffer;
    MRecorder* recoder = nullptr;
    Mui::Render::MDS_AudioPlayer* audio_player = nullptr;
    Mui::Render::MAudioStream* audio_stream = nullptr;
    size_t emptyLen = 30000;
#endif
#endif
};

MoeVoiceStudioCoreEnd
