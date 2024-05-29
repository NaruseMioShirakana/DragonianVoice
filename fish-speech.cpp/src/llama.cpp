#include "llama.h"

LibTTSBegin

RMSNorm::RMSNorm(Module* _Parent, const std::wstring& _Name, SizeType dim, float eps) :
	Module(_Parent, _Name),
	RegisterLayer(weight, { dim }),
	dim_(dim),
	eps_(eps)
{

}

ggml_tensor* RMSNorm::operator()(ggml_tensor* _Tensor, ggml_context* _Ctx, bool _Inplace)
{
	if(_Inplace)
		return ggml_mul_inplace(_Ctx, ggml_rms_norm_inplace(_Ctx, _Tensor, eps_), weight);
	return ggml_mul(_Ctx, ggml_rms_norm(_Ctx, _Tensor, eps_), weight);
}

void RMSNorm::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} RMSNorm(dim[{}], eps[{}])",
		RegName_,
		dim_,
		eps_
	);
}

void RMSNorm::ChangeParam(SizeType dim, float eps)
{
	weight.ChangeShape({ dim });
	eps_ = eps;
}

FeedForward::FeedForward(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config) :
	Module(_Parent, _Name),
	RegisterLayer(w1, { config.dim, config.intermediate_size, false }),
	RegisterLayer(w3, { config.dim, config.intermediate_size, false }),
	RegisterLayer(w2, { config.intermediate_size, config.dim, false }),
	args_(config)
{

}

ggml_tensor* FeedForward::operator()(ggml_tensor* x, ggml_context* _Ctx, bool _Inplace)
{
	return w2(
		ggml_mul(
			_Ctx,
			ggml_silu_inplace(_Ctx, w1(x, _Ctx, _Inplace)),
			w3(x, _Ctx, _Inplace)
		),
		_Ctx,
		_Inplace
	);
}

void FeedForward::ChangeParam(const BaseModelArgs& config)
{
	args_ = config;
	w1.ChangeParam({ config.dim, config.intermediate_size, false });
	w3.ChangeParam({ config.dim, config.intermediate_size, false });
	w2.ChangeParam({ config.intermediate_size, config.dim, false });
}

void FeedForward::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} FeedForward()",
		RegName_
	);
}

Attention::Attention(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config, bool use_sdpa) :
	Module(_Parent, _Name),
	total_head_dim((config.n_head + 2 * config.n_local_heads)* config.head_dim),
	RegisterLayer(wqkv, { config.dim, (config.n_head + 2 * config.n_local_heads) * config.head_dim, false }),
	RegisterLayer(wo, { config.dim, config.dim, false }),
	dropout(config.dropout),
	n_head(config.n_head),
	head_dim(config.head_dim),
	n_local_heads(config.n_local_heads),
	dim(config.dim),
	use_sdpa(use_sdpa),
	args_(config)
{
	
}

ggml_tensor* Attention::operator()(
	ggml_tensor* x,
	ggml_tensor* freqs_cis,
	ggml_tensor* mask,
	ggml_tensor* input_pos,
	ggml_context* _Ctx,
	bool _Inplace
	)
{
	int64_t bsz = x->ne[2], seqlen = x->ne[1];
	int64_t kv_size = (int64_t)n_local_heads * head_dim;

	auto QKV = wqkv(x, _Ctx);

	int64_t n_tokens = 1;
	for (size_t i = 1; i < 4; ++i)
		n_tokens *= QKV->ne[i];

	auto Q = ggml_cont(
		_Ctx,
		ggml_view_2d(_Ctx, QKV, dim, n_tokens, QKV->nb[1], 0 * sizeof(float) * (dim))
	);
	auto K = ggml_cont(
		_Ctx,
		ggml_view_2d(_Ctx, QKV, kv_size, n_tokens, QKV->nb[1], 1 * sizeof(float) * (dim))
	);
	auto V = ggml_cont(
		_Ctx,
		ggml_view_2d(_Ctx, QKV, kv_size, n_tokens, QKV->nb[1], 1 * sizeof(float) * (dim + kv_size))
	);

	Q = ggml_reshape_4d(_Ctx, Q, head_dim, n_head, seqlen, bsz);
	K = ggml_reshape_4d(_Ctx, K, head_dim, n_local_heads, seqlen, bsz);
	V = ggml_reshape_4d(_Ctx, V, head_dim, n_local_heads, seqlen, bsz);

	Q = apply_rotary_emb(Q, freqs_cis, _Ctx, _Inplace);
	K = apply_rotary_emb(K, freqs_cis, _Ctx, _Inplace);

	Q = ggml_cont(_Ctx, ggml_permute(_Ctx, Q, 0, 2, 1, 3));
	K = ggml_cont(_Ctx, ggml_permute(_Ctx, K, 0, 2, 1, 3));
	V = ggml_cont(_Ctx, ggml_permute(_Ctx, V, 0, 2, 1, 3));

	//TODO KVCACHE

	int Scale = n_head / n_local_heads;
	if(Scale > 1)
	{
		K = ggml_repeat(_Ctx, K, ggml_new_tensor_4d(_Ctx, K->type, K->ne[0], K->ne[1], K->ne[2] * Scale, K->ne[3]));
		V = ggml_repeat(_Ctx, V, ggml_new_tensor_4d(_Ctx, V->type, V->ne[0], V->ne[1], V->ne[2] * Scale, V->ne[3]));
	}

	//TODO

	return nullptr;
}

ggml_tensor* Attention::apply_rotary_emb(ggml_tensor* x, ggml_tensor* freqs_cis, ggml_context* _Ctx, bool _Inplace)
{
	//TODO
	return nullptr;
}

void Attention::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} AttentionLayer()",
		RegName_
	);
}

TransformerBlock::TransformerBlock(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config, bool use_sdpa):
	Module(_Parent, _Name),
	RegisterLayer(attention, config, use_sdpa),
	RegisterLayer(feed_forward, config),
	RegisterLayer(ffn_norm, config.dim, config.norm_eps),
	RegisterLayer(attention_norm, config.dim, config.norm_eps)
{
	
}

ggml_tensor* TransformerBlock::operator()(
	ggml_tensor* x,
	ggml_tensor* freqs_cis,
	ggml_tensor* mask,
	ggml_tensor* input_pos,
	ggml_context* _Ctx,
	bool _Inplace
	)
{
	UNUSED(_Inplace);
	auto h = attention(attention_norm(x, _Ctx, true), freqs_cis, mask, input_pos, _Ctx);
	return ggml_add_inplace(_Ctx, h, feed_forward(ffn_norm(h, _Ctx), _Ctx));
}

void TransformerBlock::DumpCurrentLayerInfo(std::wstring& _Tmp)
{
	_Tmp += std::format(
		L"{} TransformerBlock()",
		RegName_
	);
}

BaseTransformer::BaseTransformer(Module* _Parent, const std::wstring& _Name, const BaseModelArgs& config):
	Module(_Parent, _Name),
	config_(config),
	RegisterLayer(embeddings, {
		config.vocab_size + config.codebook_size * config.num_in_codebooks,
			config.dim
		}),
	RegisterLayer(layers),
	RegisterLayer(norm, config.dim, config.norm_eps),
	RegisterLayer(output, {
	config.dim,
			config.vocab_size,
			false
		}),
	RegisterLayer(freqs_cis, {
			config.max_seq_len,
			(config.dim / config.n_head) / 2
		}),
	RegisterLayer(causal_mask, {
		config.max_seq_len,
		config.max_seq_len
		})
{
	for (int i = 0; i < config.n_layer; ++i)
		layers.Append(LayerItem(TransformerBlock, config, true));
}

LibTTSEnd