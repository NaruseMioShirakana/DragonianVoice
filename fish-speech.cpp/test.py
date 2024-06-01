import torch
import time
for i in range(20):
    a = torch.ones(size=(1, 768, 100000))
    beg = time.time()
    a.fill_(i)
    print(time.time() - beg)