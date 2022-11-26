# 用户协议：
## 使用该项目代表你同意如下几点：
- 1、你愿意自行承担由于使用该项目而造成的一切后果。
- 2、你承诺不会出售该程序以及其附属模型，若由于出售而造成的一切后果由你自己承担。
- 3、你不会使用之从事违法活动，若从事违法活动，造成的一切后果由你自己承担。
- 4、禁止用于任何商业游戏、低创游戏以及Galgame制作，不反对无偿的精品游戏制作以及Mod制作。
- 5、禁止使用该项目及该项目衍生物以及发布模型等制作各种电子垃圾（比方说AIGalgame，AI游戏制作等）

# Mio-TextToSpeech
一个基于各种开源TTS项目的完全C++UI化TTS转换软件

使用到的项目的仓库：
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)

使用的图像素材来源于：
- [SummerPockets](http://key.visualarts.gr.jp/summer/)

目前仅支持Windows平台。

## 使用方法：
    1、在release中下载zip包，解压之
    2、打开Mio-TTS.exe
    3、在右上方Mods模块中选择模型
    4、在下方执行模块输入框中输入要转换的文字，支持Symbol中的字符，
    换行为批量转换的分句符号。
    5、点击开始合成，即可开始合成语音，等待进度完成后，可以在右上方
    播放器预览，也可以在右上方直接保存。
    6、可以使用命令行启动：（仅1.X版本）
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
    - Folder:Shiroha         路径名称即该文件夹下的Shiroha子文件夹
    - Name:鸣濑白羽                                 Mod显示名称
    - Type:Tacotron2         Mod项目名（见下文“支持的model项目”）
    - Symbol:_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklm
      nopqrstuvwxyz                         Symbol（设置见下文）
    - Cleaner:characters_cleaner                Cleaner（详见下文）
    - 角色名1                    这里往下的只有VITS多角色需要设定
    - 角色名2
    - 角色名n

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
    至于VITS的模型，可以在Issue提交分享地址，我帮忙做（Tac的也可以）

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
        ${Mdid}_solo.pt（Jit模型） 
    VITS_VCTK:   //多角色VITS
        ${Mdid}_mul.pt（Jit模型）
    
## Symbol的设置
    Symb:_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
    打开你训练模型的项目，打开text\symbol.py，如图
![image](https://user-images.githubusercontent.com/40709280/183290732-dcb93323-1061-431b-aafa-c285a3ec5e82.png)
    按照划线的List顺序将上面的4个字符串连接即可

## Cleaner的设置
    Cleaner请放置于根目录的Cleaners文件夹内，应该是一个可执行
    程序（.exe）
    Cleaner的程序编写语言不限，但执行参数必须为C++执行参数标准
    的argv（可以使用控制台 "xxx.exe" "input_str"）这种格式执行
    执行参数只接受一个（输入的文本，可以是UTF16和UTF32字符）
    输出只有一个（标准控制台输出）也就是经过Cleaner处理后的文本。
    在前文中需要设定的cleaner，即为此exe的文件名（不带路径拓展名）

## 已经制作好的模型
[Github](https://github.com/FujiwaraShirakana/ShirakanaTTSMods)
