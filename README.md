<div align="center">

![image](logo/logo256(AIGen).png)
# DragonianVoice
[中文](README.md) | [English](README_en.md)

</div>

> 本仓库已经暂时停止了UI项目的维护，以后可能会成为一个纯Lib项目。

> 本仓库为：1、TTS（Tacotron2、Vits、EmotionalVits、BERTVits2、GPtSoVits）；2、SVC（SoVitsSvc、RVC、DiffusionSvc、FishDiffusion、ReflowSvc）；3、SVS（DiffSinger） 的Onnx框架推理仓库，目前支持C/Cpp/C#调用。

> 本仓库的最新版本已经与 [fish-speech](https://github.com/fishaudio/fish-speech) 联动，使用ggml框架重写fish-speech，组成fish-speech.cpp子项目

> 注意：支持SVS的分支：[MoeVoiceStudio](https://github.com/NaruseMioShirakana/MoeVoiceStudio/tree/MoeVoiceStudio) [MoeVoiceStudioCore](https://github.com/NaruseMioShirakana/MoeVoiceStudio/tree/MoeVoiceStudioCore)

> 关于Cuda支持的相关问题可以前往 [OnnxRuntime官方仓库](https://github.com/microsoft/onnxruntime/releases) 查看

> 经过实验，Dml会导致Onnx中一些不支持的算子在使用时并不会报错，而是会返回一个不可预料的结果，所以会导致SoVits3.0和SoVits4.0在DmlEP上的推理结果错误。不过最新的SoVits仓库中的Onnx导出已经替换了这些算子，故SoVits3.0和SoVits4.0恢复支持Dml使用，但是要使用最新（2023/7/17）版本的SoVitsOnnx导出重新导出Onnx模型

> 由于Diffusion和Reflow模型的特性，如果你推理的总步数大于模型训练时最大的步数（实际步数=总步数/加速倍率，这个总步数不是推理时实际走过的步数，而是K_Step），会导致输出音频炸掉或者出现非常大的噪声，所以建议在推理前请仔细观察自己模型配置文件中的MaxStep（或K_Step_Max） 

<details><summary><b>支持的Net：</b></summary>
    
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)
- [Vits](https://github.com/jaywalnut310/vits)
- [EmotionalVits](https://github.com/innnky/emotional-vits)
- [BertVits2](https://github.com/fishaudio/Bert-VITS2)
- [SoVitsSvc (v2/v3/v4)](https://github.com/svc-develop-team/so-vits-svc)
- [RVC](https://github.com/RVC-Project/Retrieval-based-Voice-Conversion-WebUI)
- [DiffSvc](https://github.com/prophesier/diff-SVC)
- [DiffusionSvc (v1/v2)](https://github.com/CNChTu/Diffusion-SVC)
- [FishDiffusion](https://github.com/fishaudio/fish-diffusion)
- [ReflowSvc](https://github.com/prophesier/diff-SVC)
- [DiffSinger](https://github.com/openvpi/DiffSinger)
- [FCPE](https://github.com/CNChTu/MelPE)
- [RMVPE](https://github.com/yxlllc/RMVPE)

</details>

## 目录
- [协议](#用户协议)
  - [免责声明](#免责声明) 
- [说明](#说明)
  - [分支说明](#分支)
- [FAQ问答](#faq)
  - Q: [项目名的来历是什么？](#q-项目名的来历)
  - Q: [这个项目到底有什么作用?](#q-这个项目到底有什么作用)
  - Q: [该项目以后会收费吗?](#q-该项目以后会收费吗)
  - Q: [是否提供有偿模型代训练?](#q-是否提供有偿模型代训练)
  - Q: [电子垃圾评判标准是什么?](#q-电子垃圾评判标准是什么)
  - Q: [技术支持?](#q-技术支持)
- [注意事项](#注意事项)
- [使用方法](#使用方法)
- [模型配置](#模型配置)
  - [前置模型](#前置模型) 
  - [配置文件](#配置文件)
- [支持的项目](#支持的项目)
- [其他设置](#其他设置)
  - [Symbol](#symbol)
  - [Cleaner](#cleaner)
- [本地编译](#本地编译)
- [依赖列表](#依赖列表)
- [相关法规](#-相关法规)

## 用户协议：
### 使用该项目你必须同意以下条款，若不同意则禁止使用该项目：
1. 你必须自行承担由于使用该项目而造成的一切后果。
2. 禁止出售该程序。
3. 使用该项目时，你必须自觉遵守当地的法律法规，禁止使用该项目从事违法活动。
4. 禁止用于任何商业游戏、低创游戏[^1]以及Galgame制作，不反对无偿的精品游戏制作以及Mod制作。
5. 禁止使用该项目及该项目衍生物以及发布模型等制作各种电子垃圾[^2] (例如AIGalgame，AI游戏制作等)。
6. 禁止一切政治相关内容。
7. 你使用该项目生成的一切内容均与该项目开发者无关。
8. 本项目开发者主张使用者不具有其所生成“音频本身”的版权，应当划归公共领域；而音频背后涉及到的歌词、乐曲的版权所有者均为该歌词、乐曲的版权所有方。

[^1]: 低创的评判标准为: 大量使用AI生成的任何内容，原创内容较低，意义不明，内容质量低下等。
[^2]: 电子垃圾评判标准: 1、原创度。自己的东西在整个项目中的比例（对于AI来说，使用完全由你独立训练模型的创作属于你自己；使用他人模型的创作属于别人）。涵盖的方面包括但不限于程序、美工、音频、策划等等。例如，套用Unity等引擎模板换皮属于电子垃圾。

## 免责声明
本项目为**开源、离线的项目，本项目的所有开发者以及维护者（以下简称贡献者）对本项目没有控制力**。本项目的贡献者从未向任何组织或个人提供包括但不限于数据集提取、数据集加工、算力支持、训练支持、推理等一切形式的帮助；本项目的贡献者不知晓也无法知晓使用者使用该项目的用途。故一切基于本项目合成的音频都与本项目贡献者无关。**一切由此造成的问题由使用者自行承担**。

本项目本身不具备任何语音合成的功能，只是用于启动使用者自行训练并自行制作为Onnx模型的模型，且模型的训练与Onnx模型的制作均与本项目的贡献者无关，均为使用者自己的行为，本项目贡献者未参与一切使用者的模型训练与制作。

本项目为完全离线状态下运行，无法采集任何用户信息，也无法获取用户的输入数据，故本项目贡献者对用户的一切输入以及模型不知情，因此不对任何用户输入负责。

**本项目也没有附带任何模型，任何二次发布所附带的模型以及用于此项目的模型均与此项目开发者无关。**

## 说明
本项目目前已完全支持自行调用其中方法来实现命令行推理或其他软件，欢迎大家向本项目提PR

作者的其他项目：[AiToolKits](https://github.com/NaruseMioShirakana/ShirakanaNoAiToolKits)

如果想要参加开发，可以加入QQ群:263805400或直接提PR

**模型需要转换为ONNX模型，详情见你选择的项目的源仓库，PTH模型不能直接使用！！！！！！！！！！！！！**

## 分支
本项目的各分支:
- [MoeVoiceStudioCore(主分支)](https://github.com/NaruseMioShirakana/MoeVoiceStudio/tree/MoeVoiceStudioCore) 项目核心
- [MoeVoiceStudio](https://github.com/NaruseMioShirakana/MoeVoiceStudio/tree/MoeVoiceStudio) 本项目的简单GUI实现 基于qt
- [MoeSSV2](https://github.com/NaruseMioShirakana/MoeVoiceStudio/tree/V2) 旧版MoeSSV2版本存档
- [MoeSSV1](https://github.com/NaruseMioShirakana/MoeVoiceStudio/tree/V2) 旧版MoeSSV1版本存档

## FAQ
### Q: 项目名的来历？
<details><summary>A: </b></summary>

> XP至上主义者狂喜，有谁不喜欢龙娘呢

</details>

### Q: 这个项目到底有什么作用？
<details><summary>A: </b></summary>

> 这个项目的开发初衷主要是实现无需环境部属各个语音合成项目，而现在打算制作为一个SVC的辅助编辑器。
> 
> 由于这个项目毕竟是一个"个人的" "不专业"的项目，所以在您拥有更专业的软件，或者您是Python Cli爱好者，又或者您是相关领域大佬。我自知本软件不够专业且很大可能无法满足您的需求甚至对您没有用处。
> 
> 本项目并不是不可替代的项目，相反的本项目的功能您可以使用各种工具替代，我没有奢望本项目成为相关领域的领军项目，我只是怀着一腔热情继续着该项目的开发。但是热情总有消散的一天，但是该项目承诺在我的开   > 发热情完全消散之前会一直保持维护（不管有没有人使用，就算用户数目为0）
> 
> 本项目在设计上可能存在着各种各样的问题，所以也是需要大家积极的点炒饭来帮助我完善功能的，大部分对于功能和体验的优化我都会接受。

</details>

### Q: 该项目以后会收费吗？
<details><summary>A: </b></summary>

> 该项目永久开源免费，如果在其他地方存在本项目的收费版本，请立即举报且不要购买，本项目永久免费。如果想用疯狂星期四塞满白叶，可以前往爱发癫 https://afdian.net/a/NaruseMioShirakana
> 
</details>

### Q: 是否提供有偿模型代训练？
<details><summary>A: </b></summary>

> 不提供，训练模型比较简单，没必要花冤枉钱，按照网上教程一步一步走就可以了。

</details>


### Q: 电子垃圾评判标准是什么？
<details><summary>A: </b></summary>

> 1. 原创度。自己的东西在整个项目中的比例（对于AI来说，使用完全由你独立训练模型的创作属于你自己；使用他人模型的创作属于别人）。涵盖的方面包括但不限于程序、美工、音频、策划等等。举个例子，套用Unity等引擎模板换皮属于电子垃圾。
> 2. 开发者态度。作者开发的态度是不是捞一波流量和钱走人或单纯虚荣。比方说打了无数的tag，像什么“国产”“首个”“最强”“自制”这种引流宣传，结果是非常烂或是平庸的东西，且作者明显没有好好制作该项目的想法，属于电子垃圾。
> 3. 反对一切使用未授权的数据集训练出来的AI模型商用的行为。 

</details>

### Q: 技术支持？
<details><summary>A: </b></summary>

> 如果能够确定你做的不是电子垃圾，同时合法合规，没有严重的政治错误，我会提供一些力所能及的技术支持。 

</details>

## 注意事项
### 由于OnnxRuntime引发的问题
不支持中文路径？实际上项目本体是支持中文路径的，不过2023年3月前版本的OnnxRuntime是不支持中文路径的，因为这些版本的OnnxRuntime使用了Win32Api的A系列函数，A系列函数都是不支持非ANSI编码的路径的。这个问题并不是我能够解决的也不是我应该解决的，只有微软官方才可以修复这个BUG，不过好在最新的OnnxRuntime使用了W系列函数，解决了中文路径问题。

### Cuda版本问题
由于Cuda有着极差的兼容性，导致一个基于Cuda的程序在使用时，必须安装和编译该程序时相同或是未修改Api最近版本的Cuda。这个问题只能等英伟达公司重视兼容性了。

## 使用方法
MoeVoiceStudioCore以Lib的形式提供 使用C++语言调用

按需引用以下对应的类
```cpp
#include <Modules/Models/header/Tacotron.hpp>
#include <Modules/Models/header/Vits.hpp>
#include <Modules/Models/header/VitsSvc.hpp>
#include <Modules/Models/header/DiffSvc.hpp>
#include <Modules/Models/header/DiffSinger.hpp>

InferClass::Tacotron2;
InferClass::Vits;
InferClass::VitsSvc;
InferClass::DiffusionSvc;
InferClass::DiffusionSinger;

/*
构造函数第一个是配置文件json
第二个是进度条回调
第三个是参数回调 (若为TTS 此参数为空即可)
第四个参数为设备
使用调用Inference函数即可
*/
```
模型配置请参见[#模型配置](#模型配置)

demo: [RVC命令行示例](https://github.com/NaruseMioShirakana/MoeVoiceStudio/tree/MoeVoiceStudioCore/CMD-RVC-Onnx-Inference)

## 模型配置
### 前置模型
#### 与所支持的几个项目无关 为深度学习领域的通用模型
停止更新（由于下载和上传速度）: [Vocoder & HiddenUnitBert](https://github.com/NaruseMioShirakana/RequireMent-Model-For-MoeSS) 

停止更新（由于HuggingFace被墙） : [HuggingFace](https://huggingface.co/NaruseMioShirakana/MoeSS-SUBModel) 

最新仓库：[Openi](https://openi.pcl.ac.cn/Shirakana/SubModel)

自己导出前置：
- HuBert：`input_names`应该为`["source"]`，`output_names`应该为`["embed"]`，`dynamic_axes`应当为`{"source":[0,2],}`
- Diffusion模型使用的hifigan：`input_names`应该为`["c","f0"]`，`output_names`应该为`["audio"]`，`dynamic_axes`应当为`{"c":[0,1],"f0":[0,1],}`
- Tacotron2使用的hifigan：`input_names`应该为`["x"]`，`output_names`应该为`["audio"]`，`dynamic_axes`应当为`{"x":[0,1],}`

Vec模型和Hubert模型放在Hubert文件夹下，Hifigan模型放在Hifigan文件夹下
如需使用FCPE或RMVPE这两个F0预测器，则需要在根目录创建F0Predictor文件夹并将onnx模型放置在其中

## 配置文件
- 本项目标准化了模型读取模块，模型保存在Mods文件夹下的子文件夹中。`xxx.json` 为模型的配置文件，需要自行按照模板编写，同时需要自行将模型转换为Onnx。

#### 通用参数(不管是啥模型都必须填的，不填就不识别)：
- `Folder`：保存模型的文件夹名
- `Name`：模型在UI中的显示名称
- `Type`：模型类别
- `Rate`：采样率（必须和你训练时候的一模一样，不明白原因建议去学计算机音频相关的知识）

### 配置示例

<details><summary>Tacotron2:</summary>

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

</details>
<details><summary>Vits:</summary>
    
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
</details>
<details><summary>Pits:</summary>

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
    
</details>
<details><summary>RVC:</summary>
    
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
    "SoVits2": true,
    "ShallowDiffusion" : "NyaruTaffy"
    "HiddenSize": 256,
    "Cluster": "Index"
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//CharaMix：是否使用角色混合轨道
//ShallowDiffusion：SoVits浅扩散模型，须填写ShallowDiffusion模型配置文件名（不带后缀和完整路径），小显存或内存下速度巨慢，效果未知，请根据实际情况决定是否使用）
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
//Cluster：聚类类型，包括"KMeans"和"Index"，KMeans需要前往SoVits仓库将KMeans文件导出为可用格式，放置到模型文件夹；Index同理，需要前往SoVits仓库导出为可用格式（如果是RVC的单角色Index只需要改名为Index-0.index）然后放置到模型文件夹下（有几个角色就有几个Index文件）
```
</details>
<details><summary>SoVits_3.0_32k:</summary>

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
    "ShallowDiffusion" : "NyaruTaffy"
    "Diffusion": false,
    "CharaMix": true,
    "Volume": false,
    "HiddenSize": 256,
    "Cluster": "KMeans"
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//ShallowDiffusion：SoVits浅扩散模型，须填写ShallowDiffusion模型配置文件名（不带后缀和完整路径），小显存或内存下速度巨慢，效果未知，请根据实际情况决定是否使用）
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
//Cluster：聚类类型，包括"KMeans"和"Index"，KMeans需要前往SoVits仓库将KMeans文件导出为可用格式，放置到模型文件夹；Index同理，需要前往SoVits仓库导出为可用格式（如果是RVC的单角色Index只需要改名为Index-0.index）然后放置到模型文件夹下（有几个角色就有几个Index文件）
```
    
</details>
<details><summary>SoVits_3.0_48k:</summary>

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
    "ShallowDiffusion" : "NyaruTaffy"
    "Diffusion": false,
    "CharaMix": true,
    "Volume": false,
    "HiddenSize": 256,
    "Cluster": "KMeans"
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//ShallowDiffusion：SoVits浅扩散模型，须填写ShallowDiffusion模型配置文件名（不带后缀和完整路径），小显存或内存下速度巨慢，效果未知，请根据实际情况决定是否使用）
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
//Cluster：聚类类型，包括"KMeans"和"Index"，KMeans需要前往SoVits仓库将KMeans文件导出为可用格式，放置到模型文件夹；Index同理，需要前往SoVits仓库导出为可用格式（如果是RVC的单角色Index只需要改名为Index-0.index）然后放置到模型文件夹下（有几个角色就有几个Index文件）
```
    
</details>
<details><summary>SoVits_4.0:</summary>
    
```jsonc
{
    "Folder" : "NyaruTaffySo",
    "Name" : "NyaruTaffy-SoVits",
    "Type" : "SoVits",
    "Rate" : 44100,
    "Hop" : 512,
    "Cleaner" : "",
    "Hubert": "hubert4.0",
    "SoVits4.0V2": false,
    "ShallowDiffusion" : "NyaruTaffy"
    "Diffusion" : false,
    "CharaMix" : true,
    "Volume" : false,
    "HiddenSize" : 256,
    "Cluster" : "KMeans"
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：模型的HopLength，不知道HopLength是啥的建议多看几个视频了解了解音频的基础知识，这一项在SoVits中必须填。（数值必须为你训练时的数值，可以在你训练模型时候的配置文件里看到）
//Cleaner：插件名，可以不填，填了就必须要在Cleaner文件夹防止相应的CleanerDll，如果Dll不存在或者是Dll内部有问题，则会在加载模型时报插件错误
//Hubert：Hubert模型名，必须填且必须将在前置模型中下载到的Hubert放置到Hubert文件夹
//Characters：如果是多角色模型必须填写为你的角色名称组成的列表，如果是单角色模型可以不填
//Diffusion：是否为DDSP仓库下的扩散模型
//ShallowDiffusion：SoVits浅扩散模型，须填写ShallowDiffusion模型配置文件名（不带后缀和完整路径），小显存或内存下速度巨慢，效果未知，请根据实际情况决定是否使用）
//CharaMix：是否使用角色混合轨道
//Volume：该模型是否有音量Emb
//HiddenSize：Vec模型的尺寸（768/256）
//Cluster：聚类类型，包括"KMeans"和"Index"，KMeans需要前往SoVits仓库将KMeans文件导出为可用格式，放置到模型文件夹；Index同理，需要前往SoVits仓库导出为可用格式（如果是RVC的单角色Index只需要改名为Index-0.index）然后放置到模型文件夹下（有几个角色就有几个Index文件）
//SoVits4.0V2: 是否为SoVits4.0V2模型
```
    
</details>
<details><summary>DiffSVC:</summary>
    
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
    
</details>
<details><summary>DiffSinger:</summary>
    
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

</details>
<details><summary>BertVits:</summary>
    
```jsonc
{
    "Folder": "HimenoSena",
    "Name": "HimenoSena",
    "Type": "BertVits",
    "Symbol": [
        "_",
        "AA",
        "E",
        "EE",
        "En",
        "N",
        "OO",
        "V",
        "a",
        "a:",
        "aa",
        "ae",
        "ah",
        "ai",
        "an",
        "ang",
        "ao",
        "aw",
        "ay",
        "b",
        "by",
        "c",
        "ch",
        "d",
        "dh",
        "dy",
        "e",
        "e:",
        "eh",
        "ei",
        "en",
        "eng",
        "er",
        "ey",
        "f",
        "g",
        "gy",
        "h",
        "hh",
        "hy",
        "i",
        "i0",
        "i:",
        "ia",
        "ian",
        "iang",
        "iao",
        "ie",
        "ih",
        "in",
        "ing",
        "iong",
        "ir",
        "iu",
        "iy",
        "j",
        "jh",
        "k",
        "ky",
        "l",
        "m",
        "my",
        "n",
        "ng",
        "ny",
        "o",
        "o:",
        "ong",
        "ou",
        "ow",
        "oy",
        "p",
        "py",
        "q",
        "r",
        "ry",
        "s",
        "sh",
        "t",
        "th",
        "ts",
        "ty",
        "u",
        "u:",
        "ua",
        "uai",
        "uan",
        "uang",
        "uh",
        "ui",
        "un",
        "uo",
        "uw",
        "v",
        "van",
        "ve",
        "vn",
        "w",
        "x",
        "y",
        "z",
        "zh",
        "zy",
        "!",
        "?",
        "\u2026",
        ",",
        ".",
        "'",
        "-",
        "SP",
        "UNK"
    ],
    "Cleaner": "",
    "Rate": 44100,
    "CharaMix": true,
    "Characters": [
        "\u56fd\u89c1\u83dc\u5b50",
        "\u59ec\u91ce\u661f\u594f",
        "\u65b0\u5802\u5f69\u97f3",
        "\u56db\u6761\u51db\u9999",
        "\u5c0f\u97a0\u7531\u4f9d"
    ],
    "LanguageMap": {
        "ZH": [
            0,
            0
        ],
        "JP": [
            1,
            6
        ],
        "EN": [
            2,
            8
        ]
    },
    "Dict": "BasicDict",
    "BertPath": [
        "chinese-roberta-wwm-ext-large",
        "deberta-v2-large-japanese",
        "bert-base-japanese-v3"
    ]
}
```
    
</details>

## 支持的项目
```cmake
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
## 其他设置
### Symbol

    例如：_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
    打开你训练模型的项目，打开text\symbol.py，如图按照划线的List顺序将上面的4个字符串连接即可
![image](https://user-images.githubusercontent.com/40709280/183290732-dcb93323-1061-431b-aafa-c285a3ec5e82.png)

### Cleaner
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

## 本地编译
```cxx
git clone https://github.com/NaruseMioShirakana/MoeVoiceStudio.git
//自行配置OnnxRuntime和FFMPEG的Dll
//使用VisualStudio构建
```

## 依赖列表
- [FFmpeg](https://ffmpeg.org/)
- [World](https://github.com/JeremyCCHsu/Python-Wrapper-for-World-Vocoder)
- [rapidJson](https://github.com/Tencent/rapidjson)
- [mecab](https://github.com/taku910/mecab)
- [yyjson](https://github.com/ibireme/yyjson)
- [onnxruntime](https://github.com/microsoft/onnxruntime)
- [faiss](https://github.com/facebookresearch/faiss)
- [中文词典](https://www.zdic.net/)

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

#

MoeSS使用的图像素材来源于：
- [SummerPockets](http://key.visualarts.gr.jp/summer/)

## 💪 感谢所有贡献者的努力
<a href="https://github.com/NaruseMioShirakana/MoeSS/graphs/contributors" target="_blank">
  <img src="https://contrib.rocks/image?repo=NaruseMioShirakana/MoeSS" />
</a>

