```c
#include "NativeApi.h"

//注意，一切设置对象长度/大小的操作都会realloc，缓冲区的地址会发生变化

void callback(size_t,size_t) {}

SliceType _SingleSlice = 0;
LibSvcSlicerSettings _Settings; //切片机设置
short* _InputBuffer = 0;
PUINT64 _OffsetBuffer = 0;
float* _F0Buffer = 0, * _VolumeBuffer = 0, * _CurSpeakerBuffer = 0;
INT32 _Error = 0;
size_t _SliceCount = 0;
FloatVector _F0 = 0, _Volume = 0, _CurSpeaker = 0;
DoubleDimsFloatVector _Speaker = 0;
LibSvcHparams _ModelConfig; //模型配置
LibSvcParams _Params; //推理参数
SvcModel _Model = 0;
VocoderModel _Vocoder = 0;
bool _SpeakerMix = false; //启用角色混合
size_t _Process = 0;

void func(){
    LibSvcInit(); //在进行所有操作之前，必须调用该函数（该函数只能调用一次）

    //为内部实现类申请内存
    Int16Vector _InputAudio = LibSvcAllocateAudio();
    Int16Vector _OutPutAudio = LibSvcAllocateAudio();
    UInt64Vector _SliceOffset = LibSvcAllocateOffset();
    SlicesType _Slices = LibSvcAllocateSliceData();
    MelType _Mel = LibSvcAllocateMel();

    InitLibSvcSlicerSettings(&_Settings);
    InitLibSvcHparams(&_ModelConfig);
    InitLibSvcParams(&_Params);

    //设置输入缓冲区的大小
    LibSvcSetAudioLength(
        _InputAudio, 
        114514 //输入音频的长度
    );

    //获取输入缓冲区
    _InputBuffer = LibSvcGetAudioData(
        _InputAudio
    );
    
    /*
        此处自行实现：将你输入的音频（格式必须为PCM-SignedInt16-LE）写入_InputBuffer
    */
    
    /*
        此处自行修改_Settings（切片机设置）
    */
    
    //对音频进行切片（获取切片的位置）
    _Error = LibSvcSliceAudio(
        _InputAudio, 
        &_Settings, 
        _SliceOffset
    );

    if (_Error)
    {
        BSTR ErrorMessage = LibSvcGetError(0);
        LibSvcFreeString(ErrorMessage);
    }

    /*
        此时切片位置已经保存在了_SliceOffset中，你可以通过调用LibSvcGetOffsetData来获取
        其缓冲区修改其数值，也可以调用LibSvcSetOffsetLength来修改切片位置的个数
    */
    
    //预处理数据（提取F0、音量）
    _Error = LibSvcPreprocess(
        _InputAudio, 
        _SliceOffset, 
        32000, //输入音频的采样率
        512, //STFT HopSize
        20.0, //静音阈值（0 - 32767）
        L"Dio", //F0算法（"Dio" "Harvest" "RMVPE" "FCPE"）
        _Slices
    );

    if (_Error)
    {
        BSTR ErrorMessage = LibSvcGetError(0);
        LibSvcFreeString(ErrorMessage);
    }

    /*
		此时推理所必须的数据均已存储入了_Slices中_Slices为一个切片的容器
    */

    //此处获取了数据的个数（等同于切片的个数）
    _SliceCount = LibSvcGetSliceCount(
        _Slices
    );

    //此处获取了指定Index的单个切片
    _SingleSlice = LibSvcGetSlice(
        _Slices,
        0 //你需要获取到的Slice的索引
    );

    /*
        _SingleSlice为其中一个切片的数据（要注意如果_Slices的内存被释放，那么该_SingleSlice也会被释放）
    */

    //获取F0容器，为一个Float数组，该数组的长度为音频长度（频谱帧数），表示每一帧的F0频率
    _F0 = LibSvcGetF0(_SingleSlice);

    //获取F0Buffer
    _F0Buffer = LibSvcGetFloatVectorData(_F0);

    //获取Volume容器，为一个Float数组，该数组的长度为音频长度（频谱帧数），表示每一帧的音量（0-1）
    _Volume = LibSvcGetVolume(_SingleSlice);

    //获取VolumeBuffer
    _VolumeBuffer = LibSvcGetFloatVectorData(_Volume);

    /*
        此处自己使用_F0Buffer与_VolumeBuffer修改切片中F0和Volume的数值
    */

    if(_SpeakerMix){
        //此Scope中代码处理说话人混合数据（如果不需要角色混合那么就忽略该Scope）

        //设置说话人的数量（每一个SingleSlice都必须单独设置）
        LibSvcSetSpeakerMixDataSize(
            _SingleSlice,
            114 //你模型说话人的数量
        );

        //获取说话人混合信息，包含所有说话人，一共[114]个（上一个函数中设置的说话人数量），相当于float[SpeakerCount][SpecFrameCount]
        _Speaker = LibSvcGetSpeaker(
            _SingleSlice
        );

        //获取指定ID的说话人混合信息，为一个Float数组，该数组的长度为音频长度（频谱帧数），相当于float[SpecFrameCount]，该容器代表每一帧中该角色的
        //音色占全体说话人音色的比例，该容器中的数据必须 ≥ 0，可以 > 1，大于一的数据将会自动归一化（在Speaker维度上归一化）。
        _CurSpeaker = LibSvcGetDFloatVectorData(
            _Speaker,
            0 //说话人ID
        );

        //获取说话人混合信息的Buffer
        _CurSpeakerBuffer = LibSvcGetFloatVectorData(
            _CurSpeaker
        );

        /*
			接下来就可以用_CurSpeakerBuffer修改角色混合比例了
		*/
    }

    //加载模型，第一个参数为Type，0为Vits系SVC，1为Diffusion/ReflowSvc
    _Model = LibSvcLoadModel(
        0, //模型类型（0为Vits系SVC，1为Diffusion/ReflowSvc）
        &_ModelConfig, //模型配置
        callback, //进度条回调函数
        CPU, //算子提供器
        0, //GPU ID
        8 //线程数
    );

    if (!_Model)
    {
    	BSTR ErrorMessage = LibSvcGetError(0);
        LibSvcFreeString(ErrorMessage);
    }

    //加载声码器模型
    _Vocoder = LibSvcLoadVocoder(
        nullptr //声码器模型路径
    );

    if (!_Vocoder)
    {
        BSTR ErrorMessage = LibSvcGetError(0);
        LibSvcFreeString(ErrorMessage);
    }

    //一定要把_Vocoder模型的指针加载到_Params中
    _Params._VocoderModel = _Vocoder;

    //普通推理
	{
    	LibSvcInferSlice(
			_Model, //模型
			0, //模型类型
			_SingleSlice, //切片
			&_Params, //参数
			&_Process, //当前进度
			_OutPutAudio //输出
		);
	}

    //声码器增强
    {
        //推理出一个基础结果
        LibSvcInferSlice(
            _Model, //模型
            0, //模型类型
            _SingleSlice, //切片
            &_Params, //参数
            &_Process, //当前进度
            _OutPutAudio //输出
        );

        /*
			此处自行将_OutPutAudio重采样至声码器的采样率，或是保证满足以下函数的要求
        */

        //短时傅里叶变换，并将其变换到Mel空间。注意：至少要保证该函数参数中(_SamplingRate / _HopSize)与声码器参数的(_SamplingRate / _HopSize)相等
        LibSvcStft(
            _OutPutAudio, //输入音频
            44100, //声码器采样率
            512, //STFT HopSize（声码器的HopSize）
            128, //Mel Bins（必须为声码器的MelBins）
            _Mel //输出的Mel
        );

        LibSvcVocoderEnhance(
            _Vocoder, //声码器模型
            _Mel, //上一步输出的Mel
            _F0, //该切片的F0数据（必须为同一切片的数据）
            128, //Mel Bins（必须为声码器的MelBins）
            _OutPutAudio //输出
        );
    }

    //浅扩散推理
    {
        //用Vits推理出一个基础结果
        LibSvcInferSlice(
            _Model, //模型
            0, //模型类型
            _SingleSlice, //切片
            &_Params, //参数
            &_Process, //当前进度
            _OutPutAudio //输出
        );

        /*
            此处自行将_OutPutAudio重采样至Diffusion模型的采样率，或是保证满足以下函数的要求
        */

        //短时傅里叶变换，并将其变换到Mel空间。注意：至少要保证该函数参数中(_SamplingRate / _HopSize)与Diffusion模型的(_SamplingRate / _HopSize)相等
        LibSvcStft(
            _OutPutAudio, //输入音频
            44100, //Diffusion模型采样率
            512, //STFT HopSize（Diffusion模型的HopSize）
            128, //Mel Bins（必须为Diffusion模型的MelBins）
            _Mel //输出的Mel
        );

        /*
            此处自行将_OutPutAudio重采样至16000采样率
        */

        LibSvcShallowDiffusionInference(
            _Model, //此处的模型必须为Diffusion模型，写教程的时候为了方便我写成了同一个
            _OutPutAudio, //16K采样率的输入音频
            _Mel, //上一步得到的Mel
            _F0, //该切片的F0
            _Volume, //该切片的音量
            _Speaker, //该切片的的角色
            LibSvcGetSrcLength(_SingleSlice), //该切片的原始数据大小
            &_Params, //推理参数
            &_Process, //当前进度
            &_OutPutAudio //输出
        );
    }

    //释放模型，第一个参数为类型
    LibSvcUnloadModel(
        0,
        _Model
    );

    //释放声码器
    LibSvcUnloadVocoder(
        _Vocoder
    );

    //释放其他资源
    LibSvcReleaseAudio(_OutPutAudio);
    LibSvcReleaseAudio(_InputAudio);
    LibSvcReleaseMel(_Mel);
    LibSvcReleaseOffset(_SliceOffset);
    LibSvcReleaseSliceData(_Slices);
}
```