#pragma once

struct BaseModelArgs {
	int vocab_size = 32000;
	int n_layer = 32;
	int n_head = 32;
	int dim = 4096;
	int intermediate_size = 0;
	int n_local_heads = -1;
	int head_dim = 64;
	float rope_base = 10000;
	float norm_eps = 1e-5;
	int max_seq_len = 2048;
	float dropout = 0.0;

	// Codebook configs
	int codebook_size = 160;
	int num_codebooks = 4;
	int num_in_codebooks = 0;
	int codebook_padding_idx = 0;

	// Gradient checkpointing
	bool use_gradient_checkpointing = true;
};