import os
import random
import time
from typing import List, Tuple, Dict, Any

import numpy as np
import onnxruntime as ort
import torch
import yaml
from tqdm import tqdm

from data.dataset import CMUDictDataset
from tools.config_utils import load_yaml


class LstmOnnx:
    def __init__(self, onnx_model_path: str, vocab_path: str, config_path: str):
        self.session = ort.InferenceSession(onnx_model_path)

        with open(vocab_path, 'r', encoding='utf-8') as f:
            vocab_data = yaml.safe_load(f)
        with open(config_path, 'r', encoding='utf-8') as f:
            self.config = yaml.safe_load(f)

        self.char_vocab = vocab_data['char_vocab']
        self.phoneme_vocab = vocab_data['phoneme_vocab']
        self.idx_to_phone = {v: k for k, v in self.phoneme_vocab.items()}

        self.UNK_IDX = self.config['unk_idx']
        self.PAD_IDX = self.config['pad_idx']
        self.BOS_IDX = self.config['bos_idx']
        self.EOS_IDX = self.config['eos_idx']
        self.max_len = 48

    def preprocess_word(self, word: str):
        word = word.lower().strip()
        word_indices = [self.char_vocab.get(c, self.UNK_IDX) for c in word]
        return [self.BOS_IDX] + word_indices + [self.EOS_IDX]

    def decode_phonemes(self, indices: List[int]):
        phonemes = []
        for idx in indices:
            if idx not in [self.BOS_IDX, self.EOS_IDX, self.PAD_IDX, self.UNK_IDX]:
                if idx in self.idx_to_phone:
                    phonemes.append(self.idx_to_phone[idx])
        return phonemes

    def predict(self, word: str):
        input_ids = self.preprocess_word(word)
        phoneme_ids = self.session.run(None, {"input_ids": np.array(input_ids, dtype=np.int64)})
        phonemes = self.decode_phonemes(phoneme_ids[0])
        return phonemes


class OpuOnnx:
    def __init__(self, onnx_model_path: str):
        self.session = ort.InferenceSession(onnx_model_path)

        self.graphemes = [
            "<unk>", "<pad>", "<bos>", "<eos>", "'", "-", "a", "b", "c", "d", "e",
            "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
            "t", "u", "v", "w", "x", "y", "z"
        ]

        self.phonemes = [
            "<unk>", "<pad>", "<bos>", "<eos>", "aa", "ae", "ah", "ao", "aw", "ay",
            "b", "ch", "d", "dh", "eh", "er", "ey", "f", "g", "hh", "ih", "iy",
            "jh", "k", "l", "m", "n", "ng", "ow", "oy", "p", "r", "s", "sh", "t",
            "th", "uh", "uw", "v", "w", "y", "z", "zh"
        ]

        self.grapheme_to_idx = {char: idx for idx, char in enumerate(self.graphemes)}
        self.UNK_IDX = 0
        self.PAD_IDX = 1
        self.BOS_IDX = 2
        self.EOS_IDX = 3
        self.max_len = 48

    @staticmethod
    def remove_tail_digits(s: str) -> str:
        while len(s) > 0 and s[-1].isdigit():
            s = s[:-1]
        return s

    def _preprocess_word(self, word: str) -> List[int]:
        word = self.remove_tail_digits(word.lower())
        encoded = []
        for char in word:
            encoded.append(self.grapheme_to_idx.get(char, self.UNK_IDX))
        return encoded

    def _decode_phonemes(self, indexes: List[int]) -> List[str]:
        phonemes = []
        for idx in indexes:
            if 4 <= idx < len(self.phonemes):
                phonemes.append(self.phonemes[idx])
        return phonemes

    def predict(self, word: str) -> List[str]:
        src = np.array([self._preprocess_word(word)], dtype=np.int32)
        if src.shape[1] == 0:
            return []

        tgt = np.array([[self.BOS_IDX]], dtype=np.int32)
        t = np.array([0], dtype=np.int32)
        src_length = src.shape[1]

        while t[0] < src_length and tgt.shape[1] < self.max_len:
            inputs = {'src': src, 'tgt': tgt, 't': t}
            outputs = self.session.run(['pred'], inputs)
            pred = outputs[0][0]

            if pred != self.BOS_IDX:
                new_tgt = np.zeros((1, tgt.shape[1] + 1), dtype=np.int32)
                new_tgt[0, :-1] = tgt[0]
                new_tgt[0, -1] = pred
                tgt = new_tgt
            else:
                t[0] += 1

        phonemes = self._decode_phonemes(tgt[0][1:])
        return phonemes


class Benchmark:
    def __init__(self, lstm_onnx_path, opu_onnx_path):
        model_dir = os.path.dirname(lstm_onnx_path)
        vocab_path = os.path.join(model_dir, "vocab.yaml")
        self.config_path = os.path.join(model_dir, "config.yaml")

        self.lstm_onnx = LstmOnnx(lstm_onnx_path, vocab_path, self.config_path)
        self.opu_onnx = OpuOnnx(opu_onnx_path)

    def load_benchmark_data(self, sample_size: int = 1000):
        dataset = CMUDictDataset(load_yaml(self.config_path))
        benchmark_data = []

        for batch in random.sample(dataset.processed_data, sample_size):
            word_tensor, phone_tensor = batch
            word = self._tensor_to_word(word_tensor, dataset.char_vocab)
            phones = self._tensor_to_phones(phone_tensor, dataset.phone_vocab)
            benchmark_data.append((word, phones))

        print(f"Created benchmark dataset: {len(benchmark_data)} samples")
        return benchmark_data

    @staticmethod
    def _tensor_to_word(tensor: torch.Tensor, char_vocab: Dict) -> str:
        idx_to_char = {v: k for k, v in char_vocab.items()}
        indices = tensor.tolist()
        chars = [idx_to_char[idx] for idx in indices if idx not in [0, 1, 2, 3] and idx in idx_to_char]
        return ''.join(chars)

    @staticmethod
    def _tensor_to_phones(tensor: torch.Tensor, phone_vocab: Dict) -> List[str]:
        idx_to_phone = {v: k for k, v in phone_vocab.items()}
        indices = tensor.tolist()
        phones = [idx_to_phone[idx] for idx in indices if idx not in [0, 1, 2, 3] and idx in idx_to_phone]
        return phones

    @staticmethod
    def calculate_metrics(predicted: List[str], target: List[str]) -> Tuple[float, float]:
        if not predicted or not target:
            return 0.0, 0.0

        min_len = min(len(predicted), len(target))
        if min_len == 0:
            return 0.0, 0.0

        correct_phones = sum(1 for i in range(min_len) if predicted[i] == target[i])
        phone_accuracy = correct_phones / len(target)

        seq_accuracy = 1.0 if (
                len(predicted) == len(target) and all(p == t for p, t in zip(predicted, target))) else 0.0

        return phone_accuracy, seq_accuracy

    def evaluate_model(self, model_type: str, benchmark_data: List[Tuple[str, List[str]]]):
        if model_type == "lstm" and not self.lstm_onnx:
            raise ValueError("Current ONNX model not loaded")
        if model_type == "opu" and not self.opu_onnx:
            raise ValueError("Legacy ONNX model not loaded")

        total_phone_acc = 0.0
        total_seq_acc = 0.0
        inference_times = []
        results = []

        for word, target_phones in tqdm(benchmark_data, desc=f"Evaluating {model_type} model"):
            start_time = time.time()

            if model_type == "lstm":
                predicted_phones = self.lstm_onnx.predict(word)
            else:
                predicted_phones = self.opu_onnx.predict(word)

            inference_time = time.time() - start_time
            inference_times.append(inference_time)

            phone_acc, seq_acc = self.calculate_metrics(predicted_phones, target_phones)
            total_phone_acc += phone_acc
            total_seq_acc += seq_acc

            results.append({
                'word': word,
                'target': target_phones,
                'predicted': predicted_phones,
                'phone_accuracy': phone_acc,
                'sequence_accuracy': seq_acc,
                'inference_time': inference_time
            })

        avg_phone_acc = total_phone_acc / len(results)
        avg_seq_acc = total_seq_acc / len(results)
        avg_inference_time = np.mean(inference_times)

        return {
            'model_type': model_type,
            'avg_phone_accuracy': avg_phone_acc,
            'avg_sequence_accuracy': avg_seq_acc,
            'avg_inference_time': avg_inference_time,
            'total_samples': len(results),
            'results': results
        }

    def run_benchmark(self, sample_size: int = 1000) -> Dict[str, Any]:
        print("Starting benchmark...")
        benchmark_data = self.load_benchmark_data(sample_size)
        results = {}

        if self.lstm_onnx:
            print("\nEvaluating LSTM ONNX model...")
            results['lstm'] = self.evaluate_model("lstm", benchmark_data)

        if self.opu_onnx:
            print("\nEvaluating opu ONNX model...")
            results['opu'] = self.evaluate_model("opu", benchmark_data)

        self._print_results(results)
        return results

    @staticmethod
    def _print_results(results: Dict[str, Any]):
        print("\n" + "=" * 70)
        print("Benchmark Results")
        print("=" * 70)

        for model_type, result in results.items():
            model_name = "Lstm onnx" if model_type == "lstm" else "Opu onnx"
            print(f"\n{model_name} Model:")
            print(f"  Samples: {result['total_samples']}")
            print(
                f"  Avg Phone Accuracy: {result['avg_phone_accuracy']:.4f} ({result['avg_phone_accuracy'] * 100:.2f}%)")
            print(
                f"  Avg Sequence Accuracy: {result['avg_sequence_accuracy']:.4f} ({result['avg_sequence_accuracy'] * 100:.2f}%)")
            print(f"  Avg Inference Time: {result['avg_inference_time'] * 1000:.2f} ms")

        print(f"\nModel Comparison:")
        lstm_acc = results['lstm']['avg_phone_accuracy']
        opu_acc = results['opu']['avg_phone_accuracy']
        lstm_time = results['lstm']['avg_inference_time']
        opu_time = results['opu']['avg_inference_time']

        acc_diff = lstm_acc - opu_acc
        time_ratio = lstm_time / opu_time if opu_time > 0 else float('inf')

        print(f"  Phone Accuracy Difference: {acc_diff:+.4f} ({acc_diff * 100:+.2f}%)")
        print(f"  Inference Time Ratio: {time_ratio:.2f}x")


if __name__ == "__main__":
    SAMPLE_SIZE = 1000

    benchmark = Benchmark(
        lstm_onnx_path="lstm_g2p_en/model.onnx",
        opu_onnx_path="g2p-arpabet/g2p.onnx"
    )

    benchmark.run_benchmark(SAMPLE_SIZE)
