import soundfile
from onnx_inference import OnnxRVC

hop_size = 512
sampling_rate = 40000
model_path = "ShirohaRVC.onnx" #模型的完整路径
vec_name = "vec-256-layer-9" #内部自动补齐为 f"pretrained/{vec_name}.onnx" 需要onnx的vec模型
wav_path = "" #输入路径或ByteIO实例
out_path = "" #输出路径或ByteIO实例

model = OnnxRVC(model_path, vec_path=vec_name, sr=sampling_rate, hop_size=hop_size, device="cpu")

audio = model.inference(wav_path, 0)

soundfile.write(out_path, audio, 40000)