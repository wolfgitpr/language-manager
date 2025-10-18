import re
from typing import Tuple, Dict, List

import torch
from torch.utils.data import Dataset


class CMUDictDataset(Dataset):
    def __init__(self, config: dict):
        self.config = config
        self.data_config = config['data']
        self.model_config = config['model']

        self.vocab_config = self.config['vocab']
        self.graphemes = self.vocab_config['graphemes']
        self.phonemes = self.vocab_config['phonemes']

        self.special_symbols = {k: v for k, v in enumerate(config['special_symbols'])}

        self.word_digit_pattern = re.compile(r'\(\d+\)')
        self.phone_digit_pattern = re.compile(r'\d+')

        self.char_vocab = {**self.special_symbols,
                           **{v: k + len(self.special_symbols) for k, v in enumerate(self.graphemes)}}
        self.phone_vocab = {**self.special_symbols,
                            **{v: k + len(self.special_symbols) for k, v in enumerate(self.phonemes)}}

        self.processed_data = self._preprocess_data(config['data']['dict_path'], config['data']['dict_type'])

    def _preprocess_data(self, file_path: str, dict_type: str):
        processed_data = []

        if dict_type == 'cmudict':
            with open(file_path, 'r', encoding='latin-1') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith(';;;'):
                        parts = line.lower().split('  ')
                        if len(parts) == 2:
                            word = parts[0]
                            phones = parts[1].split()

                            word = self._clean_word(word)
                            phones = self._clean_phones(phones)

                            word_indices = self._word_to_indices(word)
                            phone_indices = self._phones_to_indices(phones)

                            processed_data.append((
                                torch.tensor(word_indices, dtype=torch.long),
                                torch.tensor(phone_indices, dtype=torch.long)
                            ))
        else:
            raise "data/dataset _preprocess_data: unknown dict_type"

        print(f"preprocessed {len(processed_data)} examples")
        return processed_data

    def _clean_word(self, word: str) -> str:
        if self.data_config['remove_word_digits']:
            return self.word_digit_pattern.sub('', word)
        return word

    def _clean_phones(self, phones: List[str]) -> List[str]:
        if self.data_config['remove_phoneme_digits']:
            return [self.phone_digit_pattern.sub('', p) for p in phones]
        return phones

    def _word_to_indices(self, word: str) -> List[int]:
        word_indices = [self.char_vocab.get(c, self.config['unk_idx']) for c in word]
        return [self.config['bos_idx']] + word_indices + [self.config['eos_idx']]

    def _phones_to_indices(self, phones: List[str]) -> List[int]:
        phone_indices = [self.phone_vocab.get(p, self.config['unk_idx']) for p in phones]
        return [self.config['bos_idx']] + phone_indices + [self.config['eos_idx']]

    def __len__(self) -> int:
        return len(self.processed_data)

    def __getitem__(self, idx: int) -> Tuple[torch.Tensor, torch.Tensor]:
        return self.processed_data[idx]

    def get_vocab_sizes(self) -> Tuple[int, int]:
        return len(self.char_vocab), len(self.phone_vocab)

    def get_vocabs(self) -> Tuple[Dict[str, int], Dict[str, int]]:
        return self.char_vocab, self.phone_vocab


def collate_fn(batch: List[Tuple[torch.Tensor, torch.Tensor]]) -> Tuple[torch.Tensor, torch.Tensor]:
    src_batch, tgt_batch = zip(*batch)
    src_batch = torch.nn.utils.rnn.pad_sequence(src_batch, batch_first=True, padding_value=1)
    tgt_batch = torch.nn.utils.rnn.pad_sequence(tgt_batch, batch_first=True, padding_value=1)
    return src_batch, tgt_batch
