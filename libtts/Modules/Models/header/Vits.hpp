/**
 * FileName: Vits.hpp
 * Note: MoeVoiceStudioCore Vits模型类
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
 * date: 2023-11-9 Create
*/

#pragma once
#include "TTS.hpp"
#include <any>

MoeVoiceStudioCoreHeader

void SetBertEnabled(bool cond);

void DestoryAllBerts();

class EmoLoader
{
public:
    static constexpr long startPos = 128;
    EmoLoader() = default;
    EmoLoader(const std::wstring& path)
    {
        if (emofile)
            fclose(emofile);
        emofile = nullptr;
        _wfopen_s(&emofile, path.c_str(), L"r");
        if (!emofile)
            LibDLVoiceCodecThrow("emoFile not exists")
    }
    ~EmoLoader()
    {
        if (emofile)
            fclose(emofile);
        emofile = nullptr;
    }
    void close()
    {
        if (emofile)
            fclose(emofile);
        emofile = nullptr;
    }
    void open(const std::wstring& path)
    {
        if (emofile)
            fclose(emofile);
        emofile = nullptr;
        _wfopen_s(&emofile, path.c_str(), L"rb");
        if (!emofile)
            LibDLVoiceCodecThrow("emoFile not exists")
    }
    std::vector<float> operator[](long index) const
    {
        if (emofile)
        {
            fseek(emofile, index * 4096 + startPos, SEEK_SET);
            char buffer[4096];
            const auto buf = reinterpret_cast<float*>(buffer);
            const auto bufread = fread_s(buffer, 4096, 1, 4096, emofile);
            if (bufread == 4096)
                return { buf ,buf + 1024 };
            LibDLVoiceCodecThrow("emo index out of range")
        }
        LibDLVoiceCodecThrow("emo file not opened")
    }
private:
    FILE* emofile = nullptr;
};

class Vits : public TextToSpeech
{
public:
    Vits(const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    Vits(const std::map<std::string, std::wstring>& _PathDict, const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback, const std::vector<std::wstring>& _BertPaths,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    void load(const std::map<std::string, std::wstring>& _PathDict,
        const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback, const std::vector<std::wstring>& _BertPaths = {});

	~Vits() override;

    void destory()
    {
        delete sessionDec;
        delete sessionSdp;
        delete sessionDp;
        delete sessionEnc_p;
        delete sessionFlow;
        delete sessionEmb;
        sessionDec = nullptr;
        sessionSdp = nullptr;
        sessionEnc_p = nullptr;
        sessionFlow = nullptr;
        sessionEmb = nullptr;
        sessionDp = nullptr;
    }

    [[nodiscard]] std::vector<float> GetEmotionVector(const std::vector<std::wstring>& src) const;

    [[nodiscard]] std::tuple<bool, std::vector<std::wstring>, std::vector<std::wstring>, std::vector<int64_t>, std::vector<int64_t>, std::vector<int64_t>, std::vector<size_t>, bool> PreProcessSeq(const MoeVSProjectSpace::MoeVSTTSSeq& Seq, int64_t CurLanguageIdx) const;

    [[nodiscard]] std::vector<std::vector<int16_t>> Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const override;

    [[nodiscard]] std::vector<int16_t> Infer(const MoeVSProjectSpace::MoeVSTTSSeq& Seq) const;

    std::vector<Ort::Value> GetBertFeature(size_t IndexOfBert, const std::vector<std::wstring>& TokenSeq, Ort::Session* CurBertSession) const;

protected:

    Ort::Session* sessionDec = nullptr;
    Ort::Session* sessionSdp = nullptr;
    Ort::Session* sessionDp = nullptr;
    Ort::Session* sessionEnc_p = nullptr;
    Ort::Session* sessionFlow = nullptr;
    Ort::Session* sessionEmb = nullptr;

    std::string VitsType;
    
    int64_t DefBertSize = 1024;
    int64_t VQCodeBookSize = 10;

    bool UseTone = false;
    bool UseBert = false;
    bool UseLength = true;
    bool UseLanguage = false;
    bool EncoderG = false;
    bool ReferenceBert = false;
    bool UseVQ = false;
    bool UseClap = false;
    bool Emotion = false;

    std::wstring ClapName;

private:

    EmoLoader EmoLoader;
    std::unordered_map<std::wstring, long> Emo2Id;

    std::vector<const char*> EncoderInputNames = { "x" };
    const std::vector<const char*> EncoderOutputNames = { "xout", "m_p", "logs_p", "x_mask" };

    std::vector<const char*> SdpInputNames = { "x", "x_mask", "zin" };
    const std::vector<const char*> SdpOutputNames = { "logw" };

    std::vector<const char*> DpInputNames = { "x", "x_mask" };
    const std::vector<const char*> DpOutputNames = { "logw" };

	std::vector<const char*> FlowInputNames = { "z_p", "y_mask" };
    const std::vector<const char*> FlowOutputNames = { "z" };

    std::vector<const char*> DecInputNames = { "z_in" };
    const std::vector<const char*> DecOutputNames = { "o" };

    const std::vector<const char*> EmbiddingInputNames = { "sid" };
    const std::vector<const char*> EmbiddingOutputNames = { "g" };

    const std::vector<const char*> BertInputNames = { "input_ids", "attention_mask", "token_type_ids" };
    const std::vector<const char*> BertOutputNames = { "last_hidden_state" };
};

MoeVoiceStudioCoreEnd