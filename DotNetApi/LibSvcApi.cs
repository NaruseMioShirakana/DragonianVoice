using System;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using static LibSvcApi.LibSvc;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace LibSvcApi
{
    
    public unsafe class FloatVector
    {
        private FloatVector() { }

        public FloatVector(void* _Obj)
        {
            Obj_ = _Obj;
        }

        public void* Object { get { return Obj_; } }

        public float* Data()
        {
            return LibSvcGetFloatVectorData(Obj_);
        }

        public ulong Size()
        {
            return LibSvcGetFloatVectorSize(Obj_);
        }

        [DllImport("libsvc.dll")]
        private static extern float* LibSvcGetFloatVectorData(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern ulong LibSvcGetFloatVectorSize(void* _Obj);

        private void* Obj_ = null;
    }

    public unsafe class DFloatVector
    {
        private DFloatVector() { }

        public DFloatVector(void* _Obj)
        {
            Obj_ = _Obj;
        }

        public FloatVector Data(ulong _Index)
        {
            return new FloatVector(LibSvcGetDFloatVectorData(Obj_, _Index));
        }

        public FloatVector this[ulong i] => Data(i);

        public void* Object { get { return Obj_; } }

        public ulong Size()
        {
            return LibSvcGetDFloatVectorSize(Obj_);
        }

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcGetDFloatVectorData(void* _Obj, ulong _Index);

        [DllImport("libsvc.dll")]
        private static extern ulong LibSvcGetDFloatVectorSize(void* _Obj);

        private void* Obj_ = null;
    }

    public unsafe class Int16Vector
    {

        public Int16Vector()
        {
            Obj_ = LibSvcAllocateAudio();
            Owner_ = true;
        }

        public Int16Vector(void* _Obj)
        {
            Obj_ = _Obj;
        }

        ~Int16Vector()
        {
            if (Owner_)
                LibSvcReleaseAudio(Obj_);
        }

        public void* Object { get { return Obj_; } }

        public short* Data()
        {
            return LibSvcGetAudioData(Obj_);
        }

        public ulong Size()
        {
            return LibSvcGetAudioSize(Obj_);
        }

        public void Resize(ulong _Size)
        {
            LibSvcSetAudioLength(Obj_, _Size);
        }

        public void Insert(ref Int16Vector _Input)
        {
            LibSvcInsertAudio(Obj_, _Input.Object);
        }

        [DllImport("libsvc.dll")]
        private static extern short* LibSvcGetAudioData(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern ulong LibSvcGetAudioSize(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcAllocateAudio();

        [DllImport("libsvc.dll")]
        private static extern void LibSvcReleaseAudio(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void LibSvcSetAudioLength(void* _Obj, ulong _Size);

        [DllImport("libsvc.dll")]
        private static extern void LibSvcInsertAudio(void* _ObjA, void* _ObjB);

        private void* Obj_ = null;
        private bool Owner_ = false;
    }

    public unsafe class UInt64Vector
    {

        public UInt64Vector()
        {
            Obj_ = LibSvcAllocateOffset();
            Owner_ = true;
        }

        public UInt64Vector(void* _Obj)
        {
            Obj_ = _Obj;
        }

        ~UInt64Vector()
        {
            if (Owner_)
                LibSvcReleaseOffset(Obj_);
        }

        public void* Object { get { return Obj_; } }

        public ulong* Data()
        {
            return LibSvcGetOffsetData(Obj_);
        }

        public ulong Size()
        {
            return LibSvcGetOffsetSize(Obj_);
        }

        public void Resize(ulong _Size)
        {
            LibSvcSetOffsetLength(Obj_, _Size);
        }

        [DllImport("libsvc.dll")]
        private static extern ulong* LibSvcGetOffsetData(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern ulong LibSvcGetOffsetSize(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcAllocateOffset();

        [DllImport("libsvc.dll")]
        private static extern void LibSvcReleaseOffset(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void LibSvcSetOffsetLength(void* _Obj, ulong _Size);

        private void* Obj_ = null;
        private bool Owner_ = false;
    }

    public unsafe class Mel
    {

        public Mel()
        {
            Obj_ = LibSvcAllocateMel();
            Owner_ = true;
        }

        public Mel(void* _Obj)
        {
            Obj_ = _Obj;
        }

        ~Mel()
        {
            if (Owner_)
                LibSvcReleaseMel(Obj_);
        }

        public void* Object { get { return Obj_; } }

        public FloatVector MelData()
        {
            return new FloatVector(LibSvcGetMelData(Obj_));
        }

        public long Size()
        {
            return LibSvcGetMelSize(Obj_);
        }

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcGetMelData(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern long LibSvcGetMelSize(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcAllocateMel();

        [DllImport("libsvc.dll")]
        private static extern void LibSvcReleaseMel(void* _Obj);

        private void* Obj_ = null;
        private bool Owner_ = false;
    }

    public unsafe class Slice
    {
        private Slice() { }

        public Slice(void* _Obj)
        {
            Obj_ = _Obj;
        }

        public void* Object { get { return Obj_; } }

        public Int16Vector GetAudio()
        {
            return new Int16Vector(LibSvcGetAudio(Obj_));
        }

        public FloatVector GetF0()
        {
            return new FloatVector(LibSvcGetF0(Obj_));
        }

        public FloatVector GetVolume()
        {
            return new FloatVector(LibSvcGetVolume(Obj_));
        }

        public DFloatVector GetSpeaker()
        {
            return new DFloatVector(LibSvcGetSpeaker(Obj_));
        }

        public int SrcLength()
        {
            return LibSvcGetSrcLength(Obj_);
        }

        public bool IsNotMute()
        {
            return LibSvcGetIsNotMute(Obj_) != 0;
        }

        public void SetSpeakerMixDataCount(ulong _NSpeaker)
        {
            LibSvcSetSpeakerMixDataSize(Obj_, _NSpeaker);
        }

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcGetAudio(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcGetF0(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcGetVolume(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcGetSpeaker(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern int LibSvcGetSrcLength(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern int LibSvcGetIsNotMute(void* _Obj);

        [DllImport("libsvc.dll")]
        private static extern void LibSvcSetSpeakerMixDataSize(void* _Obj, ulong _NSpeaker);

        private void* Obj_ = null;
    }

    public unsafe class Slices
    {

        public Slices()
        {
            Obj_ = LibSvcAllocateSliceData();
            Owner_ = true;
        }

        public Slices(void* _Obj)
        {
            Obj_ = _Obj;
        }

        ~Slices()
        {
            if (Owner_)
                LibSvcReleaseSliceData(Obj_);
        }

        public void* Object { get { return Obj_; } }

        public Slice GetSlice(ulong _Index)
        {
            return new Slice(LibSvcGetSlice(Obj_, _Index));
        }

        public string GetAudioPath()
        {
            return LibSvcGetAudioPath(Obj_);
        }

        public ulong Size()
        {
            return LibSvcGetSliceCount(Obj_);
        }

        public Slice this[ulong i] => GetSlice(i);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcAllocateSliceData();

        [DllImport("libsvc.dll")]
        private static extern void LibSvcReleaseSliceData(void* _Obj);

        [DllImport("libsvc.dll", CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.BStr)]
        private static extern string LibSvcGetAudioPath(void * _Obj);

        [DllImport("libsvc.dll")]
        private static extern void* LibSvcGetSlice(void* _Obj, ulong _Index);

        [DllImport("libsvc.dll")]
        private static extern ulong LibSvcGetSliceCount(void* _Obj);

        private void* Obj_ = null;
        private bool Owner_ = false;
    }

    public unsafe class ModelBase
    {
        protected ModelBase() { }

        [DllImport("libsvc.dll")]
        protected static extern int LibSvcInferSlice(
            void* _Model,
            uint _T,
            void* _Slice,
            ref Params _InferParams,
            ref ulong _Process,
            void* _Output
        );

        [DllImport("libsvc.dll")]
        protected static extern int LibSvcUnloadModel(
            uint _T,
            void* _Model
        );
    }

    public unsafe class VitsModel : ModelBase
    {
        private VitsModel() { }
        public VitsModel(void* _Obj) 
        {
            Model_ = _Obj;
        }

        ~VitsModel() 
        {
            if (LibSvcUnloadModel(0, Model_) == 0)
                return;
            throw new Exception(GetError(0));
        }

        public Int16Vector Inference(
            Slice _Slice,
            ref Params _InferParams,
            ref ulong _Process
        )
        {
            Int16Vector Ret = new();
            if (LibSvcInferSlice(Model_, 0, _Slice.Object, ref _InferParams, ref _Process, Ret.Object) == 0)
                return Ret;
            throw new Exception(GetError(0));
        }

        private void* Model_ = null;
    }

    public unsafe class UnionModel : ModelBase
    {
        private UnionModel() { }
        public UnionModel(void* _Obj)
        {
            Model_ = _Obj;
        }

        ~UnionModel()
        {
            if (LibSvcUnloadModel(1, Model_) == 0)
                return;
            throw new Exception(GetError(0));
        }

        public Int16Vector Inference(
            Slice _Slice,
            ref Params _InferParams,
            ref ulong _Process
        )
        {
            Int16Vector Ret = new();
            if (LibSvcInferSlice(Model_, 1, _Slice.Object, ref _InferParams, ref _Process, Ret.Object) == 0)
                return Ret;
            throw new Exception(GetError(0));
        }

        public Int16Vector ShallowDiffusionInference(
            ref Int16Vector _16KAudioHubert,
            ref Mel _Mel,
            ref FloatVector _SrcF0,
            ref FloatVector _SrcVolume,
            ref DFloatVector _SrcSpeakerMap,
            long _SrcSize,
            ref Params _InferParams,
            ref ulong _Process
        )
        {
            Int16Vector Ret = new();
            if (
                LibSvcShallowDiffusionInference(
                    Model_,
                    _16KAudioHubert.Object,
                    _Mel.Object,
                    _SrcF0.Object,
                    _SrcVolume.Object,
                    _SrcSpeakerMap.Object,
                    _SrcSize,
                    ref _InferParams,
                    ref _Process,
                    Ret.Object
                ) == 0
            ) return Ret;
            throw new Exception(GetError(0));
        }

        [DllImport("libsvc.dll")]
        private static extern int LibSvcShallowDiffusionInference(
            void* _Model,
            void* _16KAudioHubert,
            void* _Mel,
            void* _SrcF0,
            void* _SrcVolume,
            void* _SrcSpeakerMap,
            long _SrcSize,
            ref Params _InferParams,
            ref ulong _Process,
            void* _Output
        );

        private void* Model_ = null;
    }

    public unsafe class VocoderModel
    {
        private VocoderModel() { }
        public VocoderModel(void* _Obj)
        {
            Model_ = _Obj;
        }

        ~VocoderModel()
        {
            if (LibSvcUnloadVocoder(Model_) == 0)
                return;
            throw new Exception(GetError(0));
        }

        public void* GetModel()
        {
            return Model_;
        }

        public Int16Vector Inference(
            ref Mel _Mel,
            ref FloatVector _F0,
            int _VocoderMelBins
        )
        {
            Int16Vector Ret = new();
            if (LibSvcVocoderEnhance(Model_, _Mel.Object, _F0.Object, _VocoderMelBins, Ret.Object) == 0)
                return Ret;
            throw new Exception(GetError(0));
        }

        [DllImport("libsvc.dll")]
        private static extern int LibSvcVocoderEnhance(
            void* Model,
            void* Mel,
            void* F0,
		    int VocoderMelBins,
            void* _Output
	    );

        [DllImport("libsvc.dll")]
        private static extern int LibSvcUnloadVocoder(void* _Model);

        private void* Model_ = null;
    }

    public unsafe class LibSvc
    {
        public enum Device
        {
            CPU = 0,
            CUDA = 1,
            DML = 2
        }

        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public unsafe struct SlicerSettings
        {
            public int SamplingRate = 48000;
            public double Threshold = 30;
            public double MinLength = 3;
            public int WindowLength = 2048;
            public int HopSize = 512;
            public SlicerSettings() { }
        };

        [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
        public unsafe struct Params
        {
            public Params() { }

            public float NoiseScale = 0.3f;                           //噪声修正因子          0-10
            public long Seed = 52468;                                 //种子
            public long SpeakerId = 0;                                //角色ID
            public ulong SrcSamplingRate = 48000;                     //源采样率
            public long SpkCount = 2;                                 //模型角色数
            public float IndexRate = 0;                               //索引比               0-1
            public float ClusterRate = 0;                             //聚类比               0-1
            public float DDSPNoiseScale = 0.8f;                       //DDSP噪声修正因子      0-10
            public float Keys = 0;                                    //升降调               -64-64
            public ulong MeanWindowLength = 2;                        //均值滤波器窗口大小     1-20
            public ulong Pndm = 100;                                  //Diffusion加速倍数    1-200
            public ulong Step = 1000;                                 //Diffusion总步数      1-1000
            public float TBegin = 0;
            public float TEnd = 1;

            //"Pndm" "DDim"
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Sampler = "Pndm";                           //Diffusion采样器

            //"Eular" "Rk4" "Heun" "Pecece"
            [MarshalAs(UnmanagedType.LPWStr)]
            public string ReflowSampler = "Eular";                    //Reflow采样器

            //"Dio" "Harvest" "RMVPE" "FCPE"
            [MarshalAs(UnmanagedType.LPWStr)]
            public string F0Method = "Dio";                           //F0提取算法

            public int UseShallowDiffusion = 0;                       //使用浅扩散
            public void* _VocoderModel = null;

            public void SetVocoder(ref VocoderModel Vocoder)
            {
                _VocoderModel = Vocoder.GetModel();
            }
        };

        [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
        public unsafe struct DiffusionSvcPaths
        {
            public DiffusionSvcPaths() { }

            [MarshalAs(UnmanagedType.LPWStr)]
            public string Encoder = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string Denoise = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string Pred = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string After = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string Alpha = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string Naive = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string DiffSvc = "";
        };

        [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
        public unsafe struct ReflowSvcPaths
        {
            public ReflowSvcPaths() { }

            [MarshalAs(UnmanagedType.LPWStr)]
            public string Encoder = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string VelocityFn = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string After = "";
        };

        [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
        public unsafe struct VitsSvcPaths
        {
            public VitsSvcPaths() { }

            [MarshalAs(UnmanagedType.LPWStr)]
            public string VitsSvc = "";
        };

        [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
        public unsafe struct LibSvcClusterConfig
        {
            public LibSvcClusterConfig() { }

            public long ClusterCenterSize = 10000;

            [MarshalAs(UnmanagedType.LPWStr)]
            public string Path = "";

            //"KMeans" "Index"
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Type = "";
        };

        [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
        public unsafe struct LibSvcHparams
        {
            public LibSvcHparams() { }

            [MarshalAs(UnmanagedType.LPWStr)]

            //"SoVits2.0" "SoVits3.0" "SoVits4.0" "SoVits4.0-DDSP" "RVC" "DiffSvc" "DiffusionSvc"
            public string TensorExtractor = "";

            [MarshalAs(UnmanagedType.LPWStr)]
            public string HubertPath = "";

            public DiffusionSvcPaths DiffusionSvc;
            public VitsSvcPaths VitsSvc;
            public ReflowSvcPaths ReflowSvc;
            public LibSvcClusterConfig Cluster;

            public int SamplingRate = 22050;

            public int HopSize = 320;
            public long HiddenUnitKDims = 256;
            public long SpeakerCount = 1;
            public int EnableCharaMix = 0;
            public int EnableVolume = 0;
            public int VaeMode = 1;

            public long MelBins = 128;
            public long Pndms = 100;
            public long MaxStep = 1000;
            public float SpecMin = -12;
            public float SpecMax = 2;
            public float Scale = 1000;
        };

        public delegate void CallbackProgress(ulong arg1, ulong arg2);

        private LibSvc() 
        {
            Init();
        }

        private static LibSvc? instance = null;

        public static LibSvc Factory
        {
            get { 
                instance ??= new LibSvc();
                return instance;
            }
        }

        public void Init()
        {
            LibSvcInit();
            EnableFileLogger(false);
            SetF0PredictorEnv(8, 0, 0);
        }

        public void SetF0PredictorEnv(uint ThreadCount, uint DeviceID, Device Provider)
        {
            if (LibSvcSetGlobalEnv(ThreadCount, DeviceID, (uint)Provider) == 0)
                return;
            throw new Exception(GetError(0));
        }

        public static void SetMaxErrorCount(ulong Count)
        {
            LibSvcSetMaxErrorCount(Count);
        }

        public static string GetError(ulong Index)
        {
            return LibSvcGetError(Index);
        }

        public Int16Vector ReadAudio(ref string _Path, int _SamplingRate)
        {
            Int16Vector Ret = new();
            if (LibSvcReadAudio(_Path, _SamplingRate, Ret.Object) == 0)
                return Ret;
            throw new Exception(GetError(0));
        }

        public UInt64Vector SliceAudio(
            ref Int16Vector Audio,
            ref SlicerSettings Setting
        )
        {
            UInt64Vector Ret = new();
            if(LibSvcSliceAudio(Audio.Object, ref Setting, Ret.Object) == 0)
                return Ret;
            throw new Exception(GetError(0));
        }

        public Slices Preprocess(
            ref Int16Vector Audio,
            ref UInt64Vector SlicePos,
            int SamplingRate = 48000,
            int HopSize = 480,
            double Threshold = 30,
            string F0Method = "Dio"
        )
        {
            Slices Ret = new();
            if (LibSvcPreprocess(Audio.Object, SlicePos.Object, SamplingRate, HopSize, Threshold, F0Method, Ret.Object) == 0) 
                return Ret;
            throw new Exception(GetError(0));
        }

        public Mel Stft(
            ref Int16Vector Audio,
            int SamplingRate,
            int Hopsize,
            int MelBins
        )
        {
            Mel Ret = new();
            if (LibSvcStft(Audio.Object, SamplingRate, Hopsize, MelBins, Ret.Object) == 0)
                return Ret;
            throw new Exception(GetError(0));
        }

        public VitsModel LoadVitsSvcModel(
            ref LibSvcHparams _Config,
            ref CallbackProgress _ProgressCallback,
            Device _ExecutionProvider,
            uint _DeviceID,
            uint _ThreadCount
        )
        {
            void* Model = LibSvcLoadModel(0, ref _Config, _ProgressCallback, (uint)_ExecutionProvider, _DeviceID, _ThreadCount);
            if(Model != null)
                return new VitsModel(Model);
            throw new Exception(GetError(0));
        }

        public UnionModel LoadUnionSvcModel(
            ref LibSvcHparams _Config,
            ref CallbackProgress _ProgressCallback,
            Device _ExecutionProvider,
            uint _DeviceID,
            uint _ThreadCount
        )
        {
            void* Model = LibSvcLoadModel(1, ref _Config, _ProgressCallback, (uint)_ExecutionProvider, _DeviceID, _ThreadCount);
            if (Model != null)
                return new UnionModel(Model);
            throw new Exception(GetError(0));
        }

        public VocoderModel LoadVocoderModel(ref string _Path)
        {
            void* Model = LibSvcLoadVocoder(_Path);
            if (Model != null)
                return new VocoderModel(Model);
            throw new Exception(GetError(0));
        }

        public void EnableFileLogger(bool _Cond) { LibSvcEnableFileLogger(_Cond); }

        public void WriteAudio(Int16Vector _PCMData, string _OutputPath, int _SamplingRate)
        {
            LibSvcWriteAudioFile(_PCMData.Object, _OutputPath, _SamplingRate);
        }

        [DllImport("libsvc.dll")]
        private static extern void LibSvcInit();

        [DllImport("libsvc.dll")]
        private static extern int LibSvcSetGlobalEnv(uint ThreadCount, uint DeviceID, uint Provider);

        [DllImport("libsvc.dll")]
        private static extern void LibSvcSetMaxErrorCount(ulong Count);

        [DllImport("libsvc.dll", CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.BStr)]
        private static extern string LibSvcGetError(ulong Index);

        [DllImport("libsvc.dll")]
        private static extern int LibSvcSliceAudio(
            void* _Audio,
            ref SlicerSettings _Setting,
            void* _Output
        );

        [DllImport("libsvc.dll", CharSet = CharSet.Unicode)]
        private static extern int LibSvcPreprocess(
            void* _Audio,
            void* _SlicePos,
            int _SamplingRate,
            int _HopSize,
            double _Threshold,
            string _F0Method,
            void* _Output
        );

        [DllImport("libsvc.dll")]
        private static extern int LibSvcStft(
            void* _Audio,
            int _SamplingRate,
            int _Hopsize,
            int _MelBins,
            void* _Output
        );

        [DllImport("libsvc.dll", CharSet = CharSet.Unicode)]
        private static extern void* LibSvcLoadModel(
            uint _T,
            ref LibSvcHparams _Config,
            CallbackProgress _ProgressCallback,
            uint _ExecutionProvider,
            uint _DeviceID,
            uint _ThreadCount
	    );

        [DllImport("libsvc.dll", CharSet = CharSet.Unicode)]
        private static extern void* LibSvcLoadVocoder(string VocoderPath);

        [DllImport("libsvc.dll", CharSet = CharSet.Unicode)]
        private static extern int LibSvcReadAudio(string _AudioPath, int _SamplingRate, void* _Output);

        [DllImport("libsvc.dll", CharSet = CharSet.Unicode)]
        private static extern void LibSvcWriteAudioFile(void* _PCMData, string _OutputPath, int _SamplingRate);

        [DllImport("libsvc.dll")]
        private static extern void LibSvcEnableFileLogger(bool _Cond);
    }
}
