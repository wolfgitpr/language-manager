from typing import Tuple

import torch
import torch.nn as nn

from models.attention import Attention


class LstmDecoder(nn.Module):
    def __init__(self, vocab_size: int, embedding_dim: int, hidden_dim: int, num_layers: int, dropout: float,
                 padding_idx: int):
        super(LstmDecoder, self).__init__()

        self.num_layers = num_layers  # 保存层数

        self.embedding = nn.Embedding(
            vocab_size,
            embedding_dim,
            padding_idx=padding_idx
        )

        self.attention = Attention(hidden_dim)

        self.lstm = nn.LSTM(
            embedding_dim + hidden_dim,
            hidden_dim,
            dropout=dropout if num_layers > 1 else 0,
            num_layers=num_layers,
            batch_first=True,
        )

        self.fc = nn.Sequential(
            nn.Linear(hidden_dim * 2, hidden_dim),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(hidden_dim, vocab_size)
        )

    def forward(self, x: torch.Tensor, hidden: torch.Tensor, cell: torch.Tensor,
                encoder_outputs: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
        embedded = self.embedding(x)

        last_hidden = hidden[-1].unsqueeze(0)  # [1, batch_size, hidden_dim]

        context, attention_weights = self.attention(last_hidden.squeeze(0), encoder_outputs)

        lstm_input = torch.cat((embedded, context.unsqueeze(1)), dim=2)
        output, (hidden, cell) = self.lstm(lstm_input, (hidden, cell))

        output_with_context = torch.cat((output, context.unsqueeze(1)), dim=2)
        prediction = self.fc(output_with_context)
        return prediction, hidden, cell, attention_weights
