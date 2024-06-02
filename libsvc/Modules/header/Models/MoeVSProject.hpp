/**
 * FileName: MoeVSProject.hpp
 * Note: MoeVoiceStudioCore 项目相关的定义
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
#include <string>
#include <vector>
#include "../StringPreprocess.hpp"
#include "../../Lib/MJson/MJson.h"
namespace MoeVSProjectSpace
{
    class FileWrapper
    {
    public:
        FileWrapper() = delete;
        FileWrapper(const wchar_t* _path, const wchar_t* _mode)
        {
            _wfopen_s(&file_, _path, _mode);
        }
        ~FileWrapper()
        {
            if (file_)
                fclose(file_);
            file_ = nullptr;
        }
        operator FILE* () const
        {
            return file_;
        }
        [[nodiscard]] bool IsOpen() const
        {
            return file_;
        }
    private:
        FILE* file_ = nullptr;
    };

    using size_type = size_t;

    template <typename T = float>
    size_type GetSize(const std::vector<T>& inp_vec)
    {
        return inp_vec.size() * sizeof(T);
    }

    struct MoeVoiceStudioSvcSlice
    {
        std::vector<int16_t> Audio;
        std::vector<float> F0;
        std::vector<float> Volume;
        std::vector<std::vector<float>> Speaker;
        int32_t OrgLen = 0;
        bool IsNotMute = false;
        MoeVoiceStudioSvcSlice() = default;
    };

    struct MoeVoiceStudioSvcData
    {
        /*std::vector<std::vector<int16_t>> Audio;
        std::vector<std::vector<float>> F0;
        std::vector<std::vector<float>> Volume;
        std::vector<std::vector<std::vector<float>>> Speaker;
        std::vector<long> OrgLen;
        std::vector<bool> IsNotMute;*/
        std::vector<MoeVoiceStudioSvcSlice> Slices;
        std::wstring Path;
        MoeVoiceStudioSvcData() = default;
        [[nodiscard]] size_type Size() const
        {
            size_type _size = 0;
            const std::string bytePath = to_byte_string(Path);
            _size += sizeof(long) * Slices.size() + Slices.size() + bytePath.length() + 1;
            for (const auto& Slice : Slices)
            {
                _size += GetSize(Slice.Audio) + GetSize(Slice.F0) + GetSize(Slice.Volume);
                for (const auto& it : Slice.Speaker)
                    _size += GetSize(it);
            }
            return _size;
        }
    };

	struct MoeVSParams
	{
        //通用
        float NoiseScale = 0.3f;                           //噪声修正因子          0-10
        int64_t Seed = 52468;                              //种子
        int64_t SpeakerId = 0;                             //角色ID
        uint64_t SrcSamplingRate = 48000;                  //源采样率
        int64_t SpkCount = 2;                              //模型角色数

        //SVC
		float IndexRate = 0.f;                             //索引比               0-1
		float ClusterRate = 0.f;                           //聚类比               0-1
		float DDSPNoiseScale = 0.8f;                       //DDSP噪声修正因子      0-10
		float Keys = 0.f;                                  //升降调               -64-64
		size_t MeanWindowLength = 2;                       //均值滤波器窗口大小     1-20
		size_t Pndm = 1;                                 //Diffusion加速倍数    1-200
		size_t Step = 100;                                //Diffusion总步数      1-1000
        float TBegin = 0.f;
        float TEnd = 1.f;
		std::wstring Sampler = L"Pndm";                    //Diffusion采样器
        std::wstring ReflowSampler = L"Eular";             //Reflow采样器
		std::wstring F0Method = L"Dio";                    //F0提取算法
        bool UseShallowDiffusion = false;                  //使用浅扩散
        void* _VocoderModel = nullptr;
        void* _ShallowDiffusionModel = nullptr;
        bool ShallowDiffusionUseSrcAudio = true;
        int VocoderHopSize = 512;
        int VocoderMelBins = 128;
        int VocoderSamplingRate = 44100;
        int64_t ShallowDiffuisonSpeaker = 0;

        //SVCRTInfer
        int64_t RTSampleSize = 44100;
        int64_t CrossFadeLength = 320;

        //TTS
        std::vector<float> SpeakerMix;                     //角色混合比例
        float LengthScale = 1.0f;                          //时长修正因子
        float DurationPredictorNoiseScale = 0.8f;          //随机时长预测器噪声修正因子
        float FactorDpSdp = 0.f;                           //随机时长预测器与时长预测器混合比例
        float GateThreshold = 0.66666f;                    //Tacotron2解码器EOS阈值
        int64_t MaxDecodeStep = 2000;                      //Tacotron2最大解码步数
        std::vector<std::wstring> EmotionPrompt;           //情感标记
        std::wstring PlaceHolderSymbol = L"|";             //音素分隔符
        float RestTime = 0.5f;                             //停顿时间，为负数则直接断开音频并创建新音频
        std::string LanguageSymbol = "JP";                 //语言
        std::wstring AdditionalInfo;                       //G2P额外信息
        std::wstring SpeakerName = L"0";                   //角色名
	};

    struct MoeVSTTSToken
    {
        std::wstring Text;                                 //输入文本
        std::vector<std::wstring> Phonemes;                //音素序列
        std::vector<int64_t> Tones;                        //音调序列
        std::vector<int64_t> Durations;                    //时长序列
        std::vector<std::string> Language;                 //语言序列

        MoeVSTTSToken() = default;
        MoeVSTTSToken(const MJsonValue& _JsonDocument, const MoeVSParams& _InitParams);

        [[nodiscard]] std::wstring Serialization() const;
    };

    struct MoeVSTTSSeq
    {
        std::wstring TextSeq;
        std::vector<MoeVSTTSToken> SlicedTokens;
        std::vector<float> SpeakerMix;                     //角色混合比例
        std::vector<std::wstring> EmotionPrompt;           //情感标记
        std::wstring PlaceHolderSymbol = L"|";             //音素分隔符
        float NoiseScale = 0.3f;                           //噪声修正因子             0-10
        float LengthScale = 1.0f;                          //时长修正因子
        float DurationPredictorNoiseScale = 0.3f;          //随机时长预测器噪声修正因子
        float FactorDpSdp = 0.3f;                          //随机时长预测器与时长预测器混合比例
        float GateThreshold = 0.66666f;                    //Tacotron2解码器EOS阈值
        int64_t MaxDecodeStep = 2000;                      //Tacotron2最大解码步数
        int64_t Seed = 52468;                              //种子
        float RestTime = 0.5f;                             //停顿时间，为负数则直接断开音频并创建新音频
        std::string LanguageSymbol = "ZH";                 //语言标记
        std::wstring SpeakerName = L"0";                   //角色或名称ID
        std::wstring AdditionalInfo;                       //G2P额外信息

        MoeVSTTSSeq() = default;
        MoeVSTTSSeq(const MJsonValue& _JsonDocument, const MoeVSParams& _InitParams);

        [[nodiscard]] std::wstring Serialization() const;

        bool operator==(const MoeVSTTSSeq& right) const;
    };

    using MoeVSSvcParams = MoeVSParams;
    using MoeVSTTSParams = MoeVSParams;

    struct ParamsOffset
    {
        std::vector<size_type> OrgAudio;
        //std::vector<size_type> Hidden_Unit;
        std::vector<size_type> F0;
        std::vector<size_type> Volume;
        std::vector<size_type> Speaker;
        [[nodiscard]] size_type Size() const
        {
            return size_type(OrgAudio.size() + F0.size() + Volume.size() + Speaker.size()) * sizeof(size_type);
        }
        ParamsOffset() = default;
    };

    class MoeVSProject
    {
    public:
        struct Header
        {
            char ChunkSymbol[8] = { 'M','O','E','V','S','P','R','J' };
            size_type DataHeaderAmount = 0;
        };
        struct DataHeader
        {
            char ChunkSymbol[4] = { 'D','A','T','A' };

            size_type OrgAudioOffsetPosSize = 0;

            size_type F0OffsetPosSize = 0;

            size_type VolumeOffsetPosSize = 0;

            size_type CharacterOffsetPosSize = 0;

            size_type OrgLenSize = 0;

            size_type SymbolSize = 0;

            size_type PathSize = 0;

            size_type HeaderSize = sizeof(DataHeader) - sizeof(size_type);
        };
        struct Data
        {
            DataHeader Header;
            ParamsOffset Offset;
            MoeVoiceStudioSvcData ParamData;
            [[nodiscard]] size_type Size() const
            {
                size_type size = sizeof(DataHeader);
                size += Offset.Size();
                size += ParamData.Size();
                return size;
            }
        };

        LibSvcApi MoeVSProject() = delete;
        ~MoeVSProject() = default;

        //从文件读取项目
        LibSvcApi MoeVSProject(const std::wstring& _path);

        //从音频数据构造项目
        LibSvcApi MoeVSProject(const std::vector<MoeVoiceStudioSvcData>& _params);

        //写入文件
        LibSvcApi void Write(const std::wstring& _path) const;

        //从项目获取音频数据
        [[nodiscard]] std::vector<MoeVoiceStudioSvcData> GetParams() const
        {
            std::vector<MoeVoiceStudioSvcData> Params_data;
            Params_data.reserve(data_.size());
            for (const auto& i : data_)
                Params_data.emplace_back(i.ParamData);
            return Params_data;
        }

        //从项目获取音频数据（移动构造）
        [[nodiscard]] std::vector<MoeVoiceStudioSvcData> GetParamsMove()
        {
            std::vector<MoeVoiceStudioSvcData> Params_data;
            Params_data.reserve(data_.size());
            for (auto& i : data_)
                Params_data.emplace_back(std::move(i.ParamData));
            return Params_data;
        }
    private:
        static constexpr size_t begin_offset = sizeof(Header);
        Header moevs_proj_header_;
        std::vector<size_type> data_pos_;
        size_type data_chunk_begin_ = 0;
        std::vector<Data> data_;
    };
}
