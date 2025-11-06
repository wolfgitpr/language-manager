import json
import os
import random
import time
from typing import List, Tuple, Dict, Any

import numpy as np
import onnxruntime as ort
import torch
import yaml
from tqdm import tqdm

from models.lstm_g2p import LstmG2p
from tools.config_utils import load_yaml
from tools.dataset import CMUDictDataset


class LstmOnnx:
    def __init__(self, onnx_model_dir: str):
        encoder_path = os.path.join(onnx_model_dir, "encoder.onnx")
        decoder_path = os.path.join(onnx_model_dir, "decoder.onnx")

        self.encoder_session = ort.InferenceSession(encoder_path)
        self.decoder_session = ort.InferenceSession(decoder_path)

        char_path = os.path.join(onnx_model_dir, "char.json")
        phonemes_path = os.path.join(onnx_model_dir, "phonemes.json")
        config_path = os.path.join(onnx_model_dir, "config.json")

        with open(config_path, 'r', encoding='utf-8') as f:
            self.config = json.load(f)
        with open(char_path, 'r', encoding='utf-8') as f:
            self.char_vocab = json.load(f)
        with open(phonemes_path, 'r', encoding='utf-8') as f:
            self.phoneme_vocab = json.load(f)
        self.idx_to_phone = {v: k for k, v in self.phoneme_vocab.items()}

        self.UNK_IDX = self.phoneme_vocab['<unk>']
        self.PAD_IDX = self.phoneme_vocab['<pad>']
        self.BOS_IDX = self.phoneme_vocab['<bos>']
        self.EOS_IDX = self.phoneme_vocab['<eos>']
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
        input_tensor = np.array(input_ids, dtype=np.int64)

        encoder_outputs, hidden, cell = self.encoder_session.run(
            None, {"input_ids": input_tensor}
        )

        decoder_input = np.array([self.BOS_IDX], dtype=np.int64)
        phoneme_ids = []

        for _ in range(self.max_len):
            outputs = self.decoder_session.run(
                None,
                {
                    "decoder_input": decoder_input,
                    "hidden": hidden,
                    "cell": cell,
                    "encoder_outputs": encoder_outputs
                }
            )

            output, hidden, cell, attention_weights = outputs

            predicted_id = np.argmax(output, axis=-1)

            if predicted_id == self.EOS_IDX:
                break

            phoneme_ids.append(int(predicted_id))

            decoder_input = np.array([predicted_id], dtype=np.int64)

        phonemes = self.decode_phonemes(phoneme_ids)
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


class PyTorchModel:
    def __init__(self, ckpt_path: str, config_path: str, beam_size: int = 5):
        with open(config_path, 'r', encoding='utf-8') as f:
            self.config = yaml.safe_load(f)

        self.model = LstmG2p.load_from_checkpoint(ckpt_path, config=self.config)
        self.model.to('cuda' if torch.cuda.is_available() else 'cpu')
        self.model.eval()

        self.beam_size = beam_size
        self.max_len = 48

    def predict(self, word: str) -> List[str]:
        with torch.no_grad():
            return self.model.predict(word, self.max_len, self.beam_size)


class Benchmark:
    def __init__(self, lstm_onnx_path=None, opu_onnx_path=None, ckpt_path=None, beam_size=5):
        model_dir = None

        self.lstm_onnx = LstmOnnx(lstm_onnx_path)
        self.opu_onnx = OpuOnnx(opu_onnx_path)

        if not model_dir:
            model_dir = os.path.dirname(ckpt_path)
            self.config_path = os.path.join(model_dir, "config.yaml")
        self.pytorch_model = PyTorchModel(ckpt_path, self.config_path, beam_size)

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
            raise ValueError("LSTM ONNX model not loaded")
        if model_type == "opu" and not self.opu_onnx:
            raise ValueError("OPU ONNX model not loaded")
        if model_type == "pytorch" and not self.pytorch_model:
            raise ValueError("PyTorch model not loaded")

        total_phone_acc = 0.0
        total_seq_acc = 0.0
        inference_times = []
        results = []

        for word, target_phones in tqdm(benchmark_data, desc=f"Evaluating {model_type} model"):
            start_time = time.time()

            if model_type == "lstm":
                predicted_phones = self.lstm_onnx.predict(word)
            elif model_type == "opu":
                predicted_phones = self.opu_onnx.predict(word)
            else:
                predicted_phones = self.pytorch_model.predict(word)

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
            print("\nEvaluating OPU ONNX model...")
            results['opu'] = self.evaluate_model("opu", benchmark_data)

        if self.pytorch_model:
            print("\nEvaluating PyTorch model (with beam search)...")
            results['pytorch'] = self.evaluate_model("pytorch", benchmark_data)

        self._print_results(results)
        return results

    @staticmethod
    def _print_results(results: Dict[str, Any]):
        print("\n" + "=" * 70)
        print("Benchmark Results")
        print("=" * 70)

        for model_type, result in results.items():
            if model_type == "lstm":
                model_name = "LSTM ONNX"
            elif model_type == "opu":
                model_name = "OPU ONNX"
            else:
                model_name = "PyTorch (beam search)"

            print(f"\n{model_name} Model:")
            print(f"  Samples: {result['total_samples']}")
            print(
                f"  Avg Phone Accuracy: {result['avg_phone_accuracy']:.4f} ({result['avg_phone_accuracy'] * 100:.2f}%)")
            print(
                f"  Avg Sequence Accuracy: {result['avg_sequence_accuracy']:.4f} ({result['avg_sequence_accuracy'] * 100:.2f}%)")
            print(f"  Avg Inference Time: {result['avg_inference_time'] * 1000:.2f} ms")

        if len(results) > 1:
            print(f"\nModel Comparison:")
            model_types = list(results.keys())

            for i in range(len(model_types)):
                for j in range(i + 1, len(model_types)):
                    model1 = model_types[i]
                    model2 = model_types[j]
                    acc1 = results[model1]['avg_phone_accuracy']
                    acc2 = results[model2]['avg_phone_accuracy']
                    time1 = results[model1]['avg_inference_time']
                    time2 = results[model2]['avg_inference_time']

                    acc_diff = acc1 - acc2
                    time_ratio = time1 / time2 if time2 > 0 else float('inf')

                    print(f"  {model1.upper()} vs {model2.upper()}:")
                    print(f"    Phone Accuracy Difference: {acc_diff:+.4f} ({acc_diff * 100:+.2f}%)")
                    print(f"    Inference Time Ratio: {time_ratio:.2f}x")


if __name__ == "__main__":
    SAMPLE_SIZE = 1000

    benchmark = Benchmark(
        lstm_onnx_path="lstm_g2p_en/",
        opu_onnx_path="g2p-arpabet/g2p.onnx",
        ckpt_path="ckpt/LSTM_G2P/best-step=15120-val_seq_acc=0.69333.ckpt",
        beam_size=1
    )

    benchmark.run_benchmark(SAMPLE_SIZE)
