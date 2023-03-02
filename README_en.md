觉得现在存档还是为时过早，至少等语音合成的几个项目停更再说（）

Discussions我开了，如果有什么问题可以在里面提，等其他大佬的回答就可以了，issue请发布确定为BUG的反馈或对于期望加入的功能的反馈。

[简体中文](README.md)    [English](README_en.md)

---

# Predecessor models：
Github - Vocoder & HiddenUnitBert: [Vocoder & HiddenUnitBert](https://github.com/NaruseMioShirakana/RequireMent-Model-For-MoeSS) 

HuggingFace : [HuggingFace](https://huggingface.co/NaruseMioShirakana/MoeSS-SUBModel) 

Export predecessor models by yourself：
- HuBert：input_names must be ["source"]，output_names must be ["embed"]，dynamic_axes must be {"source":[0,2],}
- Diffusion - hifigan：input_names must be ["c","f0"]，output_names must be ["audio"]，dynamic_axes must be {"c":[0,1],"f0":[0,1],}
- Tacotron2 - hifigan：input_names must be ["x"]，output_names must be ["audio"]，dynamic_axes must be {"x":[0,1],}

---
# User Agreement：
On the basis of the open source agreement please also comply with the following rules.

- Please indicate the project repository when citing this project. The project cannot be compiled at the moment (due to the use of UI libraries that are not open source)

- You can also indicate author's bilibili space address：https://space.bilibili.com/108592413

## By using this program you are agreeing to the following：
- 1. You are willing to bear all the consequences caused by the use of the program.
- 2、You promise that you will not sell the program and its affiliated models, and that you will bear all the consequences caused by the sale.
- 3、You will not use it to engage in illegal activities, and if you engage in illegal activities, you will be responsible for all the consequences.
- 4、Prohibited to use it for any commercial game, low creation game and Galgame production, not against gratuitous fine game production and Mod production.
- 5、Prohibit the use of the project and the project derivatives and the release of models and other production of various electronic junk (such as AIGalgame, AI game production, etc.)
---

## Q&A：
### Q：Will the program be charged in the future?？
    A：No, and this project is permanently open source and free, if there is a paid version of this software elsewhere, please report it immediately and do not buy it, this software is permanently free. If you want to support the project，see：https://afdian.net/a/NaruseMioShirakana 
### Q：Paid model training services？
    A：In principle, it does not provide, training TTS model is relatively simple, there is no need to spend money, follow the online tutorial step by step on it. Free Onnx output is provided。
### Q：Technical Support？
    A：If you can be sure that what you produce is not e-waste, I will provide some technical support in my ability. 
---

# Moe Speech Synthesis
A full C++ Speech Synthesis UI software based on various open source TTS, VC and SVS projects

Repository of supported projects：
- [DeepLearningExamples](https://github.com/NVIDIA/DeepLearningExamples)
- [VITS](https://github.com/jaywalnut310/vits)
- [SoVits](https://github.com/innnky/so-vits-svc/tree/32k)
- [DiffSvc](https://github.com/prophesier/diff-SVC)
- [DiffSinger](https://github.com/openvpi/DiffSinger)

The image material used is derived from：
- [SummerPockets](http://key.visualarts.gr.jp/summer/)

Windows Only

---
## Usage：
    1、Download the software package in the release and unzip it.

    2、Download the corresponding pre-models or additional modules in the [Vocoder & HiddenUnitBert] repository above and place them in the corresponding folders, the correspondence between pre-models and projects will be mentioned below.

    3. Place the model in the Mods folder, and select the model from the "模型选择" Module at the top left, for the standard model structure, please refer to "Supported Projects" below.

    4, enter the text to be converted in the input box below, click "启用插件" to execute the text Cleaner, and change the behavior of the batch conversion of clause symbols (SoVits/DiffSvc need to enter the audio path, DiffSinger need to enter the path of the ds or json project file)

    5, click "开始合成", you can start synthesizing voice, wait for the progress to complete, you can preview in the top right player, you can also save directly in the top right
---
## Making models：
- The software standardizes the model reading module, the model is placed in a subfolder under the Mods folder. ********.json file is used to declare the model path and its display name, etc. The model needs to be converted to Onnx, the repository for exporting onnx will be mentioned below

### General parameters(All types of models must be filled in, if you do not fill in the model will not be recognized)：
- Folder: the name of the folder where the model is saved
- Name: the name of the model displayed in the UI
- Type: model category
- Rate: sampling rate
### Tacotron2：
```jsonc
{
    "Folder" : "Atri",
    "Name" : "亚托莉-Tacotron2",
    "Type" : "Tacotron2",
    "Rate" : 22050,
    "Symbol" : "_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
    "Cleaner" : "JapaneseCleaner",
    "Hifigan": "hifigan"
}
//Symbol: Symbol of the model
//Cleaner: the name of the plugin, optional, if this is not empty, you must place the corresponding CleanerDll in the Cleaner folder, if the Dll does not exist or there is a problem inside the Dll, it will report a plugin error when loading the model
//Hifigan: Hifigan model name, must be filled in and the hifigan model downloaded from the pre-model or exported by yourself must be placed in the hifigan folder.
```
### Vits：
```jsonc
{
    "Folder" : "SummerPockets",
    "Name" : "SummerPocketsReflectionBlue",
    "Type" : "Vits",
    "Rate" : 22050,
    "Symbol" : "_,.!?-~…AEINOQUabdefghijkmnoprstuvwyzʃʧʦ↓↑ ",
    "Cleaner" : "JapaneseCleaner",
    "Characters" : ["鳴瀬しろは","空門蒼","鷹原うみ","紬ヴェンダース","神山識","水織静久","野村美希","久島鴎","岬鏡子"]
}
//Symbol: Symbol of the model
//Cleaner: the name of the plugin, optional, if this is not empty, you must place the corresponding CleanerDll in the Cleaner folder, if the Dll does not exist or there is a problem inside the Dll, it will report a plugin error when loading the model
//Characters: If it is a multi-role model, you must fill in the list of your role names, if it is a single-role model, you can leave it out
```
### SoVits：
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
    "SoVits4": false,
    "Characters" : ["Taffy","Nyaru"]
}
//Hop：HopLength
//Cleaner: the name of the plugin, optional, if this is not empty, you must place the corresponding CleanerDll in the Cleaner folder, if the Dll does not exist or there is a problem inside the Dll, it will report a plugin error when loading the model
//Hubert: The name of the Hubert model, must be filled in and the Hubert model downloaded from the pre-model or exported by yourself must be placed in the Hubert folder.
//SoVits3: whether it is SoVits3.0, if not SoVits3.0 fill in False
//SoVits4: whether it is SoVits4.0, if it is not SoVits4.0, fill in False
//Characters: If it is a multi-role model, it must be filled in as a list of your role names, if it is a single-role model, you can leave it out.
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
    "V2" : true
}
//Hop：HopLength
//MelBins：MelBins
//Cleaner: the name of the plugin, optional, if this is not empty, you must place the corresponding CleanerDll in the Cleaner folder, if the Dll does not exist or there is a problem inside the Dll, it will report a plugin error when loading the model
//Hubert: The name of the Hubert model, must be filled in and the Hubert model downloaded from the pre-model or exported by yourself must be placed in the Hubert folder.
//Hifigan: Hifigan model name, must be filled in and the nsf_hifigan model downloaded from the pre-model or exported by yourself must be placed in the hifigan folder.
//Characters: If it is a multi-role model, it must be filled in as a list of your role names, if it is a single-role model, you can leave it out.
//Pndm: Acceleration multiplier, required for V1 models and must be the acceleration multiplier during export
//V2: whether it is V2 model, V2 model is divided into 4 modules
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
//Hop：HopLength
//Cleaner: the name of the plugin, optional, if this is not empty, you must place the corresponding CleanerDll in the Cleaner folder, if the Dll does not exist or there is a problem inside the Dll, it will report a plugin error when loading the model
//Hifigan: Hifigan model name, must be filled in and the nsf_hifigan model downloaded from the pre-model or exported by yourself must be placed in the hifigan folder.
//Characters: If it is a multi-role model, it must be filled in as a list of your role names, if it is a single-role model, you can leave it out.
//MelBins：MelBins
```

---
## Supported Projects
```cxx 
// Tacotron2：
    ${Folder}_decoder_iter.onnx
    ${Folder}_encoder.onnx
    ${Folder}_postnet.onnx
// Vits:   single-spk 单角色VITS
    ${Folder}_dec.onnx
    ${Folder}_flow.onnx
    ${Folder}_enc_p.onnx
    ${Folder}_dp.onnx 
// Vits:   mult-spk VITS
    ${Folder}_dec.onnx
    ${Folder}_emb.onnx
    ${Folder}_flow.onnx
    ${Folder}_enc_p.onnx
    ${Folder}_dp.onnx
// SoVits:
    ${Folder}_SoVits.onnx
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
## Symbols
    For example：_-!'(),.:;? ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
    in training project, text\symbol.py，Just connect the above strings in List order as shown
![image](https://user-images.githubusercontent.com/40709280/183290732-dcb93323-1061-431b-aafa-c285a3ec5e82.png)

---
## Cleaners
```cxx
/*
Plugins should be placed in the Plugins folder and should be a dynamic library exported as required, the dll should be named Plugin Name, the Plugin Name is the content filled in the Cleaner column in the Model Definition Json file.
All Plugins dlls need to define the following functions, the function name must be PluginMain and the Dll name must be the Plugins name (or Cleaner name).
*/
const wchar_t* PluginMain(const wchar_t*);
// The interface only requires consistent input and output, not consistent function, that is, you can implement any function you want in this Dll, such as ChatGpt, machine translation, etc.
// For example, the PluginMain function passes in a string "input", passes "input" into ChatGpt, then passes the output of ChatGpt into Clean, and finally returns the output.
wchar_t* PluginMain(wchar_t* input){
    wchar_t* tmpOutput = ChatGpt(input);
    return Clean(tmpOutput);
}
// Note: Please use extern "C" keyword when exporting the dll.
```

## Requirements
- [FFmpeg](https://ffmpeg.org/)
- [World](https://github.com/JeremyCCHsu/Python-Wrapper-for-World-Vocoder)
- [rapidJson](https://github.com/Tencent/rapidjson)
