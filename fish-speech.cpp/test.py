import torch

a = torch.nn.ConvTranspose2d(114, 514, 3)
def precompute_freqs_cis(seq_len: int, n_elem: int, base: int = 20000):
    freqs = 1.0 / (
        base ** (torch.arange(0, n_elem, 2)[: (n_elem // 2)].float() / n_elem)
    )
    t = torch.arange(seq_len, device=freqs.device)
    freqs = torch.outer(t, freqs)
    freqs_cis = torch.polar(torch.ones_like(freqs), freqs)
    cache = torch.stack([freqs_cis.real, freqs_cis.imag], dim=-1)
    return cache.to(dtype=torch.bfloat16)

b = precompute_freqs_cis(2000, 4999)
print(b.size())