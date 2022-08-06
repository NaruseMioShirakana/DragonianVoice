# Shirakana-TTS-CPP
一个基于各种开源TTS项目的完全C++UI化TTS转换软件

使用到的项目的仓库：
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)


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
    ********.mod文件用于声明模型路径以及其显示名称，以我的预置模型
    为例（Shiroha.mod）
    - Mdid:Shiroha         路径名称即该文件夹下的Shiroha子文件夹
    - Name:鸣濑白羽                                 Mod显示名称
    - Type:Tacotron2         Mod项目名（见下文“支持的model项目”）

## 模型制作：
    Tacotron2：
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

- [Pytorch2Onnx(Tacotron2)](https://github.com/NVIDIA/DeepLearningExamples/tree/master/PyTorch/SpeechSynthesis/Tacotron2/)
- [演示视频](https://www.bilibili.com/video/BV1AB4y1t783)

## 支持的model项目
    ${xxx}是什么意思大家都知道吧（）里面的变量详见“模型导入”
    Tacotron2：
        需要在Mod文件夹中加入 
        ${Mdid}_decoder_iter.onnx 
        ${Mdid}_encoder.onnx
        ${Mdid}_postnet.onnx
    VITS:
        ${Mdid}_ljs.onnx 
        ${Mdid}_vctk.onnx
    


