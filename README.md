# 用户协议：
## 本项目暂时停止新版本（V2）的开源，仅提供exe可执行文件，以后会恢复开源。V1版本在另一个分支

## 使用该项目代表你同意如下几点：
- 1、你愿意自行承担由于使用该项目而造成的一切后果。
- 2、你承诺不会出售该程序以及其附属模型，若由于出售而造成的一切后果由你自己承担。
- 3、你不会使用之从事违法活动，若从事违法活动，造成的一切后果由你自己承担。
- 4、禁止用于任何商业游戏、低创游戏以及Galgame制作，不反对无偿的精品游戏制作以及Mod制作。
- 5、禁止使用该项目及该项目衍生物以及发布模型等制作各种电子垃圾（比方说AIGalgame，AI游戏制作等）

# Moe Speech Synthesis
一个基于各种开源TTS项目的完全C++UI化Speech Synthesis软件

支持的项目的仓库：
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)
- [VITS](https://github.com/jaywalnut310/vits)
- [SoVits](https://github.com/innnky/so-vits-svc/tree/32k)
- [DiffSvc](https://github.com/prophesier/diff-SVC)  //TODO

使用的图像素材来源于：
- [SummerPockets](http://key.visualarts.gr.jp/summer/)

目前仅支持Windows，未来可能移植Android，并为Linux用户提供软件，暂无开发Mac与Ios的计划。

## 使用方法：
    1、在release中下载zip包，解压之

    2、打开MoeSS.exe

    3、在左上方Mods模块中选择模型

    4、在下方输入框中输入要转换的文字，点击“清理”可以执行文本Cleaner，换行为批量转换的分句符号。（SoVits需要输入音频路径）

    5、点击开始合成，即可开始合成语音，等待进度完成后，可以在右上方播放器预览，也可以在右上方直接保存。

    6、可以使用命令行启动：（仅1.X版本）
    Shell：& '.\xxx.exe' "ModDir" "InputText." "outputDir" "Symbol"
    CMD："xxx.exe" "ModDir" "InputText." "outputDir" "Symbol"
    其中ModDir为"模型路径\\模型名" 如预置模型的"Mods\\Shiroha\\Shiroha"
    InputText为需要转换的文字（仅支持空格逗号句号以及字母）
    outputDir为输出文件名（不是路径，是文件名，不需要加后缀）
    Symbol见下文
    输出文件默认在tmpDir中

## 模型导入：
```json
// 本软件标准化了模型读取模块，模型保存在Mods文件夹下的子文件夹中********.json文件用于声明模型路径以及其显示名称，以我的模型为例（SummerPockets.json）
{
    "Folder" : "SummerPockets",
    "Name" : "SummerPocketsReflectionBlue",
    "Type" : "VITS_VCTK",
    "Symbol" : "_,.!?-~…AEINOQUabdefghijkmnoprstuvwyzʃʧʦ↓↑" ,
    "Cleaner" : "LowerCharacters",
    "Rate" : 22050,
    "Hop" : 0,
    "Hifigan": "",
    "Hubert": "3.0",
    "Characters" : ["鳴瀬しろは","空門蒼","鷹原うみ","紬ヴェンダース","神山識","水織静久","野村美希","久島鴎","岬鏡子"]
}
// 其中必填项目为Folder,Name,Type,Rate
// TTS（Tacotron2，Vits，串联用模型）需要填写Symbol,Cleaner
// 无自带声码器的项目（Tacotron2，DiffSvc）需要填写Hifigan（hifigan模型应该放置于hifigan，该项设置为模型文件名（不带后缀））
// VC（Sovits，DiffSvc）需要填写Hop和Hubert，如果你的项目为sovits3.0，则需要将Hubert设置为3.0，如果不是，则此行可以填写任意字符串
// 含多角色embidding的（Vits多人模型，Sovits）需要填写Characters
```

## 支持的model项目
```cxx 
// ${xxx}是什么意思大家应该都知道吧，总之以下是多个不同项目需要的模型文件（需要放置在对应的模型文件夹下）。
// Tacotron2：
    ${Folder}_decoder_iter.onnx
    ${Folder}_encoder.onnx
    ${Folder}_postnet.onnx
// VITS_LJS:    单角色VITS
    ${Folder}_dec.onnx
    ${Folder}_flow.onnx
    ${Folder}_enc_p.onnx
    ${Folder}_dp.onnx 
// VITS_VCTK:   多角色VITS
    ${Folder}_dec.onnx
    ${Folder}_emb.onnx
    ${Folder}_flow.onnx
    ${Folder}_enc_p.onnx
    ${Folder}_dp.onnx
// SoVits:
    ${Folder}_SoVits.onnx
```
## Symbol的设置
    例如：_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
    打开你训练模型的项目，打开text\symbol.py，如图按照划线的List顺序将上面的4个字符串连接即可
![image](https://user-images.githubusercontent.com/40709280/183290732-dcb93323-1061-431b-aafa-c285a3ec5e82.png)


## Cleaner的设置
```cxx
/*
Cleaner请放置于根目录的Cleaners文件夹内，应该是一个按照要求定义的动态库（.dll），dll应当命名为Cleaner名，Cleaner名即为模型定义Json文件中Cleaner一栏填写的内容。
所有的插件dll需要定义以下函数，函数名必须为PluginMain，Dll名必须为插件名（或Cleaner名）：
*/
const wchar_t* PluginMain(const wchar_t*);
// 该接口只要求输入输出一致，并不要求功能一致，也就是说，你可以在改Dll中实现任何想要的功能，比方说ChatGpt，机器翻译等等。
// 以ChatGpt为例，PluginMain函数传入了一个输入字符串input，将该输入传入ChatGpt，再将ChatGpt的输出传入PluginMain，最后返回输出。
wchar_t* PluginMain(wchar_t* input){
    wchar_t* tmpOutput = ChatGpt(input);
    return Clean(tmpOutput);
}
// 注意：导出dll时请使用 extern "C" 关键字来防止C++语言的破坏性命名。
```

## 杂项
- [已经制作好的模型](https://github.com/FujiwaraShirakana/ShirakanaTTSMods)
- [演示视频](https://www.bilibili.com/video/BV1bD4y1V7zu)
