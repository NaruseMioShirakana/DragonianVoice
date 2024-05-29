#pragma once
#include "Module.h"

LibTTSBegin

struct BaseModelArgs {
	int vocab_size = 32000;
	int n_layer = 32;
	int n_head = 32;
	int dim = 4096;
	int intermediate_size = 1;
	int n_local_heads = -1;
	int head_dim = 64;
	float rope_base = 10000.f;
	float norm_eps = 1e-5f;
	int max_seq_len = 2048;
	float dropout = 0.f;

	// Codebook configs
	int codebook_size = 160;
	int num_codebooks = 4;
	int num_in_codebooks = 0;
	int codebook_padding_idx = 0;

	// Gradient checkpointing
	bool use_gradient_checkpointing = true;
};

class RMSNorm : public Module
{
public:
	RMSNorm(Module* _Parent, const std::wstring& _Name, SizeType dim, float eps = 1e-5f);
	ggml_tensor* operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(SizeType dim, float eps = 1e-5f);
	
private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Parameter weight;
	SizeType dim_;
	float eps_;
};

class FeedForward : public Module
{
public:
	FeedForward(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config);
	ggml_tensor* operator()(ggml_tensor* x, ggml_context* _Ctx, bool _Inplace = false) override;
	void ChangeParam(const BaseModelArgs& config);

private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Linear w1, w3, w2;
	BaseModelArgs args_;
};

class Attention : public Module
{
public:
	Attention(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config, bool use_sdpa = true);

	ggml_tensor* operator()(
		ggml_tensor* x,
		ggml_tensor* freqs_cis,
		ggml_tensor* mask,
		ggml_tensor* input_pos,
		ggml_context* _Ctx,
		bool _Inplace = false
		);

	ggml_tensor* apply_rotary_emb(
		ggml_tensor* x,
		ggml_tensor* freqs_cis,
		ggml_context* _Ctx,
		bool _Inplace = false
	);

private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	int total_head_dim;
	Linear wqkv, wo;
	float dropout;
	int n_head;
	int head_dim;
	int n_local_heads;
	int dim;
	bool use_sdpa;
	BaseModelArgs args_;
};

class TransformerBlock : public Module
{
public:
	TransformerBlock(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config, bool use_sdpa = true);
	ggml_tensor* operator()(
		ggml_tensor* x,
		ggml_tensor* freqs_cis,
		ggml_tensor* mask,
		ggml_tensor* input_pos,
		ggml_context* _Ctx,
		bool _Inplace = false
		);

private:
	void DumpCurrentLayerInfo(std::wstring& _Tmp) override;
	Attention attention;
	FeedForward feed_forward;
	RMSNorm ffn_norm;
	RMSNorm attention_norm;
};

class BaseTransformer : public Module
{
public:
	BaseTransformer(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config);

private:
	BaseModelArgs config_;
	Embedding embeddings;
	ModuleList layers;
	RMSNorm norm;
	Linear output;
	Parameter freqs_cis;
	Parameter causal_mask;

	int max_batch_size = -1;
	int max_seq_len = -1;
};

LibTTSEnd