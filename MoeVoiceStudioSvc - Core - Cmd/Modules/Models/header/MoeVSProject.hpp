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
#include "../../StringPreprocess.hpp"
namespace MoeVSProjectSpace
{
    using size_type = size_t;

    template <typename T = float>
    size_type GetSize(const std::vector<std::vector<T>>& inp_vec)
    {
        size_type _size = 0;
        for (const auto& i : inp_vec)
            _size += i.size() * sizeof(T);
        return _size;
    }

	struct MoeVSAudioSlice
	{
		std::vector<std::vector<int16_t>> Audio;
		std::vector<std::vector<float>> F0;
		std::vector<std::vector<float>> Volume;
		std::vector<std::vector<std::vector<float>>> Speaker;
		std::vector<long> OrgLen;
		std::vector<bool> IsNotMute;
		std::wstring Path;
        MoeVSAudioSlice() = default;
        [[nodiscard]] size_type Size() const
        {
            size_type _size = 0;
            const std::string bytePath = to_byte_string(Path);
            _size += sizeof(long) * OrgLen.size() + IsNotMute.size() + bytePath.length() + 1;
            _size += GetSize(Audio) + GetSize(F0) + GetSize(Volume);
            for (const auto& it : Speaker)
                _size += GetSize(it);
            return _size;
        }
	};

    struct MoeVSAudioSliceRef
    {
        const std::vector<int16_t>& Audio;
        const std::vector<float>& F0;
        const std::vector<float>& Volume;
        const std::vector<float> Mel;
        const std::vector<std::vector<float>>& Speaker;
        bool IsNotMute;
        long OrgLen;
        const std::wstring& Path;
        MoeVSAudioSliceRef(const std::vector<int16_t>& audio, const std::vector<float>& f0, const std::vector<float>& volume, const std::vector<std::vector<float>>& speaker, bool isnotmute, long orglen, const std::wstring& path) :Audio(audio), F0(f0), Volume(volume), Speaker(speaker), IsNotMute(isnotmute), OrgLen(orglen), Path(path) {}
    };

	struct MoeVSSvcParams
	{
		float IndexRate = 0.f;                             //索引比               0-1
		float ClusterRate = 0.f;                           //聚类比               0-1
		float NoiseScale = 0.3f;                           //噪声规模             0-10
		float DDSPNoiseScale = 0.8f;                        //DDSP噪声规模         0-10
		int64_t Seed = 52468;                              //种子
		float Keys = 0.f;                                  //升降调               -64-64
		size_t MeanWindowLength = 2;                       //均值滤波器窗口大小     1-20
		size_t Pndm = 100;                                 //Diffusion加速倍数    2-200
		size_t Step = 1000;                                //Diffusion总步数      200-1000
		std::wstring Sampler = L"Pndm"; //采样器
		std::wstring F0Method = L"Dio";                    //F0提取算法
		int64_t SpeakerId = 0;
		int64_t HopSize = 320;
		int64_t SpkCount = 2;
        uint64_t SrcSamplingRate = 48000;
	};

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

            size_type NSpeaker = 0;

            size_type HeaderSize = sizeof(DataHeader) - sizeof(size_type);
        };
        struct Data
        {
            DataHeader Header;
            ParamsOffset Offset;
            MoeVSAudioSlice ParamData;
            [[nodiscard]] size_type Size() const
            {
                size_type size = sizeof(DataHeader);
                size += Offset.Size();
                size += ParamData.Size();
                return size;
            }
        };

        MoeVSProject() = delete;
        ~MoeVSProject() = default;

        //从文件读取项目
        MoeVSProject(const std::wstring& _path);

        //从音频数据构造项目
        MoeVSProject(const std::vector<MoeVSAudioSlice>& _params);

        //写入文件
        void Write(const std::wstring& _path) const;

        //从项目获取音频数据
        [[nodiscard]] std::vector<MoeVSAudioSlice> GetParams() const
        {
            std::vector<MoeVSAudioSlice> Params_data;
            Params_data.reserve(data_.size());
            for (const auto& i : data_)
                Params_data.emplace_back(i.ParamData);
            return Params_data;
        }

        //从项目获取音频数据（移动构造）
        [[nodiscard]] std::vector<MoeVSAudioSlice> GetParamsMove()
        {
            std::vector<MoeVSAudioSlice> Params_data;
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
