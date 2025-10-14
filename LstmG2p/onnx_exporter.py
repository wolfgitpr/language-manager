import argparse
import pathlib
import onnx
import onnxsim
import torch
import yaml

from models.lstm_g2p import LstmG2p


class LstmG2pOnnx(torch.nn.Module):
    def __init__(self, model, config, max_len_for_onnx: int):
        super().__init__()
        self.config = config
        self.model_config: dict = config['model']
        self.encoder = model.encoder
        self.decoder = model.decoder
        self.bos_idx = self.config['bos_idx']
        self.eos_idx = self.config['eos_idx']
        self.pad_idx = self.config['pad_idx']
        self.max_len_for_onnx = max_len_for_onnx

    def forward(self, src: torch.Tensor):
        src_batch = src.unsqueeze(0)
        encoder_outputs, hidden, cell = self.encoder(src_batch)

        decoder_input = torch.tensor([[self.bos_idx]],
                                     dtype=torch.long, device=src.device)

        raw_phoneme_ids = []
        finished = torch.tensor(0, dtype=torch.long, device=src.device)

        for t in range(self.max_len_for_onnx):
            decoder_input_effective = torch.where(
                finished == 0,
                decoder_input,
                torch.tensor([[self.pad_idx]], device=src.device, dtype=torch.long)
            )

            output, hidden, cell, _ = self.decoder(decoder_input_effective, hidden, cell, encoder_outputs)
            pred_token = output.argmax(2)

            current_output = torch.where(
                finished == 0,
                pred_token,
                torch.tensor([[self.pad_idx]], device=src.device, dtype=torch.long)
            )

            raw_phoneme_ids.append(current_output)

            current_eos = (pred_token.squeeze() == self.eos_idx).long()
            finished = torch.maximum(finished, current_eos)

            decoder_input = pred_token

        phoneme_ids = torch.cat(raw_phoneme_ids, dim=1)  # [1, seq_len]

        phoneme_seq = phoneme_ids.squeeze(0)

        eos_mask = (phoneme_seq == self.eos_idx).long()
        positions = torch.arange(phoneme_seq.size(0), device=src.device)
        eos_positions = torch.where(
            eos_mask == 1,
            positions,
            torch.tensor(phoneme_seq.size(0) + 1, device=src.device)
        )
        first_eos_pos = torch.min(eos_positions)

        has_eos = torch.any(eos_mask == 1).long()
        final_length = torch.where(
            has_eos == 1,
            first_eos_pos + 1,
            torch.tensor(phoneme_seq.size(0), device=src.device)
        )

        final_length = torch.minimum(final_length, torch.tensor(phoneme_seq.size(0), device=src.device))
        return phoneme_seq[:final_length]


class LstmG2pOnnxExporter:
    def __init__(self, model):
        self.model = model
        self.config = model.config
        self.device = next(model.parameters()).device

    def export(self, onnx_path: str, max_output_seq_len: int = 48):
        example_word_chars = list("antidisestablishmentarianism")
        example_word_indices = [self.model.char_vocab.get(c, self.config['unk_idx']) for c in example_word_chars]
        example_input = torch.tensor([self.config['bos_idx']] + example_word_indices + [self.config['eos_idx']],
                                     dtype=torch.long, device=self.device)

        wrapper = LstmG2pOnnx(self.model, self.config, max_output_seq_len)
        wrapper.eval()

        torch.onnx.export(
            wrapper,
            (example_input,),
            onnx_path,
            input_names=["input_ids"],
            output_names=["phoneme_ids"],
            dynamic_axes={
                "input_ids": {0: "src_seq_len"},
                "phoneme_ids": {0: "tgt_seq_len"}
            },
            opset_version=14,
            do_constant_folding=True,
            export_params=True,
            keep_initializers_as_inputs=True,
            verbose=False
        )

        try:
            model_onnx = onnx.load(onnx_path)
            model_onnx, check = onnxsim.simplify(model_onnx)
            if check:
                onnx.save(model_onnx, onnx_path)
                print("Onnx simplified model saved to {}".format(onnx_path))
            else:
                print("Onnx failed to simplify.")
        except Exception as e:
            print(f"Onnx failed: {e}")

        print(f"模型已导出到: {onnx_path}")
        return onnx_path

    def vocab_data(self):
        return {"char_vocab": self.model.char_vocab, "phoneme_vocab": self.model.phoneme_vocab,
                "idx_to_phoneme": self.model.idx_to_phoneme}


def export_lstm_g2p_to_onnx(ckpt_path: str, onnx_dir: str, config_path: str = "config.yaml",
                            max_output_seq_len: int = 48):
    with open(config_path, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)

    model = LstmG2p.load_from_checkpoint(ckpt_path, config=config)
    model.eval()

    onnx_dir = pathlib.Path(onnx_dir)
    onnx_dir.mkdir(parents=True, exist_ok=True)

    exporter = LstmG2pOnnxExporter(model)

    model_path = exporter.export(str(onnx_dir / "model.onnx"),
                                 max_output_seq_len=max_output_seq_len)

    with open(onnx_dir / "config.yaml", 'w', encoding='utf-8') as f:
        yaml.dump(config, f, default_flow_style=False, allow_unicode=True)

    with open(onnx_dir / "vocab.yaml", 'w', encoding='utf-8') as f:
        yaml.dump(exporter.vocab_data(), f, default_flow_style=False, allow_unicode=True)

    print(f"LSTM G2p Onnx exported to: {model_path}")
    return model_path


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Export LSTM G2P to Onnx')
    parser.add_argument('--ckpt_path', type=str, required=True, help='checkpoint path')
    parser.add_argument('--onnx_dir', type=str, required=True, help='onnx output directory')
    parser.add_argument('--config_path', type=str, default="config.yaml", help='config file path')
    parser.add_argument('--max_output_seq_len', type=int, default=48, help='max output sequence length')

    args = parser.parse_args()
    result = export_lstm_g2p_to_onnx(args.ckpt_path, args.onnx_dir, args.config_path, args.max_output_seq_len)
