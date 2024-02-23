#pragma once
#include <vector>
#include <string>
#include "../../StringPreprocess.hpp"
#include "matlabfunctions.h"
#include "../inferTools.hpp"
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
    LibSvcApi static WAV_HEADER GetHeader(const std::wstring& path);
    LibSvcApi static std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0);
    LibSvcApi std::vector<short> codec(const std::wstring& path, int sr);
    LibSvcApi void release();
    LibSvcApi void init();
    LibSvcApi AudioPreprocess();
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
