# Tacotron2-CPP
一个基于Tacotron2（Text to mel）和Hifigan（mel to wav）的TTS转换软件

以上两个项目的原仓库：[DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)

目前仅支持Windows平台。

## 使用方法：
    1、在release中下载zip包，解压之
    2、打开Tacotron2MoeTTSCpp.exe
    3、在右上方Mods模块中选择模型，然后点击导入模型
    4、在下方执行模块输入框中输入要转换的文字，仅支持  字母(大小写)  ' '
    ','  '.'(结束符号)  '~'(分句符号)，不支持一切全角字符）
    5、点击开始合成，即可开始合成语音，等待进度完成后，会提示保存文件

## 模型导入：
    本软件标准化了模型读取模块，模型保存在Mods文件夹下的子文件夹中
    mods.modlist文件用于声明模型路径以及其显示名称，以我的预置模型
    为例，Shiroha:鸣濑白羽，冒号(半角)之前是路径名称(即该文件夹下的
    Shiroha子文件夹)，冒号后面是其显示名称；子文件夹中有4个文件，分
    别是xxx.png，xxx_decoder_iter.onnx，xxx_encoder.onnx 以及一
    个,xxx_postnet.onnx，其中xxx为冒号之前的字符串。

## 模型制作：
    Tacotron：
    详见 下文
    Hifigan：
    导入你的Hifigan模型
    在Python中使用如下代码：
    torch.onnx.export(generator, mel_outputs.float(), "hifigan.onnx",
                          opset_version=opset_version,
                         do_constant_folding=True,
                          input_names=["x"],
                          output_names=["audio"],
                          dynamic_axes={"x":   {0: "batch_size", 2: "mel_seq"},
                                        "audio": {0: "batch_size", 2: "audio_seq"}})

[Pytorch2Onnx](https://github.com/NVIDIA/DeepLearningExamples/tree/master/PyTorch/SpeechSynthesis/Tacotron2/)



