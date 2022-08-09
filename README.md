# Shirakana-TTS-CPP
一个基于各种开源TTS项目的完全C++UI化TTS转换软件

使用到的项目的仓库：
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)


目前仅支持Windows平台。

## 使用方法：
    1、在release中下载zip包，解压之
    2、打开ShirakanaTTS.exe
    3、在右上方Mods模块中选择模型，然后点击导入模型
    4、在下方执行模块输入框中输入要转换的文字，支持Symbol中的字符，
    换行为批量转换的分句符号。
    5、点击开始合成，即可开始合成语音，等待进度完成后，可以在右上方
    播放器预览，也可以在右上方直接保存。
    6、可以使用命令行启动：
    Shell：& '.\Tacotron inside.exe' "ModDir" "InputText." "outputDir" "Symbol"
    CMD："Tacotron inside.exe" "ModDir" "InputText." "outputDir" "Symbol"
    其中ModDir为"模型路径\\模型名" 如预置模型的"Mods\\Shiroha\\Shiroha"
    InputText为需要转换的文字（仅支持空格逗号句号以及字母）
    outputDir为输出文件名（不是路径，是文件名，不需要加后缀）
    Symbol见下文
    输出文件默认在tmpDir中

## 模型导入：
    本软件标准化了模型读取模块，模型保存在Mods文件夹下的子文件夹中
    ********.mod文件用于声明模型路径以及其显示名称，以我的预置模型
    为例（Shiroha.mod）
    - Mdid:Shiroha         路径名称即该文件夹下的Shiroha子文件夹
    - Name:鸣濑白羽                                 Mod显示名称
    - Type:Tacotron2         Mod项目名（见下文“支持的model项目”）
    - Symb:_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklm
      nopqrstuvwxyz                         Symbol（设置见下文）

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
    VITS_LJS:    //单角色VITS
        ${Mdid}_ljs.onnx 
    VITS_VCTK:   //多角色VITS
        ${Mdid}_vctk.onnx
    
## Symbol的设置
    Symb:_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
    打开你训练模型的项目，打开text\symbol.py，如图
![image](https://user-images.githubusercontent.com/40709280/183290732-dcb93323-1061-431b-aafa-c285a3ec5e82.png)
    按照划线的List顺序将上面的4个字符串连接即可
