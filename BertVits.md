# BertVits及Vits使用指南
- 1、按照要求安装模型
- 2、安装Cleaner（[下载地址](https://github.com/NaruseMioShirakana/TextCleaner/releases)，将文件夹“G2P”解压到Exe路径）
- 3、将Bert文件夹复制到Exe路径，其中的子文件夹可以放置我发布的Bert模型，也可以啥都不放（如果不放模型就不能用Bert模型，但是不影响正常推理，就是效果可能会大打折扣）
- 4、按照自己的需要配置字典（Dict）文件
- 5、编写输入，载入程序推理

## BasicDict.json
    字典的作用就是将软件自动处理出来的文本替换为你使用的模型的Symbol，而字典文件的作用就是规定这个替换规则，字典文件是如同BasicDict.json的文件，其中由非常多的键值对组成，其中的Key就是待替换文本，而Value就是替换后的文本。

## VitsInputTemplate.json
```jsonc
//Json需要是数组类型
[
    {
            "Tokens": "私は誰？",//必填，进入Bert的文本
            "Seq": ["w","a","t","a","s","h","i","w","a","d","a","r","e","?"],//选填，音素组成的序列，如果不填会根据Tokens自动生成
            "Tones": [0,0,0,0,0,0,0,0,0,0,0,0,0],//选填，音调序列，必须与音素序列等长
            "Durations": [2,5,2,5,2,2,5,2,5,2,5,2,5],//选填，音素时长序列，必须与音素序列等长
            "Language": [0,0,0,0,0,0,0,0,0,0,0,0,0],//选填，语言序列，必须与音素序列等长
            "SpeakerMix": [1,0,0],//选填，角色混合比例，决定对应下标角色音色的混合比例
            "EmotionPrompt": ["sad", "happy"],//选填，情感参数，有情感模型的情况下可用
            "NoiseScale": 0.666,//选填，噪声修正因子
            "LengthScale": 1.1,//选填，时长修正因子
            "DurationPredictorNoiseScale": 0.333,//选填，随机时长预测器噪声修正因子
            "FactorDpSdp": 0.6,//选填，时长预测器和随机时长预测器的混合比例
            "GateThreshold": 0.777,//选填，Tacotron2 EOS阈值
            "MaxDecodeStep": 114514,//选填，Tacotron2 最大解码步数
            "Seed": 1919810,//选填，种子
            "SpeakerId": 2,//选填，角色ID（若SpeakerMix为空则使用）
            "RestTime": 1.0,//选填，决定与上一个片段的时间间隔（单位为秒），若为负数则表示切断音频并输出一个新的
            "PlaceHolderSymbol": "|",//选填，当Seq为String时，隔开两个音素的记号
            "LanguageID": "JP",//选填，语言（ZH，JP或EN）
            "G2PAdditionalInfo": "/[Japanese2]"//选填，Cleaner额外参数
        },
]
```