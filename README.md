# 强调：导出HuBert时inputname应该为source，outputname应该为embed；Diffusion使用的hifigan的inputname应该为c和f0，outputname应该是audio，Tacotron2使用的声码器inputname应该为x，outname应该为audio

# 用户协议：
## 本项目暂时停止新版本（V2）的开源，仅提供exe可执行文件，以后会恢复开源。V1版本在另一个分支。

使用本项目进行二创时请标注本项目仓库地址或作者bilibili空间地址：https://space.bilibili.com/108592413

## 使用该项目代表你同意如下几点：
- 1、你愿意自行承担由于使用该项目而造成的一切后果。
- 2、你承诺不会出售该程序以及其附属模型，若由于出售而造成的一切后果由你自己承担。
- 3、你不会使用之从事违法活动，若从事违法活动，造成的一切后果由你自己承担。
- 4、禁止用于任何商业游戏、低创游戏以及Galgame制作，不反对无偿的精品游戏制作以及Mod制作。
- 5、禁止使用该项目及该项目衍生物以及发布模型等制作各种电子垃圾（比方说AIGalgame，AI游戏制作等）

## Q&A：
### Q：该项目以后会收费吗？
A：该项目为永久免费、暂时闭源的项目，如果在其他地方存在本软件的收费版本，请立即举报且不要购买，本软件永久免费。如果想用疯狂星期四塞满白叶，可以前往爱发癫 https://afdian.net/a/NaruseMioShirakana 
### Q：是否提供有偿模型代训练？
A：原则上不提供，训练TTS模型比较简单，没必要花冤枉钱，按照网上教程一步一步走就可以了。提供免费的Onnx转换。
### Q：电子垃圾评判标准是什么？
A：
+ 1、原创度。自己的东西在整个项目中的比例（对于AI来说，使用完全由你独立训练模型的创作属于你自己；使用他人模型的创作属于别人）。涵盖的方面包括但不限于程序、美工、音频、策划等等。举个例子，套用Unity等引擎模板换皮属于电子垃圾。
+ 2、开发者态度。作者开发的态度是不是捞一波流量和钱走人或单纯虚荣。比方说打了无数的tag，像什么“国产”“首个”“最强”“自制”这种引流宣传，结果是非常烂或是平庸的东西，且作者明显没有好好制作该项目的想法，属于电子垃圾。
+ 3、反对一切使用未授权的数据集训练出来的AI模型商用的行为。 
### Q：技术支持？
A：如果能够确定你做的不是电子垃圾，我会提供一些力所能及的技术支持。 

## 以上均为君子协议，要真要做我也拦不住，但还是希望大家自觉，有这个想法的也希望乘早改悔罢
---

# Moe Speech Synthesis
一个基于各种开源TTS项目的完全C++UI化Speech Synthesis软件

支持的项目的仓库：
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)
- [VITS](https://github.com/jaywalnut310/vits)
- [SoVits](https://github.com/innnky/so-vits-svc/tree/32k)
- [DiffSvc](https://github.com/prophesier/diff-SVC)

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
    "Hifigan": "hifigan",
    "SoVits3": false,
    "Hubert": "hubert",
    "Pndm" : 100,
    "MelBins" : 128,
    "Characters" : ["鳴瀬しろは","空門蒼","鷹原うみ","紬ヴェンダース","神山識","水織静久","野村美希","久島鴎","岬鏡子"]
}
// 其中必填项目为Folder,Name,Type,Rate
// TTS（Tacotron2，Vits，串联用模型）需要填写Symbol,Cleaner
// 无自带声码器的项目（Tacotron2，DiffSvc）需要填写Hifigan（hifigan模型应该放置于hifigan，该项设置为模型文件名（不带后缀））
// VC（Sovits，DiffSvc）需要填写Hop和Hubert（Hubert放到Hubert文件夹下）
// SoVits3为Sovits3.0的标记，如果该模型基于SoVits3.0训练则需要填写为true
// DiffSvc需要填写Pndm（就是你导出模型时的加速倍率），MelBins（在你的模型config.yaml里面的前几项有一个带mel_bins的一项）
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
// DiffSvc:
    ${Folder}_diffSvc.onnx
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
