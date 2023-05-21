# 模型需要转换为ONNX模型，转换ONNX的程序我已经pull到每个项目的源仓库了，PTH不能直接用！！！！！！！！！！！！！

---

选择语言：[简体中文](README.md)    [English](README_en.md)

目前该仓库的项目为MoeSS，MoeVoiceStudio会在本月或下月发布，敬请期待。

作者的其他项目：[AiToolKits](https://github.com/NaruseMioShirakana/ShirakanaNoAiToolKits)

---

# 免责声明
本项目为开源、离线的项目，本项目的所有开发者以及维护者（以下简称贡献者）对本项目没有控制力。本项目的贡献者从未向任何组织或个人提供包括但不限于数据集提取、数据集加工、算力支持、训练支持、推理等一切形式的帮助；本项目的贡献者不知晓也无法知晓使用者使用该项目的用途。故一切基于本项目合成的音频都与本项目贡献者无关。一切由此造成的问题由使用者自行承担。

本项目本身不具备任何语音合成的功能，一切的功能均需要由使用者自行训练模型并自行将其制作为Onnx模型，且模型的训练与Onnx模型的制作均与本项目的贡献者无关，均为使用者自己的行为，本项目贡献者未参与一切使用者的模型训练与制作。

本项目为完全离线状态下运行，无法采集任何用户信息，也无法获取用户的输入数据，故本项目贡献者对用户的一切输入以及模型不知情，因此不对任何用户输入负责。

本项目也没有附带任何模型，任何二次发布所附带的模型以及用于此项目的模型均与此项目开发者无关。

---
# 由于OnnxRuntime引发的问题

使用GPU（CUDA）版本，请安装12.0以下，11.0版本以上的CUDA驱动程序，83.0版本以下的CUDNN动态库，并按照网上的教程安装。

为什么有这样的要求？那就得问CUDA，CUDNN背后的英伟达公司以及OnnxRuntime的官方了，这两个问题都是由CUDA驱动的一些特性和OnnxRuntime的一些问题引起的。

之前的版本不支持中文路径是什么原因？实际上就是上述问题的体现。项目本体是支持中文路径的，不过它底层的OnnxRuntime是不支持中文路径的，因为Windows版本的OnnxRuntime使用了Win32Api的A系列函数，A系列函数都是不支持非ANSI编码的路径的。这个问题并不是我能够解决的也不是我应该解决的，只有OnnxRuntime官方修复了这个BUG才可以解决，不过好在最新的OnnxRuntime使用了W系列函数，解决了中文路径的这个问题。

模型加载时候遇到弹窗报错，就是由于上述问题引起（主要是没有安装或者没有按照要求安装CUDA和CUDNN），如果引发了这些问题，可以前往https://github.com/microsoft/onnxruntime Onnx官方仓库的Issue查找解决办法。

建议使用CPU版本，CPU版本推理速度也比较可观，且没有其他问题。

---

# 前置模型（与所支持的几个项目无关）：
停止更新（由于下载和上传速度）: [Vocoder & HiddenUnitBert](https://github.com/NaruseMioShirakana/RequireMent-Model-For-MoeSS) 

最新仓库地址 : [HuggingFace](https://huggingface.co/NaruseMioShirakana/MoeSS-SUBModel) 

自己导出前置：
- HuBert：input_names应该为["source"]，output_names应该为["embed"]，dynamic_axes应当为{"source":[0,2],}
- Diffusion模型使用的hifigan：input_names应该为["c","f0"]，output_names应该为["audio"]，dynamic_axes应当为{"c":[0,1],"f0":[0,1],}
- Tacotron2使用的hifigan：input_names应该为["x"]，output_names应该为["audio"]，dynamic_axes应当为{"x":[0,1],}

---
# 用户协议：
## 使用该项目你必须同意以下条款，若不同意则禁止使用该项目：
- 1、你必须自行承担由于使用该项目而造成的一切后果。
- 2、禁止出售该程序。
- 3、使用该项目时，你必须自觉遵守当地的法律法规，禁止使用该项目从事违法活动。
- 4、禁止用于任何商业游戏、低创游戏以及Galgame制作，不反对无偿的精品游戏制作以及Mod制作。
- 5、禁止使用该项目及该项目衍生物以及发布模型等制作各种电子垃圾（比方说AIGalgame，AI游戏制作等）
- 6、禁止一切政治相关内容。
- 7、你使用该项目生成的一切内容均与该项目开发者无关。
---

## Q&A：
### Q：该项目以后会收费吗？
    A：该项目永久开源免费，如果在其他地方存在本项目的收费版本，请立即举报且不要购买，本项目永久免费。如果想用疯狂星期四塞满白叶，可以前往爱发癫 https://afdian.net/a/NaruseMioShirakana 
### Q：是否提供有偿模型代训练？
    A：不提供，训练模型比较简单，没必要花冤枉钱，按照网上教程一步一步走就可以了。
### Q：电子垃圾评判标准是什么？
    A：1、原创度。自己的东西在整个项目中的比例（对于AI来说，使用完全由你独立训练模型的创作属于你自己；使用他人模型的创作属于别人）。涵盖的方面包括但不限于程序、美工、音频、策划等等。举个例子，套用Unity等引擎模板换皮属于电子垃圾。

    2、开发者态度。作者开发的态度是不是捞一波流量和钱走人或单纯虚荣。比方说打了无数的tag，像什么“国产”“首个”“最强”“自制”这种引流宣传，结果是非常烂或是平庸的东西，且作者明显没有好好制作该项目的想法，属于电子垃圾。
    
    3、反对一切使用未授权的数据集训练出来的AI模型商用的行为。 
### Q：技术支持？
    A：如果能够确定你做的不是电子垃圾，同时合法合规，没有严重的政治错误，我会提供一些力所能及的技术支持。 
---

# Moe Voice Studio
本项目是专注于二次元亚文化圈，面向动漫爱好者的语音辅助软件。

支持的Net：
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)
- [VITS](https://github.com/jaywalnut310/vits)
- [SoVits](https://github.com/innnky/so-vits-svc/tree/32k)
- [DiffSvc](https://github.com/prophesier/diff-SVC)
- [DiffSinger](https://github.com/openvpi/DiffSinger)
- [RVC](https://github.com/liujing04/Retrieval-based-Voice-Conversion-WebUI)
- [FishDiffusion](https://github.com/fishaudio/fish-diffusion)

使用的图像素材来源于：
- [SummerPockets](http://key.visualarts.gr.jp/summer/)

目前仅支持Windows

---
## 使用方法：
    1、在release中下载项目压缩包，解压之

    2、在上文 [Vocoder & HiddenUnitBert] 仓库中下载相应的前置模型或附加模块，并放置到相应文件夹，前置模型与项目的对应关系会在下文提到

    3、将模型放置在Mods文件夹中，在左上方模型选择模块中选择模型，标准模型结构请查阅下文“支持的项目”

    4、在下方输入框中输入要转换的文字，点击“启用插件”可以执行文本Cleaner，换行为批量转换的分句符号（SoVits/DiffSvc需要输入音频路径，DiffSinger需要输入ds或json项目文件的路径）

    5、点击开始合成，即可开始合成语音，等待进度完成后，可以在右上方播放器预览，也可以在右上方直接保存
---
## 模型制作：
- 本项目标准化了模型读取模块，模型保存在Mods文件夹下的子文件夹中。********.json为模型的配置文件，需要自行按照模板编写，同时需要自行将模型转换为Onnx。

### 通用参数(不管是啥模型都必须填的，不填就不识别)：
- Folder：保存模型的文件夹名
- Name：模型在UI中的显示名称
- Type：模型类别
- Rate：采样率（必须和你训练时候的一模一样，不明白原因建议去学计算机音频相关的知识）
### Tacotron2：
```jsonc
{
    "Folder" : "Atri",
    "Name" : "亚托莉-Tacotron2",
    "Type" : "Tacotron2",
    "Rate" : 22050,
    "Symbol" : "_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
    "Cleaner" : "",
    "AddBlank": false,
    "Hifigan": "hifigan"
}
//Symbol：模型的Symbol，不知道Symbol是啥的建议多看几个视频了解了解TTS的基础知识，这一项在Tacotron2中必须填。
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hifigan：Hifigan模型名，必须填且必须将在前置模型中下载到的hifigan放置到hifigan文件夹
//AddBlank：是否在音素之间插0作为分隔
```
### Vits：
```jsonc
{
    "Folder" : "SummerPockets",
    "Name" : "SummerPocketsReflectionBlue",
    "Type" : "Vits",
    "Rate" : 22050,
    "Symbol" : "_,.!?-~…AEINOQUabdefghijkmnoprstuvwyzʃʧʦ↓↑ ",
    "Cleaner" : "",
    "AddBlank": true,
    "Emotional" : true,
    "EmotionalPath" : "all_emotions",
    "Characters" : ["鳴瀬しろは","空門蒼","鷹原うみ","紬ヴェンダース","神山識","水織静久","野村美希","久島鴎","岬鏡子"]
}
//Symbol：模型的Symbol，不知道Symbol是啥的建议多看几个视频了解了解TTS的基础知识，这一项在Vits中必须填。
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//AddBlank：是否在音素之间插0作为分隔（大多数Vits模型必须为true）
//Emotional：是否加入情感向量
//EmotionalPath：情感向量npy文件名
```
### Pits：
```jsonc
{
    "Folder" : "SummerPockets",
    "Name" : "SummerPocketsReflectionBlue",
    "Type" : "Pits",
    "Rate" : 22050,
    "Symbol" : "_,.!?-~…AEINOQUabdefghijkmnoprstuvwyzʃʧʦ↓↑ ",
    "Cleaner" : "",
    "AddBlank": true,
    "Emotional" : true,
    "EmotionalPath" : "all_emotions",
    "Characters" : ["鳴瀬しろは","空門蒼","鷹原うみ","紬ヴェンダース","神山識","水織静久","野村美希","久島鴎","岬鏡子"]
}
//Symbol：模型的Symbol，不知道Symbol是啥的建议多看几个视频了解了解TTS的基础知识，这一项在Vits中必须填。
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//AddBlank：是否在音素之间插0作为分隔（大多数Pits模型必须为true）
//Emotional：是否加入情感向量
//EmotionalPath：情感向量npy文件名
```
### RVC：
```jsonc
{
    "Folder" : "NyaruTaffy",
    "Name" : "NyaruTaffy",
    "Type" : "RVC",
    "Rate" : 40000,
    "Hop" : 320,
    "Cleaner" : "",
    "Hubert": "hubert4.0",
    "Diffusion": false,
    "CharaMix": true,
    "Volume": false,
    "HiddenSize": 256,
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
```
### SoVits_3.0_32k：
```jsonc
{
    "Folder" : "NyaruTaffySo",
    "Name" : "NyaruTaffy-SoVits",
    "Type" : "SoVits",
    "Rate" : 32000,
    "Hop" : 320,
    "Cleaner" : "",
    "Hubert": "hubert",
    "SoVits3": true,
    "Diffusion": false,
    "CharaMix": true,
    "Volume": false,
    "HiddenSize": 256,
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
```
### SoVits_3.0_48k：
```jsonc
{
    "Folder" : "NyaruTaffySo",
    "Name" : "NyaruTaffy-SoVits",
    "Type" : "SoVits",
    "Rate" : 48000,
    "Hop" : 320,
    "Cleaner" : "",
    "Hubert": "hubert",
    "SoVits3": true,
    "Diffusion": false,
    "CharaMix": true,
    "Volume": false,
    "HiddenSize": 256,
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
```
### SoVits_4.0：
```jsonc
{
    "Folder" : "NyaruTaffySo",
    "Name" : "NyaruTaffy-SoVits",
    "Type" : "SoVits",
    "Rate" : 44100,
    "Hop" : 512,
    "Cleaner" : "",
    "Hubert": "hubert4.0",
    "SoVits4": true,
    "Diffusion": false,
    "CharaMix": true,
    "Volume": false,
    "HiddenSize": 256,
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
```
### DiffSVC：
```jsonc
{
    "Folder" : "DiffShiroha",
    "Name" : "白羽",
    "Type" : "DiffSvc",
    "Rate" : 44100,
    "Hop" : 512,
    "MelBins" : 128,
    "Cleaner" : "",
    "Hifigan": "nsf_hifigan",
    "Hubert": "hubert",
    "Characters" : [],
    "Pndm" : 100,
    "Diffusion": false,
    "CharaMix": true,
    "Volume": false,
    "HiddenSize": 256,
    "V2" : true
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//MelBins：模型的MelBins，不知道MelBins是啥的建议多看几个视频了解了解梅尔基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Hifigan：Hifigan模型名，必须填且必须将在前置模型中下载到的nsf_hifigan放置到hifigan文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Pndm：加速倍数，如果是V1模型则必填且必须为导出时设置的加速倍率
//V2：是否为V2模型，V2模型就是后来我分4个模块导出的那个
//Diffusion：是否为DDSP仓库下的扩散模型
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
```
### DiffSinger：
```jsonc
{
    "Folder" : "utagoe",
    "Name" : "utagoe",
    "Type" : "DiffSinger",
    "Rate" : 44100,
    "Hop" : 512,
    "Cleaner" : "",
    "Hifigan": "singer_nsf_hifigan",
    "Characters" : [],
    "MelBins" : 128
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hifigan：Hifigan模型名，必须填且必须将在前置模型中下载到的singer_nsf_hifigan放置到hifigan文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//MelBins：模型的MelBins，不知道MelBins是啥的建议多看几个视频了解了解梅尔基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
```

---
## 支持的model项目
```cxx 
// ${xxx}是什么意思大家应该都知道吧，总之以下是多个不同项目需要的模型文件（需要放置在对应的模型文件夹下）。
// Tacotron2：
    ${Folder}_decoder_iter.onnx
    ${Folder}_encoder.onnx
    ${Folder}_postnet.onnx
// Vits:    单角色VITS
    ${Folder}_dec.onnx
    ${Folder}_flow.onnx
    ${Folder}_enc_p.onnx
    ${Folder}_dp.onnx 
// Vits:   多角色VITS
    ${Folder}_dec.onnx
    ${Folder}_emb.onnx
    ${Folder}_flow.onnx
    ${Folder}_enc_p.onnx
    ${Folder}_dp.onnx
// SoVits:
    ${Folder}_SoVits.onnx
// RVC:
    ${Folder}_RVC.onnx
// DiffSvc:
    ${Folder}_diffSvc.onnx
// DiffSvc: V2
    ${Folder}_encoder.onnx
    ${Folder}_denoise.onnx
    ${Folder}_pred.onnx
    ${Folder}_after.onnx
// DiffSinger: OpenVpiVersion
    ${Folder}_diffSinger.onnx
// DiffSinger: 
    ${Folder}_encoder.onnx
    ${Folder}_denoise.onnx
    ${Folder}_pred.onnx
    ${Folder}_after.onnx
```
---
## Symbol的设置
    例如：_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
    打开你训练模型的项目，打开text\symbol.py，如图按照划线的List顺序将上面的4个字符串连接即可
![image](https://user-images.githubusercontent.com/40709280/183290732-dcb93323-1061-431b-aafa-c285a3ec5e82.png)

---
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

---
## 依赖列表
- [FFmpeg](https://ffmpeg.org/)
- [World](https://github.com/JeremyCCHsu/Python-Wrapper-for-World-Vocoder)
- [rapidJson](https://github.com/Tencent/rapidjson)

---
## 📚 相关法规

#### 使用该项目的任何组织或个人都应当遵守包括但不限于以下的法律。

#### 《民法典》

##### 第一千零一十九条 

任何组织或者个人不得以丑化、污损，或者利用信息技术手段伪造等方式侵害他人的肖像权。未经肖像权人同意，不得制作、使用、公开肖像权人的肖像，但是法律另有规定的除外。
未经肖像权人同意，肖像作品权利人不得以发表、复制、发行、出租、展览等方式使用或者公开肖像权人的肖像。
对自然人声音的保护，参照适用肖像权保护的有关规定。

#####  第一千零二十四条 

【名誉权】民事主体享有名誉权。任何组织或者个人不得以侮辱、诽谤等方式侵害他人的名誉权。  

#####  第一千零二十七条

【作品侵害名誉权】行为人发表的文学、艺术作品以真人真事或者特定人为描述对象，含有侮辱、诽谤内容，侵害他人名誉权的，受害人有权依法请求该行为人承担民事责任。
行为人发表的文学、艺术作品不以特定人为描述对象，仅其中的情节与该特定人的情况相似的，不承担民事责任。  

#### 《[中华人民共和国宪法](http://www.gov.cn/guoqing/2018-03/22/content_5276318.htm)》

#### 《[中华人民共和国刑法](http://gongbao.court.gov.cn/Details/f8e30d0689b23f57bfc782d21035c3.html?sw=%E4%B8%AD%E5%8D%8E%E4%BA%BA%E6%B0%91%E5%85%B1%E5%92%8C%E5%9B%BD%E5%88%91%E6%B3%95)》

#### 《[中华人民共和国民法典](http://gongbao.court.gov.cn/Details/51eb6750b8361f79be8f90d09bc202.html)》

---
## 💪 Thanks to all contributors for their efforts
<a href="https://github.com/NaruseMioShirakana/MoeSS/graphs/contributors" target="_blank">
  <img src="https://contrib.rocks/image?repo=NaruseMioShirakana/MoeSS" />
</a>
