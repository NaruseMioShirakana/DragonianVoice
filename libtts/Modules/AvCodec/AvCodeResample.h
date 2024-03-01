#pragma once
#include <vector>
#include <string>
#include "../StringPreprocess.hpp"
#include "matlabfunctions.h"
#include "../InferTools/inferTools.hpp"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"
}

class AudioPreprocess
{
public:
    struct WAV_HEADER {
        char             RIFF[4] = { 'R','I','F','F' };              //RIFF标识
        unsigned long    ChunkSize;                                  //文件大小-8
        char             WAVE[4] = { 'W','A','V','E' };              //WAVE块
        char             fmt[4] = { 'f','m','t',' ' };               //fmt块
        unsigned long    Subchunk1Size;                              //fmt块大小
        unsigned short   AudioFormat;                                //编码格式
        unsigned short   NumOfChan;                                  //声道数
        WAV_HEADER(unsigned long cs = 36, unsigned long sc1s = 16, unsigned short af = 1, unsigned short nc = 1) :ChunkSize(cs), Subchunk1Size(sc1s), AudioFormat(af), NumOfChan(nc) {}
    };
    static WAV_HEADER GetHeader(const std::wstring& path)
    {
        WAV_HEADER header;
        char buf[1024];
        FILE* stream;
        _wfreopen_s(&stream, path.c_str(), L"rb", stderr);
        if (stream == nullptr) {
            throw (std::exception("Wav Load Error"));
        }
        fread(buf, 1, 1024, stream);
        int pos = 0;
        while (pos < InferTools::Wav::HEAD_LENGTH) {
            if ((buf[pos] == 'R') && (buf[pos + 1] == 'I') && (buf[pos + 2] == 'F') && (buf[pos + 3] == 'F')) {
                pos += 4;
                break;
            }
            ++pos;
        }
        if (pos >= InferTools::Wav::HEAD_LENGTH)
            throw (std::exception("Don't order fried rice (annoyed)"));
        header.ChunkSize = *(int*)&buf[pos];
        pos += 8;
        while (pos < InferTools::Wav::HEAD_LENGTH) {
            if ((buf[pos] == 'f') && (buf[pos + 1] == 'm') && (buf[pos + 2] == 't')) {
                pos += 4;
                break;
            }
            ++pos;
        }
        if (pos >= InferTools::Wav::HEAD_LENGTH)
            throw (std::exception("Don't order fried rice (annoyed)"));
        header.Subchunk1Size = *(int*)&buf[pos];
        pos += 4;
        header.AudioFormat = *(short*)&buf[pos];
        pos += 2;
        header.NumOfChan = *(short*)&buf[pos];
        if (stream != nullptr) {
            fclose(stream);
        }
        return header;
    }
    static std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0)
    {
        std::vector<double> output;
        while (start < end)
        {
            output.push_back(start / div);
            start += step;
        }
        return output;
    }
    std::vector<short> codec(const std::wstring& path, int sr)
	{
        //if (path.substr(path.rfind(L'.')) == L".wav")
        //    return resample(path, sr);
        std::vector<uint8_t> outData;
        int ret = avformat_open_input(&avFormatContext, to_byte_string(path).c_str(), nullptr, nullptr);
        if (ret != 0) {
            LibDLVoiceCodecThrow((std::string("Can't Open Audio File [ErrCode]")+std::to_string(ret)).c_str())
        }
        ret = avformat_find_stream_info(avFormatContext, nullptr);
        if (ret < 0) {
            LibDLVoiceCodecThrow("Can't Get Audio Info")
        }
        int streamIndex = 0;
        for (unsigned i = 0; i < avFormatContext->nb_streams; ++i) {
        	const AVMediaType avMediaType = avFormatContext->streams[i]->codecpar->codec_type;
            if (avMediaType == AVMEDIA_TYPE_AUDIO) {
                streamIndex = static_cast<int>(i);
            }
        }
        const AVCodecParameters* avCodecParameters = avFormatContext->streams[streamIndex]->codecpar;
        const AVCodecID avCodecId = avCodecParameters->codec_id;
        const AVCodec* avCodec = avcodec_find_decoder(avCodecId);
        if (avCodec == nullptr)
            LibDLVoiceCodecThrow("Don't order fried rice (annoyed)")
        if (avCodecContext == nullptr) {
            LibDLVoiceCodecThrow("Can't Get Decoder Info")
        }
        avcodec_parameters_to_context(avCodecContext, avCodecParameters);
        ret = avcodec_open2(avCodecContext, avCodec, nullptr);
        if (ret < 0) {
            LibDLVoiceCodecThrow("Can't Open Decoder")
        }
        packet = (AVPacket*)av_malloc(sizeof(AVPacket));
        const AVSampleFormat inFormat = avCodecContext->sample_fmt;
        constexpr AVSampleFormat  outFormat = AV_SAMPLE_FMT_S16;
        const int inSampleRate = avCodecContext->sample_rate;
        const int outSampleRate = sr;

        const auto nSample = static_cast<size_t>(avFormatContext->duration * sr / AV_TIME_BASE);

        uint64_t in_ch_layout = avCodecContext->channel_layout;
        if (path.substr(path.rfind(L'.')) == L".wav")
        {
            const auto head = GetHeader(path);
            if (head.NumOfChan == 1)
                in_ch_layout = AV_CH_LAYOUT_MONO;
            else if (head.NumOfChan == 2)
                in_ch_layout = AV_CH_LAYOUT_STEREO;
            else
                LibDLVoiceCodecThrow("unsupported Channel Num")
        }
        constexpr uint64_t out_ch_layout = AV_CH_LAYOUT_MONO;
        swr_alloc_set_opts(swrContext, out_ch_layout, outFormat, outSampleRate,
            static_cast<int64_t>(in_ch_layout), inFormat, inSampleRate, 0, nullptr
        );
        swr_init(swrContext);
        const int outChannelCount = av_get_channel_layout_nb_channels(out_ch_layout);
        int currentIndex = 0;
        out_buffer = (uint8_t*)av_malloc(2ull * sr);
        while (av_read_frame(avFormatContext, packet) >= 0) {
            if (packet->stream_index == streamIndex) {
                avcodec_send_packet(avCodecContext, packet);
                ret = avcodec_receive_frame(avCodecContext, inFrame);
                if (ret == 0) {
                    swr_convert(swrContext, &out_buffer, 2ull * sr,
                        (const uint8_t**)inFrame->data, inFrame->nb_samples);
                    const int out_buffer_size = av_samples_get_buffer_size(nullptr, outChannelCount, (inFrame->nb_samples * sr / inSampleRate) - 1, outFormat, 1);
                    outData.insert(outData.end(), out_buffer, out_buffer + out_buffer_size);
                }
                ++currentIndex;
                av_packet_unref(packet);
            }
        }
        //Wav outWav(static_cast<unsigned long>(sr), static_cast<unsigned long>(outData.size()), outData.data());
        auto outWav = reinterpret_cast<int16_t*>(outData.data());
        const auto RawWavLen = int64_t(outData.size()) / 2;
        if (nSample != static_cast<size_t>(RawWavLen))
        {
            const double interpOff = static_cast<double>(RawWavLen) / static_cast<double>(nSample);
            const auto x0 = arange(0.0, static_cast<double>(RawWavLen), 1.0, 1.0);
            std::vector<double> y0(RawWavLen);
            for (int64_t i = 0; i < RawWavLen; ++i)
                y0[i] = outWav[i] ? static_cast<double>(outWav[i]) : NAN;
            const auto yi = new double[nSample];
        	auto xi = arange(0.0, static_cast<double>(RawWavLen), interpOff, 1.0);
            while (xi.size() < nSample)
                xi.push_back(*(xi.end() - 1) + interpOff);
            while (xi.size() > nSample)
                xi.pop_back();
            interp1(x0.data(), y0.data(), static_cast<int>(RawWavLen), xi.data(), static_cast<int>(nSample), yi);
            std::vector<short> DataChun(nSample);
            for (size_t i = 0; i < nSample; ++i)
                DataChun[i] = isnan(yi[i]) ? 0i16 : static_cast<short>(yi[i]);
            delete[] yi;
            return DataChun;
        }
        release();
        return { outWav , outWav + RawWavLen };
	}
    void release()
	{
        if (packet)
	        av_packet_free(&packet);
        if (inFrame)
            av_frame_free(&inFrame);
        if (out_buffer)
            av_free(out_buffer);
        if (swrContext)
            swr_free(&swrContext);
        if (avCodecContext)
            avcodec_close(avCodecContext);
        if (avFormatContext)
            avformat_close_input(&avFormatContext);
        inFrame = nullptr;
        out_buffer = nullptr;
        swrContext = nullptr;
        avCodecContext = nullptr;
        avFormatContext = nullptr;
        packet = nullptr;
	}
    void init()
	{
        inFrame = av_frame_alloc();
        out_buffer = nullptr;
        swrContext = swr_alloc();
        avCodecContext = avcodec_alloc_context3(nullptr);
        avFormatContext = avformat_alloc_context();
        packet = nullptr;
	}
    AudioPreprocess()
	{
        inFrame = av_frame_alloc();
        out_buffer = nullptr;
        swrContext = swr_alloc();
        avCodecContext = avcodec_alloc_context3(nullptr);
        avFormatContext = avformat_alloc_context();
        packet = nullptr;
	}
    ~AudioPreprocess()
	{
        release();
	}
private:
    AVFrame* inFrame;
    uint8_t* out_buffer;
    SwrContext* swrContext;
    AVCodecContext* avCodecContext;
    AVFormatContext* avFormatContext;
    AVPacket* packet;
};