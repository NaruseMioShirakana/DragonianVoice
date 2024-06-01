using LibSvcApi;

LibSvc.LibSvcHparams Config = new();
Config.TensorExtractor = "DiffusionSvc";
Config.SamplingRate = 44100;
Config.HopSize = 512;
Config.HubertPath = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\hubert\\vec-768-layer-12.onnx";
Config.SpeakerCount = 2;
Config.HiddenUnitKDims = 768;
Config.EnableCharaMix = 1;
Config.EnableVolume = 1;
Config.MelBins = 128;
Config.DiffusionSvc.After = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\Models\\ShallowDiffusion\\ShallowDiffusion_after.onnx";
Config.DiffusionSvc.Alpha = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\Models\\ShallowDiffusion\\ShallowDiffusion_alpha.onnx";
Config.DiffusionSvc.Encoder = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\Models\\ShallowDiffusion\\ShallowDiffusion_encoder.onnx";
Config.DiffusionSvc.Denoise = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\Models\\ShallowDiffusion\\ShallowDiffusion_denoise.onnx";
Config.DiffusionSvc.Naive = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\Models\\ShallowDiffusion\\ShallowDiffusion_naive.onnx";
Config.DiffusionSvc.Pred = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\Models\\ShallowDiffusion\\ShallowDiffusion_pred.onnx";
void PrintProgress(ulong arg1, ulong arg2)
{
    Console.WriteLine(arg1 * 100.0 / 10);
}

LibSvc.CallbackProgress Callback = new LibSvc.CallbackProgress(PrintProgress);

UnionModel Model = LibSvc.Factory.LoadUnionSvcModel(
    ref Config, ref Callback,
    0, 0, 8
);

string AudioPath = "input.wav";
Int16Vector Audio = LibSvc.Factory.ReadAudio(ref AudioPath, 48000);
Console.WriteLine(Audio.Size());

LibSvc.SlicerSettings slicerSettings = new();
UInt64Vector SlicePos = LibSvc.Factory.SliceAudio(ref Audio, ref slicerSettings);
SlicePos.Resize(3);
Console.WriteLine(SlicePos.Size());

Slices slices = LibSvc.Factory.Preprocess(ref Audio, ref SlicePos);
Console.WriteLine(slices.Size());

string VocoderPath = "D:\\VSGIT\\MoeVoiceStudioSvc - Core - Cmd\\x64\\Debug\\hifigan\\nsf_hifigan.onnx";
VocoderModel Vocoder = LibSvc.Factory.LoadVocoderModel(ref VocoderPath);

LibSvc.Params _params = new();
_params.Step = 100;
_params.Pndm = 10;
_params.SetVocoder(ref Vocoder);
ulong Proc = 0;
Slice slice = slices[1];
Audio = Model.Inference(slice, ref _params, ref Proc);
Console.WriteLine((double)slice.SrcLength() * Config.SamplingRate / slicerSettings.SamplingRate);
Console.WriteLine(Audio.Size());
Callback(1, 2);
GC.KeepAlive(Callback);
GC.KeepAlive(Vocoder);

LibSvc.Factory.WriteAudio(Audio, "Out1.wav", Config.SamplingRate);
Audio.Insert(ref Audio);
LibSvc.Factory.WriteAudio(Audio, "Out2.wav", Config.SamplingRate);