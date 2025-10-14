# LSTM G2P: Grapheme-to-Phoneme conversion model

## Introduction:

The Grapheme-to-Phoneme (G2P) conversion model, based on the LSTM encoder-decoder architecture, supports
character-to-phoneme conversion for multiple languages. The model incorporates an attention mechanism, enabling it to
effectively handle variable-length sequences and is suitable for text-to-pronunciation conversion tasks.

## Quick Start

### Create Environment

1. Clone the repository:
   ```bash
   git clone https://gitclone.com/github.com/wolfgitpr/HubertFA.git
   conda create -n G2p python=3.10 -y
   conda activate G2p
   ```

2. Install dependencies:

   install [pytorch](https://pytorch.org/get-started/locally/)

    ```bash
    pip install -r requirements.txt
    ```

### Infer

```bash
python infer.py --ckpt_path path/to/ckpt --word "hello"
```

args:

- `--ckpt`:checkpoint path
- `--word`: word for inference
- `--config_path`: (optional) config file path
- `--vocab_path`: (optional) vocab file path
- `--max_len`: max length of phonemes

### train

1. prepare data

   Custom Dictionary: [dataset](data/dataset.py) _preprocess_data

   Cmudict:

   Word (two spaces) phoneme1 (one space) phoneme2

   ```
    hello  HH AH L OW
    world  W ER L D
    test   T EH S T
   ```

2. make config

   Modify the graphemes and phonemes in the config file.

   ```bash
    vocab:
      graphemes:
        - "a"
        - "b"
        - ...
      phonemes:
        - "aa"
        - "ae"
        - ...
   ```

3. `python train.py --config/-c config.yaml`
4. Training Visualization: `tensorboard --logdir=ckpt/`。

