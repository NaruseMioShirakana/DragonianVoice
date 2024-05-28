# GptSoVits主要分为三个部分

### VQ（内核为KMeans聚类）
- Train：对训练集音频的ssl[^1]进行KMeans聚类，获取到的聚类中心构造一个Embedding（CodeBook.embed）
- Infer：使用Indices获取聚类中心（CodeBook.embed）中的元素，构造一个ssl[^1]矩阵

与SoVits的KMeans/Index聚类类似，只不过SoVits的聚类在使用时是使用输入的HuBert在CodeBook中查找与其距离排名前K的点后加权平均，而GptSoVits则是使用一个AR循环预测所需的HuBert在CodeBook中的下标，之后使用该下标获取CodeBook中对应元素

---

### AR(GPT)
- Inputs:
	- text_seq：输入文本音素序列的数字ID（在Symbols数组中的下标）
	- text_bert：输入文本的Bert
	- ref_seq：参考文本音素序列的数字ID（在Symbols数组中的下标）
	- ref_bert：参考文本的Bert
	- ref_ssl：参考音频的ssl[^1]
- OutPuts:
	- codes：输入到VQ的Indices，用于获取ssl[^1]的聚类中心

与Gpt类似，使用一个AR循环，通过输入文本编码后的信息预测一个响应序列（序列终止为EOS），该响应序列为训练集音频聚类后的聚类中心在CodeBook中的下标，之后会从CodeBook中获取相应的元素，相当于SoVits中的Hubert。

---

### SoVits
- Inputs:
	- codes：输入到VQ的Indices，用于获取ssl[^1]的聚类中心
	- text_seq：输入文本音素序列的数字ID（在Symbols数组中的下标）
	- ref_audio：参考音频（训练集内音频）

与SoVits比较，其中的codes实际上相当于SoVits的Hubert，只不过这个Hubert是使用AR预测所得序列生成的。
GptSoVits使用输入音素的Embedding，AR预测所得的Hubert以及参考音频的Mel共通指导音频生成，可以有效的控制音频的语气，感情。
然而在一些时候，会出现漏字和错字的情况，可能和AR有较大的关系

---

### 实验方案
将GptSovits中的AR部分去除，将VQ的输入从Indices（code）替换为ssl（即使用最临近点搜索）。即可获得一个svc模型。

两个音频，一个训练集参考音频，一个输入音频。需完成以下步骤。

1、训练集参考音频直接编码为mel记作ref_audio。

2、输入音频经过一个asr处理为音素序列记作text_seq。

3、输入音频经过hubert后使用最临近点搜索，从vq的embedding中取元素，记作ssl。

4、将ssl，text_seq和ref_audio作为vits的输入进行推理。


---

[^1]: ssl其实就是音频的Hubert，与SoVits的Hubert一致
