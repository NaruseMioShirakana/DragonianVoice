#pragma once
#include <string>
#include <vector>
#include "AvCodeResample.h"

namespace MoeVSProject
{
    using size_type = uint64_t;

    struct SvcParamOld
    {
        std::vector<std::vector<std::vector<float>>> Hidden_Unit;
        std::vector<std::vector<std::vector<float>>> F0;
        std::vector<std::vector<long>> OrgLen;
        std::vector<std::vector<bool>> symbolb;
        std::vector<std::wstring> paths;
        SvcParamOld() = delete;
    };

    template <typename T = float>
    size_type GetSize(const std::vector<std::vector<float>>& inp_vec)
	{
        size_type _size = 0;
        for (const auto& i : inp_vec)
            _size += i.size() * sizeof(T);
        return _size;
    }

    struct Params
    {
        std::vector<std::vector<float>> Hidden_Unit;
        std::vector<std::vector<float>> F0;
        std::vector<std::vector<float>> Volume;
        std::vector<std::vector<float>> Speaker;
        std::vector<long> OrgLen;
        std::vector<bool> symbolb;
        std::wstring paths;
        Params() = default;
        [[nodiscard]] size_type Size() const
        {
            size_type _size = 0;
            const std::string bytePath = to_byte_string(paths);
            _size += sizeof(long) * OrgLen.size() + symbolb.size() + bytePath.length() + 1;
            _size += GetSize<float>(Hidden_Unit) + GetSize<float>(F0) + GetSize<float>(Volume) + GetSize<float>(Speaker);
            return _size;
        }
    };

    struct ParamsOffset
    {
        std::vector<size_type> Hidden_Unit;
        std::vector<size_type> F0;
        std::vector<size_type> Volume;
        std::vector<size_type> Speaker;
        [[nodiscard]] size_type Size() const
        {
            return size_type(Hidden_Unit.size() + F0.size() + Volume.size() + Speaker.size()) * sizeof(size_type);
        }
        ParamsOffset() = default;
    };

    class MoeVSProject
    {
    public:
        struct Header
        {
            char ChunkSymbol[8] = { 'M','O','E','V','S','P','R','J' };
            uint16_t HiddenSize = 256;
            size_type DataHeaderAmount = 0;
        };
        struct DataHeader
        {
            char ChunkSymbol[4] = { 'D','A','T','A' };

            size_type HiddenUnitOffsetPosSize = 0;

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
            Params ParamData;
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
        MoeVSProject(const std::wstring& _path);
        MoeVSProject(const std::vector<Params>& _params, uint16_t HiddenSize = 256);
        void Write(const std::wstring& _path) const;
        [[nodiscard]] std::vector<Params> GetParams() const
        {
            std::vector<Params> Params_data;
            Params_data.reserve(data_.size());
            for (const auto& i : data_)
                Params_data.emplace_back(i.ParamData);
            return Params_data;
        }
    private:
        static constexpr size_t begin_offset = sizeof(Header);
        Header moevs_proj_header_;
        std::vector<size_type> data_pos_;
        size_type data_chunk_begin_ = 0;
        std::vector<Data> data_;
    };

    struct TTSParams
    {
        double noise = 0.666;
        double noise_w = 0.800;
        double length = 1.000;
        double gate = 0.6666;
        int64_t decode_step = 2000;
        int64_t seed = 52468;
        std::wstring emotion;
        std::wstring phs;
        std::vector<int64_t> tones;
        std::vector<int64_t> durations;
        std::vector<float> chara_mix;
        wchar_t PlaceHolder = L'|';
        int64_t chara = -1;
    };

    class TTSProject
    {
    public:
        enum class T_LOAD
        {
	        APPEND,
            REPLACE
        };
        struct TTSHEADER
        {
            char ChunkSymbol[8] = { 'M','T','T','S','P','R','O','J' };
            size_type DataHeaderAmount = 0;
        };
        struct DataHeader
        {
            char ChunkSymbol[4] = { 'D','A','T','A' };

            size_type PhSize = 0;

            size_type DurSize = 0;

            size_type ToneSize = 0;

            size_type EmoSize = 0;

            size_type n_speaker = 0;
        };
        TTSProject() = default;
        ~TTSProject() = default;
        void push(const TTSParams& _item)
        {
            data_.emplace_back(_item);
        }
        void push(TTSParams&& _item)
        {
            data_.emplace_back(_item);
        }
        void push(const std::vector<TTSParams>& _items)
        {
            data_.insert(data_.end(), _items.begin(), _items.end());
        }
        void pop()
        {
	        data_.pop_back();
        }
        void erase(size_t idx)
        {
            data_.erase(data_.begin() + int64_t(idx));
        }
        [[nodiscard]] std::vector<std::string> load(const std::wstring& _path, T_LOAD _T);
        TTSParams& operator[](size_t idx)
        {
            return data_[idx];
        }
        [[nodiscard]] const std::vector<TTSParams>& data() const
    	{
            return data_;
        }
        [[nodiscard]] size_t size() const
        {
            return data_.size();
        }
        void Write(const std::wstring& _path) const;
        void clear()
        {
            data_.clear();
        }
    private:
        std::vector<TTSParams> data_;
    };
}

