/**
 * FileName: GPT-SoVits.hpp
 * Note: MoeVoiceStudioCore GPT-SoVits模型类
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

MoeVoiceStudioCoreHeader
class GptSoVits : public TextToSpeech
{
public:
    GptSoVits(const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    GptSoVits(const std::map<std::string, std::wstring>& _PathDict, const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback,
        ExecutionProviders ExecutionProvider_ = ExecutionProviders::CPU,
        unsigned DeviceID_ = 0, unsigned ThreadCount_ = 0);

    void load(const std::map<std::string, std::wstring>& _PathDict,
        const MJson& _Config, const ProgressCallback& _ProgressCallback,
        const DurationCallback& _DurationCallback);

    ~GptSoVits() override;

    void destory()
    {
        delete sessionBert;
        delete sessionVits;
        delete sessionSSL;
        sessionBert = nullptr;
        sessionVits = nullptr;
        sessionSSL = nullptr;

        delete sessionEncoder;
        delete sessionFDecoder;
        delete sessionDecoder;
        sessionEncoder = nullptr;
        sessionFDecoder = nullptr;
        sessionDecoder = nullptr;
    }

    [[nodiscard]] std::tuple<std::vector<float>, std::vector<int64_t>> GetBertPhs(const MoeVSProjectSpace::MoeVSTTSSeq& Seq, const MoeVSG2P::Tokenizer& Tokenizer) const;

    [[nodiscard]] std::vector<std::vector<int16_t>> Inference(const std::vector<MoeVSProjectSpace::MoeVSTTSSeq>& _Input) const override;
private:
    Ort::Session* sessionBert = nullptr;
    Ort::Session* sessionVits = nullptr;
    Ort::Session* sessionSSL = nullptr;

    Ort::Session* sessionEncoder = nullptr;
    Ort::Session* sessionFDecoder = nullptr;
    Ort::Session* sessionDecoder = nullptr;

    int64_t NumLayers = 24;
    int64_t EmbeddingDim = 512;
    int64_t EOSId = 1024;

    std::vector<const char*> VitsInputNames = { "text_seq", "pred_semantic", "ref_audio" };
    const std::vector<const char*> VitsOutputNames = { "audio" };

    std::vector<const char*> EncoderInputNames = { "ref_seq", "text_seq", "ref_bert", "text_bert", "ssl_content" };
    const std::vector<const char*> EncoderOutputNames = { "x", "prompts" };
    std::vector<const char*> DecoderInputNames = { "iy", "ik", "iv", "iy_emb", "ix_example" };
    const std::vector<const char*> DecoderOutputNames = { "y", "k", "v", "y_emb", "logits", "samples" };
    std::vector<const char*> FDecoderInputNames = { "x", "prompts" };
    const std::vector<const char*> FDecoderOutputNames = { "y", "k", "v", "y_emb", "x_example" };

    std::vector<const char*> SSLInputNames = { "audio" };
    const std::vector<const char*> SSLOutputNames = { "last_hidden_state" };

    const std::vector<const char*> BertInputNames = { "input_ids", "attention_mask", "token_type_ids" };
    const std::vector<const char*> BertInputNames2 = { "input_ids", "attention_mask" };
    const std::vector<const char*> BertInputNames3 = { "input_ids" };
    const std::vector<const char*> BertOutputNames = { "last_hidden_state" };
};

MoeVoiceStudioCoreEnd