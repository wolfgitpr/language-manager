from typing import Tuple

import torch
import torch.nn as nn


class Attention(nn.Module):
    def __init__(self, hidden_dim: int):
        super(Attention, self).__init__()
        self.attention = nn.Linear(hidden_dim * 2, hidden_dim)
        self.v = nn.Linear(hidden_dim, 1, bias=False)

    def forward(self, hidden: torch.Tensor, encoder_outputs: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor]:
        seq_len = encoder_outputs.shape[1]

        hidden = hidden.unsqueeze(1).repeat(1, seq_len, 1)

        energy = torch.tanh(self.attention(torch.cat((hidden, encoder_outputs), dim=2)))
        attention_weights = torch.softmax(self.v(energy).squeeze(2), dim=1)

        context_vector = torch.bmm(attention_weights.unsqueeze(1), encoder_outputs)

        return context_vector.squeeze(1), attention_weights
