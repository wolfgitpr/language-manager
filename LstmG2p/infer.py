import argparse
import pathlib

import torch
import yaml

from models.lstm_g2p import LstmG2p


def main():
    parser = argparse.ArgumentParser(description='LSTM G2P inference')
    parser.add_argument('--ckpt_path', type=str, required=True, help='checkpoint path')
    parser.add_argument('--word', type=str, help='word for inference')
    parser.add_argument('--config_path', type=str, default=None, help='config file path')
    parser.add_argument('--vocab_path', type=str, help='vocab file path')
    parser.add_argument('--max_len', type=int, default=48, help='max length of phonemes')

    args = parser.parse_args()

    config_path = args.config_path if args.config_path is not None else str(
        pathlib.Path(args.ckpt_path).parent / 'config.yaml')

    with open(config_path, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)

    model = LstmG2p.load_from_checkpoint(args.ckpt_path, config=config)
    model.to('cuda' if torch.cuda.is_available() else 'cpu')
    model.eval()

    phonemes = model.predict(args.word, args.max_len, beam_size=3)
    print(f"\nresult:")
    print(f"word: {args.word}")
    print(f"phoneme_string: {' '.join(phonemes)}")
    print(f"phonemes: {phonemes}")
    print(f"phonemes size: {len(phonemes)}")


if __name__ == '__main__':
    main()
