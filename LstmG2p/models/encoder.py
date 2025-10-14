from typing import Tuple

import torch
import torch.nn as nn


class LstmEncoder(nn.Module):
    def __init__(self, vocab_size: int, embedding_dim: int, hidden_dim: int, num_layers: int, dropout: float,
                 padding_idx: int):
        super(LstmEncoder, self).__init__()

        self.embedding = nn.Embedding(
            vocab_size,
            embedding_dim,
            padding_idx=padding_idx
        )

        self.lstm = nn.LSTM(
            embedding_dim,
            hidden_dim,
            num_layers=num_layers,
            dropout=dropout if num_layers > 1 else 0,
            batch_first=True,
            bidirectional=True
        )

        self.projection = nn.Linear(hidden_dim * 2, hidden_dim)

    def forward(self, x: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
        embedded = self.embedding(x)
        outputs, (hidden, cell) = self.lstm(embedded)

        batch_size = hidden.shape[1]
        hidden_dim = hidden.shape[2]
        num_layers = hidden.shape[0] // 2

        hidden = hidden.view(num_layers, 2, batch_size, hidden_dim)
        hidden = hidden.mean(dim=1)

        cell = cell.view(num_layers, 2, batch_size, hidden_dim)
        cell = cell.mean(dim=1)

        outputs = self.projection(outputs)
        return outputs, hidden, cell
