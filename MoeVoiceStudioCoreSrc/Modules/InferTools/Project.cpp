#include "Project.hpp"

MoeVSProject::MoeVSProject::MoeVSProject(const std::wstring& _path)
{
    FILE* project_file = nullptr;
    _wfopen_s(&project_file, _path.c_str(), L"rb");
    if(!project_file)
        throw std::exception("File Doesn't Exists");
    fseek(project_file, 0, SEEK_SET);
    

    if (fread(&moevs_proj_header_, 1, sizeof(Header), project_file) != sizeof(Header))
        throw std::exception("Unexpected EOF");
    if (!(moevs_proj_header_.ChunkSymbol[0] == 'M' && moevs_proj_header_.ChunkSymbol[1] == 'O' && moevs_proj_header_.ChunkSymbol[2] == 'E' && moevs_proj_header_.ChunkSymbol[3] == 'V' && moevs_proj_header_.ChunkSymbol[4] == 'S' && moevs_proj_header_.ChunkSymbol[5] == 'P' && moevs_proj_header_.ChunkSymbol[6] == 'R' && moevs_proj_header_.ChunkSymbol[7] == 'J'))
        throw std::exception("Unrecognized File");
    if(moevs_proj_header_.DataHeaderAmount == 0)
        throw std::exception("Empty Project");


    data_pos_ = std::vector<size_type>(moevs_proj_header_.DataHeaderAmount);
    if (fread(data_pos_.data(), 1, sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount), project_file) != sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount))
        throw std::exception("Unexpected EOF");
    data_chunk_begin_ = sizeof(Header) + sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount);


	size_type _n_bytes = 0;
    const size_type _data_begin = data_chunk_begin_;
    fseek(project_file, long(_data_begin), SEEK_SET);
    for(const auto& i : data_pos_)
    {
        Data _datas;

        //Header
        if (fread(&_datas.Header, 1, sizeof(DataHeader), project_file) != sizeof(DataHeader))
            throw std::exception("Unexpected EOF");
        if (!(_datas.Header.ChunkSymbol[0] == 'D' && _datas.Header.ChunkSymbol[1] == 'A' && _datas.Header.ChunkSymbol[2] == 'T' && _datas.Header.ChunkSymbol[3] == 'A'))
            throw std::exception("Unrecognized File");


        //HiddenUnit
        if (_datas.Header.OrgAudioOffsetPosSize != 0) {
            _datas.Offset.OrgAudio = std::vector<size_type>(_datas.Header.OrgAudioOffsetPosSize);
            _n_bytes = sizeof(size_type) * _datas.Header.OrgAudioOffsetPosSize;
            if (fread(_datas.Offset.OrgAudio.data(), 1, _n_bytes, project_file) != _n_bytes)
                throw std::exception("Unexpected EOF");
            for (const auto& j : _datas.Offset.OrgAudio)
            {
                std::vector<int16_t> hs_vector(j);
                _n_bytes = sizeof(int16_t) * j;
                if (fread(hs_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                    throw std::exception("Unexpected EOF");
                _datas.ParamData.OrgAudio.emplace_back(std::move(hs_vector));
            }
        }


        //F0
        if (_datas.Header.F0OffsetPosSize != 0)
        {
            _datas.Offset.F0 = std::vector<size_type>(_datas.Header.F0OffsetPosSize);
            _n_bytes = sizeof(size_type) * _datas.Header.F0OffsetPosSize;
            if (fread(_datas.Offset.F0.data(), 1, _n_bytes, project_file) != _n_bytes)
                throw std::exception("Unexpected EOF");
            for (const auto& j : _datas.Offset.F0)
            {
                std::vector<float> f0_vector(j);
                _n_bytes = sizeof(float) * j;
                if (fread(f0_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                    throw std::exception("Unexpected EOF");
                _datas.ParamData.F0.emplace_back(std::move(f0_vector));
            }
        }


        //Volume
        if(_datas.Header.VolumeOffsetPosSize != 0)
        {
            _datas.Offset.Volume = std::vector<size_type>(_datas.Header.VolumeOffsetPosSize);
            _n_bytes = sizeof(size_type) * _datas.Header.VolumeOffsetPosSize;
            if (fread(_datas.Offset.Volume.data(), 1, _n_bytes, project_file) != _n_bytes)
                throw std::exception("Unexpected EOF");
            for (const auto& j : _datas.Offset.Volume)
            {
                std::vector<float> Volume_vector(j);
                _n_bytes = sizeof(float) * j;
                if (fread(Volume_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                    throw std::exception("Unexpected EOF");
                _datas.ParamData.Volume.emplace_back(std::move(Volume_vector));
            }
        }


        //ChrarcterMix
        if (_datas.Header.CharacterOffsetPosSize != 0)
        {
            _datas.Offset.Speaker = std::vector<size_type>(_datas.Header.CharacterOffsetPosSize);
            _n_bytes = sizeof(size_type) * _datas.Header.CharacterOffsetPosSize;
            if (fread(_datas.Offset.Speaker.data(), 1, _n_bytes, project_file) != _n_bytes)
                throw std::exception("Unexpected EOF");
            for (const auto& j : _datas.Offset.Speaker)
            {
                std::vector<float> Speaker_vector(j);
                _n_bytes = sizeof(float) * j;
                if (fread(Speaker_vector.data(), 1, _n_bytes, project_file) != _n_bytes)
                    throw std::exception("Unexpected EOF");
                _datas.ParamData.Speaker.emplace_back(std::move(Speaker_vector));
            }
        }


        //OrgLen
        if (_datas.Header.OrgLenSize != 0)
        {
	        _datas.ParamData.OrgLen = std::vector<long>(_datas.Header.OrgLenSize);
        	_n_bytes = sizeof(long) * _datas.Header.OrgLenSize;
        	if (fread(_datas.ParamData.OrgLen.data(), 1, _n_bytes, project_file) != _n_bytes)
        		throw std::exception("Unexpected EOF");
        }


        //Symbol
        if (_datas.Header.SymbolSize != 0)
        {
            std::vector<unsigned char> BooleanVector(_datas.Header.SymbolSize);
            _n_bytes = _datas.Header.SymbolSize;
            if (fread(BooleanVector.data(), 1, _n_bytes, project_file) != _n_bytes)
                throw std::exception("Unexpected EOF");
            _datas.ParamData.symbolb = std::vector<bool>((bool*)BooleanVector.data(), (bool*)(BooleanVector.data() + BooleanVector.size()));
        }


        //path
        if (_datas.Header.PathSize != 0)
        {
            std::vector<char> PathStr(_datas.Header.PathSize);
            _n_bytes = _datas.Header.PathSize;
            if (fread(PathStr.data(), 1, _n_bytes, project_file) != _n_bytes)
                throw std::exception("Unexpected EOF");
            _datas.ParamData.paths = to_wide_string(PathStr.data());
        }

        data_.emplace_back(std::move(_datas));
    }
    fclose(project_file);
}

MoeVSProject::MoeVSProject::MoeVSProject(const std::vector<Params>& _params)
{
    moevs_proj_header_.DataHeaderAmount = size_type(_params.size());
    data_chunk_begin_ = sizeof(Header) + sizeof(size_type) * (moevs_proj_header_.DataHeaderAmount);
    data_pos_.push_back(0);
    size_type _offset = 0;
    for(const auto& i : _params)
    {
        Data _data;
        _data.ParamData = i;

        for(const auto& hidden_unit : i.OrgAudio)
            _data.Offset.OrgAudio.push_back(hidden_unit.size());
        _data.Header.OrgAudioOffsetPosSize = _data.Offset.OrgAudio.size();

        for (const auto& f0 : i.F0)
            _data.Offset.F0.push_back(f0.size());
        _data.Header.F0OffsetPosSize = _data.Offset.F0.size();

        if (!i.Volume.empty())
        {
            for (const auto& volume : i.Volume)
                _data.Offset.Volume.push_back(volume.size());
            _data.Header.VolumeOffsetPosSize = _data.Offset.Volume.size();
        }
        else
            _data.Header.VolumeOffsetPosSize = 0;

        if (!i.Speaker.empty())
        {
            for (const auto& Speaker : i.Speaker)
                _data.Offset.Speaker.push_back(Speaker.size());
            _data.Header.CharacterOffsetPosSize = _data.Offset.Speaker.size();
        }
        else
            _data.Header.CharacterOffsetPosSize = 0;

        _data.Header.OrgLenSize = i.OrgLen.size();

        _data.Header.SymbolSize = i.symbolb.size();

        const std::string bytePath = to_byte_string(i.paths);
        _data.Header.PathSize = bytePath.length() + 1;
        _offset += _data.Size();
        data_pos_.push_back(_offset);
        data_.emplace_back(std::move(_data));
    }
    data_pos_.pop_back();
}

void MoeVSProject::MoeVSProject::Write(const std::wstring& _path) const
{
    FILE* project_file = nullptr;
    _wfopen_s(&project_file, _path.c_str(), L"wb");
    if (!project_file)
        throw std::exception("Cannot Create File");

    fwrite(&moevs_proj_header_, 1, sizeof(Header), project_file);
    fwrite(data_pos_.data(), 1, data_pos_.size() * sizeof(size_type), project_file);

    for (const auto& i : data_)
    {
        fwrite(&i.Header, 1, sizeof(DataHeader), project_file);


        fwrite(i.Offset.OrgAudio.data(), 1, sizeof(size_type) * i.Offset.OrgAudio.size(), project_file);
        for (const auto& hidden_unit : i.ParamData.OrgAudio)
            fwrite(hidden_unit.data(), 1, hidden_unit.size() * sizeof(int16_t), project_file);


        fwrite(i.Offset.F0.data(), 1, sizeof(size_type) * i.Offset.F0.size(), project_file);
        for (const auto& f0 : i.ParamData.F0)
            fwrite(f0.data(), 1, f0.size() * sizeof(float), project_file);


        if(i.Header.VolumeOffsetPosSize != 0)
        {
            fwrite(i.Offset.Volume.data(), 1, sizeof(size_type) * i.Offset.Volume.size(), project_file);
            for (const auto& volume : i.ParamData.Volume)
                fwrite(volume.data(), 1, volume.size() * sizeof(float), project_file);
        }

        if (i.Header.CharacterOffsetPosSize != 0)
        {
            fwrite(i.Offset.Speaker.data(), 1, sizeof(size_type) * i.Offset.Speaker.size(), project_file);
            for (const auto& Speaker : i.ParamData.Speaker)
                fwrite(Speaker.data(), 1, Speaker.size() * sizeof(float), project_file);
        }

        fwrite(i.ParamData.OrgLen.data(), 1, sizeof(long) * i.ParamData.OrgLen.size(), project_file);

        std::vector<char> BooleanVector(i.ParamData.symbolb.size());
        for (size_t index = 0; index < i.ParamData.symbolb.size(); ++index)
            BooleanVector[index] = i.ParamData.symbolb[index] ? 1 : 0;
        fwrite(BooleanVector.data(), 1, i.ParamData.symbolb.size(), project_file);

        const std::string bytePath = to_byte_string(i.ParamData.paths);
        fwrite(bytePath.data(), 1, i.Header.PathSize, project_file);
    }
    fclose(project_file);
}

std::vector<std::string> MoeVSProject::TTSProject::load(const std::wstring& _path, T_LOAD _T)
{
    FILE* project_file = nullptr;
    _wfopen_s(&project_file, _path.c_str(), L"rb");
    if (!project_file)
        throw std::exception("File Doesn't Exists");
    std::vector<std::string> return_val;
    if (_T == T_LOAD::REPLACE)
        data_.clear();

    TTSHEADER moevs_proj_header_;
    if (fread(&moevs_proj_header_, 1, sizeof(TTSHEADER), project_file) != sizeof(TTSHEADER))
        throw std::exception("Unexpected EOF");
    if (!(moevs_proj_header_.ChunkSymbol[0] == 'M' && moevs_proj_header_.ChunkSymbol[1] == 'T' && moevs_proj_header_.ChunkSymbol[2] == 'T' && moevs_proj_header_.ChunkSymbol[3] == 'S' && moevs_proj_header_.ChunkSymbol[4] == 'P' && moevs_proj_header_.ChunkSymbol[5] == 'R' && moevs_proj_header_.ChunkSymbol[6] == 'O' && moevs_proj_header_.ChunkSymbol[7] == 'J'))
        throw std::exception("Unrecognized File");
    if (moevs_proj_header_.DataHeaderAmount == 0)
        throw std::exception("Empty Project");

    for (size_type i = 0; i < moevs_proj_header_.DataHeaderAmount; ++i)
    {
        DataHeader _datas;
        TTSParams tts_params;
        //Header
        if (fread(&_datas, 1, sizeof(DataHeader), project_file) != sizeof(DataHeader))
            throw std::exception("Unexpected EOF");
        if (!(_datas.ChunkSymbol[0] == 'D' && _datas.ChunkSymbol[1] == 'A' && _datas.ChunkSymbol[2] == 'T' && _datas.ChunkSymbol[3] == 'A'))
            throw std::exception("Unrecognized File");
        if (fread(&tts_params.noise, 1, sizeof(double), project_file) != sizeof(double))
            throw std::exception("Unexpected EOF");
        if (fread(&tts_params.noise_w, 1, sizeof(double), project_file) != sizeof(double))
            throw std::exception("Unexpected EOF");
        if (fread(&tts_params.length, 1, sizeof(double), project_file) != sizeof(double))
            throw std::exception("Unexpected EOF");
        if (fread(&tts_params.gate, 1, sizeof(double), project_file) != sizeof(double))
            throw std::exception("Unexpected EOF");
        if (fread(&tts_params.decode_step, 1, sizeof(int64_t), project_file) != sizeof(int64_t))
            throw std::exception("Unexpected EOF");
        if (fread(&tts_params.seed, 1, sizeof(int64_t), project_file) != sizeof(int64_t))
            throw std::exception("Unexpected EOF");
        std::vector<wchar_t> _ph(_datas.PhSize + 1, 0);
        std::vector<wchar_t> _emo(_datas.EmoSize + 1, 0);
        tts_params.durations = std::vector(_datas.DurSize, 0i64);
        tts_params.tones = std::vector(_datas.ToneSize, 0i64);
        tts_params.chara_mix = std::vector(_datas.n_speaker, 0.f);

        size_type _n_bytes = _datas.PhSize * sizeof(wchar_t);
        if (fread(_ph.data(), 1, _n_bytes, project_file) != _n_bytes)
            throw std::exception("Unexpected EOF");
        tts_params.phs = _ph.data();

        _n_bytes = _datas.EmoSize * sizeof(wchar_t);
        if (fread(_emo.data(), 1, _n_bytes, project_file) != _n_bytes)
            throw std::exception("Unexpected EOF");
        tts_params.emotion = _emo.data();

        _n_bytes = _datas.DurSize * sizeof(int64_t);
        if (fread(tts_params.durations.data(), 1, _n_bytes, project_file) != _n_bytes)
            throw std::exception("Unexpected EOF");

        _n_bytes = _datas.ToneSize * sizeof(int64_t);
        if (fread(tts_params.tones.data(), 1, _n_bytes, project_file) != _n_bytes)
            throw std::exception("Unexpected EOF");

        _n_bytes = _datas.n_speaker * sizeof(float);
        if (fread(tts_params.chara_mix.data(), 1, _n_bytes, project_file) != _n_bytes)
            throw std::exception("Unexpected EOF");

        if (fread(&tts_params.PlaceHolder, 1, sizeof(wchar_t), project_file) != sizeof(wchar_t))
            throw std::exception("Unexpected EOF");

        return_val.emplace_back(to_byte_string(tts_params.phs));
        data_.emplace_back(std::move(tts_params));
    }
    fclose(project_file);
    return return_val;
}

void MoeVSProject::TTSProject::Write(const std::wstring& _path) const
{
    FILE* project_file = nullptr;
    _wfopen_s(&project_file, _path.c_str(), L"wb");
    if (!project_file)
        {throw std::exception("Cannot Create File");}
	TTSHEADER moevs_proj_header_;
    moevs_proj_header_.DataHeaderAmount = data_.size();
    fwrite(&moevs_proj_header_, 1, sizeof(TTSHEADER), project_file);

    for (const auto& i : data_)
    {
        DataHeader _head;
        _head.EmoSize = i.emotion.length();
        _head.PhSize = i.phs.length();
        _head.DurSize = i.durations.size();
        _head.ToneSize = i.tones.size();
        _head.n_speaker = i.chara_mix.size();
        fwrite(&_head, 1, sizeof(DataHeader), project_file);
        fwrite(&i.noise, 1, sizeof(double), project_file);
        fwrite(&i.noise_w, 1, sizeof(double), project_file);
        fwrite(&i.length, 1, sizeof(double), project_file);
        fwrite(&i.gate, 1, sizeof(double), project_file);
        fwrite(&i.decode_step, 1, sizeof(int64_t), project_file);
        fwrite(&i.seed, 1, sizeof(int64_t), project_file);

        fwrite(i.phs.data(), 1, sizeof(wchar_t) * i.phs.size(), project_file);
        fwrite(i.emotion.data(), 1, sizeof(wchar_t) * i.emotion.size(), project_file);
        fwrite(i.durations.data(), 1, sizeof(int64_t) * i.durations.size(), project_file);
        fwrite(i.tones.data(), 1, sizeof(int64_t) * i.tones.size(), project_file);
        fwrite(i.chara_mix.data(), 1, sizeof(float) * i.chara_mix.size(), project_file);
        fwrite(&i.PlaceHolder, 1, sizeof(wchar_t), project_file);
    }
    fclose(project_file);
}
