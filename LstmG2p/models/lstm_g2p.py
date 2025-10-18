import lightning as pl
import torch
import torch.nn as nn

from models.decoder import LstmDecoder
from models.encoder import LstmEncoder


class LstmG2p(pl.LightningModule):
    def __init__(self, config: dict):
        super(LstmG2p, self).__init__()
        self.save_hyperparameters()

        self.config = config
        self.max_phoneme_len: int = config['model']['max_phoneme_len']

        self.graphemes = config['vocab']['graphemes']
        self.phonemes = config['vocab']['phonemes']

        self.special_symbols = {v: k for k, v in enumerate(config['special_symbols'])}
        self.char_vocab = {**self.special_symbols,
                           **{v: k + len(self.special_symbols) for k, v in enumerate(self.graphemes)}}
        self.phoneme_vocab = {**self.special_symbols,
                              **{v: k + len(self.special_symbols) for k, v in enumerate(self.phonemes)}}
        self.idx_to_phoneme = {v: k for k, v in self.phoneme_vocab.items()}

        self.char_vocab_size = len(self.char_vocab)
        self.phoneme_vocab_size = len(self.phoneme_vocab)

        self.teacher_forcing_ratio = config['training']['teacher_forcing_ratio']

        self.model_config: dict = config['model']
        self.encoder = LstmEncoder(
            len(self.char_vocab),
            embedding_dim=self.model_config['embedding_dim'],
            hidden_dim=self.model_config['hidden_dim'],
            num_layers=self.model_config['num_layers'],
            dropout=self.model_config['dropout'],
            padding_idx=config['pad_idx']
        )
        self.decoder = LstmDecoder(
            len(self.phoneme_vocab),
            embedding_dim=self.model_config['embedding_dim'],
            hidden_dim=self.model_config['hidden_dim'],
            num_layers=self.model_config['num_layers'],
            dropout=self.model_config['dropout'],
            padding_idx=config['pad_idx']
        )

        self.criterion = nn.CrossEntropyLoss(ignore_index=1, label_smoothing=0.08)

    def on_train_epoch_start(self):
        max_epochs = self.config['training']['max_epochs']
        current_epoch = self.current_epoch
        self.teacher_forcing_ratio = max(0.1, 1.0 - current_epoch / max_epochs)

    def forward(self, src: torch.Tensor, tgt: torch.Tensor, teacher_forcing_ratio: float = None):
        tgt_len = tgt.shape[1]

        encoder_outputs, hidden, cell = self.encoder(src)
        decoder_input = tgt[:, 0].unsqueeze(1)

        all_step_outputs = []

        for t in range(1, tgt_len):
            output, hidden, cell, _ = self.decoder(decoder_input, hidden, cell, encoder_outputs)
            all_step_outputs.append(output)  # output: [batch_size, 1, vocab_size]

            use_teacher_forcing = torch.rand(1).item() < self.teacher_forcing_ratio \
                if teacher_forcing_ratio is None else teacher_forcing_ratio
            decoder_input = tgt[:, t].unsqueeze(1) if use_teacher_forcing else output.argmax(2)

        outputs = torch.cat(all_step_outputs, dim=1)
        return outputs.reshape(-1, self.phoneme_vocab_size)

    def inference_forward(self, src: torch.Tensor):
        batch_size = src.shape[0]
        encoder_outputs, hidden, cell = self.encoder(src)

        decoder_input = torch.tensor([[self.config['bos_idx']] * batch_size],
                                     dtype=torch.long, device=self.device).transpose(0, 1)

        phoneme_ids = []
        finished = torch.zeros(batch_size, dtype=torch.bool, device=self.device)

        for t in range(self.max_phoneme_len):
            output, hidden, cell, _ = self.decoder(decoder_input, hidden, cell, encoder_outputs)
            pred_token = output.argmax(2)

            phoneme_ids.append(pred_token)

            eos_mask = (pred_token.squeeze(1) == self.config['eos_idx'])
            finished = finished | eos_mask

            if finished.all():
                break

            decoder_input = pred_token

        if phoneme_ids:
            phoneme_ids = torch.cat(phoneme_ids, dim=1)
        else:
            phoneme_ids = torch.zeros(batch_size, 0, dtype=torch.long, device=self.device)

        lengths = torch.zeros(batch_size, dtype=torch.long, device=self.device)
        for i in range(batch_size):
            eos_positions = (phoneme_ids[i] == self.config['eos_idx']).nonzero()
            if len(eos_positions) > 0:
                lengths[i] = eos_positions[0].item() + 1
            else:
                lengths[i] = phoneme_ids.shape[1]

        return phoneme_ids, lengths

    def _compute_metrics(self, output_flat: torch.Tensor, tgt_flat: torch.Tensor,
                         src: torch.Tensor, prefix: str):
        preds = output_flat.argmax(dim=-1)
        non_pad_mask = tgt_flat != self.config['pad_idx']

        char_acc = (preds[non_pad_mask] == tgt_flat[non_pad_mask]).float().mean()

        batch_size = src.size(0)
        tgt_len_minus_1 = tgt_flat.size(0) // batch_size

        seq_preds = output_flat.argmax(dim=-1).view(batch_size, tgt_len_minus_1)
        seq_targets = tgt_flat.view(batch_size, tgt_len_minus_1)

        seq_acc = ((seq_preds == seq_targets) | (seq_targets == self.config['pad_idx'])).all(dim=1).float().mean()

        return {
            f'{prefix}/char_acc': char_acc,
            f'{prefix}/seq_acc': seq_acc
        }

    def training_step(self, batch, batch_idx: int):
        src, tgt = batch
        output_flat = self(src, tgt)

        tgt_flat = tgt[:, 1:].reshape(-1)
        loss = self.criterion(output_flat, tgt_flat)
        self.log("train/loss", loss, prog_bar=True)

        if self.trainer.global_step % 50 == 0:
            metrics = self._compute_metrics(output_flat, tgt_flat, src, 'train')
            for name, value in metrics.items():
                self.log(name, value, prog_bar=True)

        return loss

    def validation_step(self, batch, batch_idx: int):
        src, tgt = batch
        output_flat = self(src, tgt, teacher_forcing_ratio=0)

        tgt_flat = tgt[:, 1:].reshape(-1)
        loss = self.criterion(output_flat, tgt_flat)
        self.log("val/loss", loss)

        metrics = self._compute_metrics(output_flat, tgt_flat, src, 'val')
        for name, value in metrics.items():
            self.log(name, value, prog_bar=True)

        return loss

    def configure_optimizers(self):
        model_config = self.config['model']

        optimizer = torch.optim.AdamW(
            self.parameters(),
            lr=model_config['learning_rate'],
            weight_decay=model_config['weight_decay'],
            betas=(0.9, 0.98)
        )

        scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(
            optimizer, mode='min', factor=0.5, patience=5
        )

        return {
            'optimizer': optimizer,
            'lr_scheduler': {
                'scheduler': scheduler,
                'monitor': 'val/seq_acc',
                'frequency': 1
            }
        }

    def beam_search_decode(self, src: torch.Tensor, max_len: int = None, beam_size: int = 5):
        if max_len is None:
            max_len = self.max_phoneme_len

        batch_size = src.size(0)
        assert batch_size == 1, "beam search requires batch_size == 1"

        encoder_outputs, hidden, cell = self.encoder(src)

        start_token = torch.tensor([[self.config['bos_idx']]], dtype=torch.long, device=self.device)

        beams = [{
            'tokens': [start_token.item()],
            'score': 0.0,
            'hidden': hidden,
            'cell': cell,
            'finished': False
        }]

        for step in range(max_len):
            candidates = []

            all_finished = all(beam['finished'] for beam in beams)
            if all_finished:
                break

            for beam in beams:
                if beam['finished']:
                    candidates.append(beam)
                    continue

                last_token = beam['tokens'][-1]
                decoder_input = torch.tensor([[last_token]], dtype=torch.long, device=self.device)

                output, new_hidden, new_cell, _ = self.decoder(
                    decoder_input, beam['hidden'], beam['cell'], encoder_outputs
                )

                log_probs = torch.log_softmax(output.squeeze(1), dim=-1)
                topk_scores, topk_indices = torch.topk(log_probs, beam_size * 2, dim=-1)  # 取更多候选

                for i in range(beam_size * 2):
                    token = topk_indices[0, i].item()
                    token_score = topk_scores[0, i].item()

                    new_length = len(beam['tokens']) + 1
                    normalized_score = (beam['score'] * (new_length - 1) + token_score) / new_length

                    new_tokens = beam['tokens'] + [token]
                    finished = (token == self.config['eos_idx']) or (len(new_tokens) >= max_len)

                    candidate = {
                        'tokens': new_tokens,
                        'score': beam['score'] + token_score,
                        'normalized_score': normalized_score,
                        'hidden': new_hidden,
                        'cell': new_cell,
                        'finished': finished
                    }
                    candidates.append(candidate)

            candidates.sort(key=lambda x: x['normalized_score'], reverse=True)

            beams = []
            seen_sequences = set()

            for candidate in candidates:
                seq_tuple = tuple(candidate['tokens'])
                if seq_tuple not in seen_sequences:
                    seen_sequences.add(seq_tuple)
                    beams.append(candidate)
                    if len(beams) >= beam_size:
                        break

        best_beam = max(beams, key=lambda x: x['normalized_score'])

        predictions = []
        for token in best_beam['tokens'][1:]:
            if token == self.config['eos_idx']:
                break
            if token not in [self.config['bos_idx'], self.config['pad_idx']]:
                predictions.append(token)

        return [self.idx_to_phoneme[idx] for idx in predictions if idx in self.idx_to_phoneme]

    def predict(self, word: str, max_len: int = None, beam_size: int = 1):
        if max_len is None:
            max_len = self.max_phoneme_len

        with torch.no_grad():
            word_indices = [self.char_vocab.get(c, self.config['unk_idx']) for c in word.lower().strip()]
            word_indices = [self.config['bos_idx']] + word_indices + [self.config['eos_idx']]
            src = torch.tensor([word_indices], dtype=torch.long).to(self.device)

            if beam_size > 1:
                return self.beam_search_decode(src, max_len, beam_size)
            else:
                encoder_outputs, hidden, cell = self.encoder(src)
                decoder_input = torch.tensor([[self.config['bos_idx']]], dtype=torch.long).to(self.device)
                predictions = []

                for _ in range(max_len):
                    output, hidden, cell, _ = self.decoder(decoder_input, hidden, cell, encoder_outputs)
                    pred_token = output.argmax(2).item()

                    if pred_token == self.config['eos_idx']:
                        break

                    if pred_token not in [self.config['bos_idx'], self.config['pad_idx']]:
                        predictions.append(pred_token)

                    decoder_input = output.argmax(2)
                return [self.idx_to_phoneme[idx] for idx in predictions if idx in self.idx_to_phoneme]
