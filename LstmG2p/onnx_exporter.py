import argparse
import json
import pathlib

import onnx
import onnxsim
import torch
import yaml

from models.lstm_g2p import LstmG2p


class EncoderOnnx(torch.nn.Module):
    def __init__(self, encoder):
        super().__init__()
        self.encoder = encoder

    def forward(self, src: torch.Tensor):
        # src: [seq_len]
        src_batch = src.unsqueeze(0)  # [1, seq_len]
        encoder_outputs, hidden, cell = self.encoder(src_batch)
        return encoder_outputs, hidden, cell


class DecoderStepOnnx(torch.nn.Module):
    def __init__(self, decoder):
        super().__init__()
        self.decoder = decoder

    def forward(self, decoder_input: torch.Tensor, hidden: torch.Tensor,
                cell: torch.Tensor, encoder_outputs: torch.Tensor):
        decoder_input_batch = decoder_input.unsqueeze(0)  # [1, 1]

        output, hidden, cell, attention_weights = self.decoder(
            decoder_input_batch, hidden, cell, encoder_outputs
        )

        return output.squeeze(0).squeeze(0), hidden, cell, attention_weights.squeeze(0)


class LstmG2pOnnxExporter:
    def __init__(self, model):
        self.model = model
        self.config = model.config
        self.device = next(model.parameters()).device

    def export_components(self, onnx_dir: str):
        onnx_dir = pathlib.Path(onnx_dir)
        onnx_dir.mkdir(parents=True, exist_ok=True)

        encoder_onnx = EncoderOnnx(self.model.encoder)
        encoder_onnx.eval()

        example_word = "hello"
        example_indices = [self.model.char_vocab.get(c, self.config['unk_idx']) for c in example_word]
        example_input = torch.tensor(
            [self.config['bos_idx']] + example_indices + [self.config['eos_idx']],
            dtype=torch.long,
            device=self.device
        )

        torch.onnx.export(
            encoder_onnx,
            (example_input,),
            str(onnx_dir / "encoder.onnx"),
            input_names=["input_ids"],
            output_names=["encoder_outputs", "hidden", "cell"],
            dynamic_axes={
                "input_ids": {0: "src_seq_len"},
                "encoder_outputs": {0: "batch_size", 1: "src_seq_len"},
                "hidden": {1: "batch_size"},
                "cell": {1: "batch_size"}
            },
            opset_version=14,
            do_constant_folding=True,
            export_params=True,
            verbose=False
        )

        decoder_onnx = DecoderStepOnnx(self.model.decoder)
        decoder_onnx.eval()

        seq_len = example_input.size(0)
        hidden_dim = self.config['model']['hidden_dim']
        num_layers = self.config['model']['num_layers']

        example_decoder_input = torch.tensor([self.config['bos_idx']], dtype=torch.long, device=self.device)  # [1]

        example_hidden = torch.randn(num_layers, 1, hidden_dim, device=self.device)  # [num_layers, 1, hidden_dim]
        example_cell = torch.randn(num_layers, 1, hidden_dim, device=self.device)  # [num_layers, 1, hidden_dim]

        example_encoder_outputs = torch.randn(1, seq_len, hidden_dim,
                                              device=self.device)  # [1, src_seq_len, hidden_dim]

        torch.onnx.export(
            decoder_onnx,
            (example_decoder_input, example_hidden, example_cell, example_encoder_outputs),
            str(onnx_dir / "decoder.onnx"),
            input_names=["decoder_input", "hidden", "cell", "encoder_outputs"],
            output_names=["output", "hidden_new", "cell_new", "attention_weights"],
            dynamic_axes={
                "decoder_input": {0: "decoder_seq_len"},
                "encoder_outputs": {1: "src_seq_len"},
                "output": {0: "vocab_size"},
                "attention_weights": {0: "src_seq_len"},
                "hidden": {1: "batch_size"},
                "cell": {1: "batch_size"},
                "hidden_new": {1: "batch_size"},
                "cell_new": {1: "batch_size"}
            },
            opset_version=14,
            do_constant_folding=True,
            export_params=True,
            verbose=False
        )

        try:
            for model_name in ["encoder.onnx", "decoder.onnx"]:
                model_path = str(onnx_dir / model_name)
                model_onnx = onnx.load(model_path)
                model_onnx, check = onnxsim.simplify(model_onnx)
                if check:
                    onnx.save(model_onnx, model_path)
                    print(f"Simplified {model_name}")
                else:
                    print(f"Failed to simplify {model_name}")
        except Exception as e:
            print(f"Simplification failed: {e}")

        with open(onnx_dir / "config.json", 'w', encoding='utf-8') as f:
            config_json = {
                "$version": "1.0",
                "level": 1,
                "schema": {},
                "configuration": {
                    "encoder": "encoder.onnx",
                    "decoder": "decoder.onnx",
                    "charVocab": "char.json",
                    "phonemeVocab": "phoneme.json"
                }
            }
            json.dump(config_json, f, indent=4)

        with open(onnx_dir / "char.json", 'w', encoding='utf-8') as f:
            json.dump(self.model.char_vocab, f, indent=4)

        with open(onnx_dir / "phoneme.json", 'w', encoding='utf-8') as f:
            json.dump(self.model.phoneme_vocab, f, indent=4)

        print(f"LSTM G2p Onnx exported to: {onnx_dir}")
        return onnx_dir


def export_lstm_g2p_to_onnx(ckpt_path: str, onnx_dir: str, config_path: str = "config.yaml"):
    with open(config_path, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)
    model = LstmG2p.load_from_checkpoint(ckpt_path, config=config)
    model.eval()

    exporter = LstmG2pOnnxExporter(model)
    result_dir = exporter.export_components(onnx_dir)
    return str(result_dir)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Export LSTM G2P to Onnx')
    parser.add_argument('--ckpt_path', type=str, required=True, help='checkpoint path')
    parser.add_argument('--onnx_dir', type=str, required=True, help='onnx output directory')
    parser.add_argument('--config_path', type=str, default="config.yaml", help='config file path')

    args = parser.parse_args()
    result = export_lstm_g2p_to_onnx(args.ckpt_path, args.onnx_dir, args.config_path)
