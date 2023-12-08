#include "../header/MoeVSProject.hpp"
#include "../../InferTools/inferTools.hpp"
#include <cassert>

namespace MoeVSProjectSpace
{
    MoeVSProject::MoeVSProject(const std::wstring& _path)
    {
        FileWrapper project_file(_path.c_str(), L"rb");
        if (!project_file.IsOpen())
            LibDLVoiceCodecThrow("File Doesn't Exists");
        fseek(project_file, 0, SEEK_SET);

        if (fread(&moevs_proj_header_, 1, sizeof(Header), project_file) != sizeof(Header))
            LibDLVoiceCodecThrow("Unexpected EOF");
        if (!(moevs_proj_header_.ChunkSymbol[0] == 'M' && moevs_proj_header_.ChunkSymbol[1] == 'O' && moevs_proj_header_.ChunkSymbol[2] == 'E' && moevs_proj_header_.ChunkSymbol[3] == 'V' && moevs_proj_header_.ChunkSymbol[4] == 'S' && moevs_proj_header_.ChunkSymbol[5] == 'P' && moevs_proj_header_.ChunkSymbol[6] == 'R' && moevs_proj_header_.ChunkSymbol[7] == 'J'))
            LibDLVoiceCodecThrow("Unrecognized File");
        if (moevs_proj_header_.DataHeaderAmount == 0)
            LibDLVoiceCodecThrow("Empty Project");


        data_pos_ = std::vector<size_type>(moevs_proj_header_.DataHeaderAmount);
        if (fread(data_pos_.data(), 1, sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount), project_file) != sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount))
            LibDLVoiceCodecThrow("Unexpected EOF");
        data_chunk_begin_ = sizeof(Header) + sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount);


        size_type _n_bytes = 0;
        const size_type _data_begin = data_chunk_begin_;
        fseek(project_file, long(_data_begin), SEEK_SET);
        for (const auto& _pos_index : data_pos_)
        {
            Data _datas;

            //Header
            if (fread(&_datas.Header, 1, sizeof(DataHeader), project_file) != sizeof(DataHeader))
                LibDLVoiceCodecThrow("Unexpected EOF");
            if (!(_datas.Header.ChunkSymbol[0] == 'D' && _datas.Header.ChunkSymbol[1] == 'A' && _datas.Header.ChunkSymbol[2] == 'T' && _datas.Header.ChunkSymbol[3] == 'A'))
                LibDLVoiceCodecThrow("Unrecognized File");
            _datas.ParamData.Slices.resize(_datas.Header.OrgLenSize);

            //Audio
            if (_datas.Header.OrgAudioOffsetPosSize != 0) {
                _datas.Offset.OrgAudio = std::vector<size_type>(_datas.Header.OrgAudioOffsetPosSize);
                _n_bytes = sizeof(size_type) * _datas.Header.OrgAudioOffsetPosSize;
                if (fread(_datas.Offset.OrgAudio.data(), 1, _n_bytes, project_file) != _n_bytes)
                    LibDLVoiceCodecThrow("Unexpected EOF");
                size_t _data_idx = 0;
                for (const auto& j : _datas.Offset.OrgAudio)
                {
                    if (!j) continue;
                    std::vector<int16_t> hs_vector(j);
                    _n_bytes = sizeof(int16_t) * j;
                    if (fread(hs_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                        LibDLVoiceCodecThrow("Unexpected EOF");
                    _datas.ParamData.Slices[_data_idx].Audio = std::move(hs_vector);
                    _data_idx += 1;
                }
            }


            //F0
            if (_datas.Header.F0OffsetPosSize != 0)
            {
                _datas.Offset.F0 = std::vector<size_type>(_datas.Header.F0OffsetPosSize);
                _n_bytes = sizeof(size_type) * _datas.Header.F0OffsetPosSize;
                if (fread(_datas.Offset.F0.data(), 1, _n_bytes, project_file) != _n_bytes)
                    LibDLVoiceCodecThrow("Unexpected EOF");
                size_t _data_idx = 0;
                for (const auto& j : _datas.Offset.F0)
                {
                    if (!j) continue;
                    std::vector<float> f0_vector(j);
                    _n_bytes = sizeof(float) * j;
                    if (fread(f0_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                        LibDLVoiceCodecThrow("Unexpected EOF");
                    _datas.ParamData.Slices[_data_idx].F0 = std::move(f0_vector);
                    _data_idx += 1;
                }
            }


            //Volume
            if (_datas.Header.VolumeOffsetPosSize != 0)
            {
                _datas.Offset.Volume = std::vector<size_type>(_datas.Header.VolumeOffsetPosSize);
                _n_bytes = sizeof(size_type) * _datas.Header.VolumeOffsetPosSize;
                if (fread(_datas.Offset.Volume.data(), 1, _n_bytes, project_file) != _n_bytes)
                    LibDLVoiceCodecThrow("Unexpected EOF");
                size_t _data_idx = 0;
                for (const auto& j : _datas.Offset.Volume)
                {
                    if (!j) continue;
                    std::vector<float> Volume_vector(j);
                    _n_bytes = sizeof(float) * j;
                    if (fread(Volume_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                        LibDLVoiceCodecThrow("Unexpected EOF");
                    _datas.ParamData.Slices[_data_idx].Volume = std::move(Volume_vector);
                    _data_idx += 1;
                }
            }


            //ChrarcterMix
            if (_datas.Header.CharacterOffsetPosSize != 0)
            {
                _datas.Offset.Speaker = std::vector<size_type>(_datas.Header.CharacterOffsetPosSize);
                _n_bytes = sizeof(size_type) * _datas.Header.CharacterOffsetPosSize;
                if (fread(_datas.Offset.Speaker.data(), 1, _n_bytes, project_file) != _n_bytes)
                    LibDLVoiceCodecThrow("Unexpected EOF");
                size_t _data_idx = 0;
                for (const auto& j : _datas.Offset.Speaker)
                {
                    if (!j) continue;

                    size_type NSpeaker = 0;
                    if (fread(&NSpeaker, 1, sizeof(size_type), project_file) != sizeof(size_type))
                        LibDLVoiceCodecThrow("Unexpected EOF");
                    if (!NSpeaker) continue;

                    std::vector<float> Speaker_vector(j);
                    _n_bytes = sizeof(float) * j;
                    if (fread(Speaker_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                        LibDLVoiceCodecThrow("Unexpected EOF");
                    std::vector<std::vector<float>> SpkVec;
                    if (!Speaker_vector.empty())
                    {
                        const auto frames = Speaker_vector.size() / NSpeaker;
                        for (size_t idxs = 0; idxs < NSpeaker; ++idxs)
                            SpkVec.emplace_back(Speaker_vector.data() + idxs * frames, Speaker_vector.data() + (idxs + 1) * frames);
                    }
                    _datas.ParamData.Slices[_data_idx].Speaker = std::move(SpkVec);
                    _data_idx += 1;
                }
            }


            //OrgLen
            if (_datas.Header.OrgLenSize != 0)
            {
                std::vector<long> OrgLen(_datas.Header.OrgLenSize);
                _n_bytes = sizeof(long) * _datas.Header.OrgLenSize;
                if (fread(OrgLen.data(), 1, _n_bytes, project_file) != _n_bytes)
                    LibDLVoiceCodecThrow("Unexpected EOF");
                for (size_t _data_idx = 0; _data_idx < OrgLen.size(); ++_data_idx)
                    _datas.ParamData.Slices[_data_idx].OrgLen = OrgLen[_data_idx];
            }


            //Symbol
            if (_datas.Header.SymbolSize != 0)
            {
                std::vector<unsigned char> BooleanVector(_datas.Header.SymbolSize);
                _n_bytes = _datas.Header.SymbolSize;
                if (fread(BooleanVector.data(), 1, _n_bytes, project_file) != _n_bytes)
                    LibDLVoiceCodecThrow("Unexpected EOF");
                for (size_t _data_idx = 0; _data_idx < BooleanVector.size(); ++_data_idx)
                    _datas.ParamData.Slices[_data_idx].IsNotMute = BooleanVector[_data_idx];
            }


            //path
            if (_datas.Header.PathSize != 0)
            {
                std::vector<char> PathStr(_datas.Header.PathSize);
                _n_bytes = _datas.Header.PathSize;
                if (fread(PathStr.data(), 1, _n_bytes, project_file) != _n_bytes)
                    LibDLVoiceCodecThrow("Unexpected EOF");
                _datas.ParamData.Path = to_wide_string(PathStr.data());
            }

            data_.emplace_back(std::move(_datas));
        }
    }

    MoeVSProject::MoeVSProject(const std::vector<MoeVoiceStudioSvcData>& _params)
    {
        moevs_proj_header_.DataHeaderAmount = size_type(_params.size());
        data_chunk_begin_ = sizeof(Header) + sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount);
        data_pos_.push_back(0);
        size_type _offset = 0;
        for (const auto& i : _params)
        {
            Data _data;
            _data.ParamData = i;
            for (const auto& SliceData : i.Slices)
            {
                _data.Offset.OrgAudio.push_back(SliceData.Audio.size());
                _data.Offset.F0.push_back(SliceData.F0.size());
                _data.Offset.Volume.push_back(SliceData.Volume.size());
                size_t size__ = 0;
                for (auto& Speaker : SliceData.Speaker)
                {
                    size__ += Speaker.size();
                }
                _data.Offset.Speaker.push_back(size__);
            }
            _data.Header.OrgAudioOffsetPosSize = _data.Offset.OrgAudio.size();
            _data.Header.F0OffsetPosSize = _data.Offset.F0.size();
            _data.Header.VolumeOffsetPosSize = _data.Offset.Volume.size();
            _data.Header.CharacterOffsetPosSize = _data.Offset.Speaker.size();
            _data.Header.OrgLenSize = i.Slices.size();
            _data.Header.SymbolSize = i.Slices.size();
            const std::string bytePath = to_byte_string(i.Path);
            _data.Header.PathSize = bytePath.length() + 1;
            _offset += _data.Size();
            data_pos_.push_back(_offset);
            data_.emplace_back(std::move(_data));
        }
        data_pos_.pop_back();
    }

    void MoeVSProject::Write(const std::wstring& _path) const
    {
        FILE* project_file = nullptr;
        _wfopen_s(&project_file, _path.c_str(), L"wb");
        if (!project_file)
            LibDLVoiceCodecThrow("Cannot Create File");

        fwrite(&moevs_proj_header_, 1, sizeof(Header), project_file);
        fwrite(data_pos_.data(), 1, data_pos_.size() * sizeof(size_type), project_file);

        for (const auto& i : data_)
        {
            fwrite(&i.Header, 1, sizeof(DataHeader), project_file);


            fwrite(i.Offset.OrgAudio.data(), 1, sizeof(size_type) * i.Offset.OrgAudio.size(), project_file);
            for (const auto& orgaudio : i.ParamData.Slices)
                fwrite(orgaudio.Audio.data(), 1, orgaudio.Audio.size() * sizeof(int16_t), project_file);


            fwrite(i.Offset.F0.data(), 1, sizeof(size_type) * i.Offset.F0.size(), project_file);
            for (const auto& f0 : i.ParamData.Slices)
                fwrite(f0.F0.data(), 1, f0.F0.size() * sizeof(float), project_file);


            if (i.Header.VolumeOffsetPosSize != 0)
            {
                fwrite(i.Offset.Volume.data(), 1, sizeof(size_type) * i.Offset.Volume.size(), project_file);
                for (const auto& volume : i.ParamData.Slices)
                    fwrite(volume.Volume.data(), 1, volume.Volume.size() * sizeof(float), project_file);
            }

            if (i.Header.CharacterOffsetPosSize != 0)
            {
                fwrite(i.Offset.Speaker.data(), 1, sizeof(size_type) * i.Offset.Speaker.size(), project_file);
                for (const auto& Speaker : i.ParamData.Slices)
                {
                    if (Speaker.Speaker.empty()) continue;
                    const size_type NSPK = Speaker.Speaker.size();
                    fwrite(&NSPK, 1, sizeof(size_type), project_file);
                    for (const auto& Spkk : Speaker.Speaker)
                        fwrite(Spkk.data(), 1, Spkk.size() * sizeof(float), project_file);
                }
            }

            assert(i.ParamData.Slices.size() == i.Header.OrgLenSize);
            std::vector<long> CurOrgLen(i.Header.OrgLenSize);
            for (size_t index = 0; index < i.ParamData.Slices.size(); ++index)
                CurOrgLen[index] = i.ParamData.Slices[index].OrgLen;
            fwrite(CurOrgLen.data(), 1, sizeof(long) * CurOrgLen.size(), project_file);

            assert(i.ParamData.Slices.size() == i.Header.SymbolSize);
            std::vector<char> BooleanVector(i.Header.SymbolSize);
            for (size_t index = 0; index < i.ParamData.Slices.size(); ++index)
                BooleanVector[index] = i.ParamData.Slices[index].IsNotMute ? 1 : 0;
            fwrite(BooleanVector.data(), 1, i.Header.SymbolSize, project_file);

            const std::string bytePath = to_byte_string(i.ParamData.Path);
            fwrite(bytePath.data(), 1, i.Header.PathSize, project_file);
        }
        fclose(project_file);
    }

    std::wstring MoeVSTTSToken::Serialization() const
    {
        if (Text.empty())
            return L"\t\t\t{ }";
        std::wstring rtn = L"\t\t\t{\n";
        rtn += L"\t\t\t\t\"Text\": \"" + Text + L"\",\n";
        rtn += L"\t\t\t\t\"Phonemes\": " + wstring_vector_to_string(Phonemes) + L",\n";
        rtn += L"\t\t\t\t\"Tones\": " + vector_to_string(Tones) + L",\n";
        rtn += L"\t\t\t\t\"Durations\": " + vector_to_string(Durations) + L",\n";
        rtn += L"\t\t\t\t\"Language\": " + string_vector_to_string(Language) + L"\n\t\t\t}";
        return rtn;
    }

    MoeVSTTSToken::MoeVSTTSToken(const MJsonValue& _JsonDocument, const MoeVSParams& _InitParams)
    {
        if (_JsonDocument.HasMember("Text") && _JsonDocument["Text"].IsString() && !_JsonDocument["Text"].Empty())
            Text = to_wide_string(_JsonDocument["Text"].GetString());
        else
            LibDLVoiceCodecThrow("Field \"Text\" Should Not Be Empty")

            if (_JsonDocument.HasMember("Phonemes") && _JsonDocument["Phonemes"].IsArray())
                for (const auto& j : _JsonDocument["Phonemes"].GetArray())
                    Phonemes.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : L"[UNK]");

        if (_JsonDocument.HasMember("Tones") && _JsonDocument["Tones"].IsArray())
            for (const auto& j : _JsonDocument["Tones"].GetArray())
                Tones.emplace_back(j.IsInt() ? j.GetInt() : 0);

        if (_JsonDocument.HasMember("Durations") && _JsonDocument["Durations"].IsArray())
            for (const auto& j : _JsonDocument["Durations"].GetArray())
                Durations.emplace_back(j.IsInt() ? j.GetInt() : 0);

        if (_JsonDocument.HasMember("Language") && _JsonDocument["Language"].IsArray())
        {
            const auto LanguageArr = _JsonDocument["Language"].GetArray();
            if (!LanguageArr.empty() && LanguageArr[0].IsString())
                for (const auto& j : LanguageArr)
                    Language.emplace_back(j.GetString());
        }
    }

    std::wstring MoeVSTTSSeq::Serialization() const
    {
        if (TextSeq.empty())
            return L"";
        std::wstring rtn = L"\t{\n";
        rtn += L"\t\t\"TextSeq\": \"" + TextSeq + L"\",\n";
        rtn += L"\t\t\"SlicedTokens\": [\n";
        for (const auto& iter : SlicedTokens)
            rtn += iter.Serialization() + L",\n";
        if (!SlicedTokens.empty())
            rtn.erase(rtn.end() - 2);
        rtn += L"\t\t],\n";
        rtn += L"\t\t\"SpeakerMix\": " + vector_to_string(SpeakerMix) + L",\n";
        rtn += L"\t\t\"EmotionPrompt\": " + wstring_vector_to_string(EmotionPrompt) + L",\n";
        rtn += L"\t\t\"NoiseScale\": " + std::to_wstring(NoiseScale) + L",\n";
        rtn += L"\t\t\"LengthScale\": " + std::to_wstring(LengthScale) + L",\n";
        rtn += L"\t\t\"DurationPredictorNoiseScale\": " + std::to_wstring(DurationPredictorNoiseScale) + L",\n";
        rtn += L"\t\t\"FactorDpSdp\": " + std::to_wstring(FactorDpSdp) + L",\n";
        rtn += L"\t\t\"GateThreshold\": " + std::to_wstring(GateThreshold) + L",\n";
        rtn += L"\t\t\"MaxDecodeStep\": " + std::to_wstring(MaxDecodeStep) + L",\n";
        rtn += L"\t\t\"Seed\": " + std::to_wstring(Seed) + L",\n";
        rtn += L"\t\t\"SpeakerName\": \"" + SpeakerName + L"\",\n";
        rtn += L"\t\t\"RestTime\": " + std::to_wstring(RestTime) + L",\n";
        rtn += L"\t\t\"PlaceHolderSymbol\": \"" + PlaceHolderSymbol + L"\",\n";
        rtn += L"\t\t\"LanguageSymbol\": \"" + to_wide_string(LanguageSymbol) + L"\",\n";
        rtn += L"\t\t\"G2PAdditionalInfo\": \"" + AdditionalInfo + L"\"\n\t}";
        return rtn;
    }

    MoeVSTTSSeq::MoeVSTTSSeq(const MJsonValue& _JsonDocument, const MoeVSParams& _InitParams)
    {
        if (_JsonDocument.HasMember("LanguageSymbol") && _JsonDocument["LanguageSymbol"].IsString() && !_JsonDocument["LanguageSymbol"].Empty())
            LanguageSymbol = _JsonDocument["LanguageSymbol"].GetString();
        else
            LanguageSymbol = _InitParams.LanguageSymbol;

        if (_JsonDocument.HasMember("G2PAdditionalInfo") && _JsonDocument["G2PAdditionalInfo"].IsString() && !_JsonDocument["G2PAdditionalInfo"].Empty())
            AdditionalInfo = to_wide_string(_JsonDocument["G2PAdditionalInfo"].GetString());
        else
            AdditionalInfo = _InitParams.AdditionalInfo;

        if (_JsonDocument.HasMember("PlaceHolderSymbol") && _JsonDocument["PlaceHolderSymbol"].IsString() && !_JsonDocument["PlaceHolderSymbol"].Empty())
            PlaceHolderSymbol = to_wide_string(_JsonDocument["PlaceHolderSymbol"].GetString());
        else
            PlaceHolderSymbol = _InitParams.PlaceHolderSymbol;

        if (_JsonDocument.HasMember("TextSeq") && _JsonDocument["TextSeq"].IsString() && !_JsonDocument["TextSeq"].Empty())
            TextSeq = to_wide_string(_JsonDocument["TextSeq"].GetString());
        else
            LibDLVoiceCodecThrow("Field \"TextSeq\" Should Not Be Empty")

            if (_JsonDocument.HasMember("SlicedTokens") && _JsonDocument["SlicedTokens"].IsArray())
            {
                const auto TokensArrayObject = _JsonDocument["SlicedTokens"].GetArray();
                for (const auto& iter : TokensArrayObject)
                    SlicedTokens.emplace_back(iter, _InitParams);
            }

        if (_JsonDocument.HasMember("SpeakerMix") && _JsonDocument["SpeakerMix"].IsArray())
            for (const auto& j : _JsonDocument["SpeakerMix"].GetArray())
                SpeakerMix.emplace_back(j.IsFloat() ? j.GetFloat() : 0.f);
        else
            SpeakerMix = _InitParams.SpeakerMix;
        if (_JsonDocument.HasMember("EmotionPrompt") && _JsonDocument["EmotionPrompt"].IsArray())
            for (const auto& j : _JsonDocument["EmotionPrompt"].GetArray())
                EmotionPrompt.emplace_back(j.IsString() ? to_wide_string(j.GetString()) : std::wstring());
        else
            EmotionPrompt = _InitParams.EmotionPrompt;
        if (_JsonDocument.HasMember("NoiseScale") && _JsonDocument["NoiseScale"].IsFloat())
            NoiseScale = _JsonDocument["NoiseScale"].GetFloat();
        else
            NoiseScale = _InitParams.NoiseScale;
        if (_JsonDocument.HasMember("LengthScale") && _JsonDocument["LengthScale"].IsFloat())
            LengthScale = _JsonDocument["LengthScale"].GetFloat();
        else
            LengthScale = _InitParams.LengthScale;
        if (_JsonDocument.HasMember("RestTime") && _JsonDocument["RestTime"].IsFloat())
            RestTime = _JsonDocument["RestTime"].GetFloat();
        else
            RestTime = _InitParams.RestTime;
        if (_JsonDocument.HasMember("DurationPredictorNoiseScale") && _JsonDocument["DurationPredictorNoiseScale"].IsFloat())
            DurationPredictorNoiseScale = _JsonDocument["DurationPredictorNoiseScale"].GetFloat();
        else
            DurationPredictorNoiseScale = _InitParams.DurationPredictorNoiseScale;
        if (_JsonDocument.HasMember("FactorDpSdp") && _JsonDocument["FactorDpSdp"].IsFloat())
            FactorDpSdp = _JsonDocument["FactorDpSdp"].GetFloat();
        else
            FactorDpSdp = _InitParams.FactorDpSdp;
        if (_JsonDocument.HasMember("GateThreshold") && _JsonDocument["GateThreshold"].IsFloat())
            GateThreshold = _JsonDocument["GateThreshold"].GetFloat();
        else
            GateThreshold = _InitParams.GateThreshold;
        if (_JsonDocument.HasMember("MaxDecodeStep") && _JsonDocument["MaxDecodeStep"].IsFloat())
            MaxDecodeStep = _JsonDocument["MaxDecodeStep"].GetInt();
        else
            MaxDecodeStep = _InitParams.MaxDecodeStep;
        if (_JsonDocument.HasMember("Seed") && _JsonDocument["Seed"].IsInt())
            Seed = _JsonDocument["Seed"].GetInt();
        else
            Seed = _InitParams.Seed;
        if (_JsonDocument.HasMember("SpeakerName") && _JsonDocument["SpeakerName"].IsString())
            SpeakerName = to_wide_string(_JsonDocument["SpeakerName"].GetString());
        else
            SpeakerName = _InitParams.SpeakerName;

        if (MaxDecodeStep < 2000) MaxDecodeStep = 2000;
        if (MaxDecodeStep > 20000) MaxDecodeStep = 20000;
        if (GateThreshold > 0.90f) GateThreshold = 0.90f;
        if (GateThreshold < 0.2f) GateThreshold = 0.2f;
        if (FactorDpSdp > 1.f) FactorDpSdp = 1.f;
        if (FactorDpSdp < 0.f) FactorDpSdp = 0.f;
        if (NoiseScale > 10.f) NoiseScale = 10.f;
        if (NoiseScale < 0.f) NoiseScale = 0.f;
        if (DurationPredictorNoiseScale > 10.f) DurationPredictorNoiseScale = 10.f;
        if (DurationPredictorNoiseScale < 0.f) DurationPredictorNoiseScale = 0.f;
        if (RestTime > 30.f) RestTime = 30.f;
        if (LengthScale > 10.f) LengthScale = 10.f;
        if (LengthScale < 0.1f) LengthScale = 0.1f;
    }

    bool MoeVSTTSSeq::operator==(const MoeVSTTSSeq& right) const
    {
        return Serialization() == right.Serialization();
    }
}