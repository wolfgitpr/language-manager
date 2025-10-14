import argparse
import pathlib
import shutil

import lightning as pl
from lightning.pytorch.callbacks import EarlyStopping
from lightning.pytorch.loggers import TensorBoardLogger
from torch.utils.data import DataLoader, random_split

from data.dataset import CMUDictDataset, collate_fn
from models.lstm_g2p import LstmG2p
from tools.config_utils import load_yaml
from tools.train_callbacks import MonitorCheckpointsCallback


def train(config_path):
    config = load_yaml(config_path)
    save_model_folder = pathlib.Path("ckpt") / config["model_name"]
    save_model_folder.mkdir(parents=True, exist_ok=True)

    training_config = config['training']
    shutil.copy(config_path, save_model_folder)

    model = LstmG2p(config)
    logger = TensorBoardLogger(save_dir=save_model_folder)

    checkpoint_callback = MonitorCheckpointsCallback(
        dirpath=save_model_folder,
        monitor="val/seq_acc",
        mode="max",
        save_top_k=3,
    )

    early_stop_callback = EarlyStopping(
        monitor='val/seq_acc',
        patience=training_config['patience'],
        mode='max'
    )

    trainer = pl.Trainer(
        max_epochs=training_config['max_epochs'],
        logger=logger,
        callbacks=[checkpoint_callback, early_stop_callback],
        log_every_n_steps=50,
        accelerator='auto',
        devices='auto',
        accumulate_grad_batches=training_config['accumulate_grad_batches'],
        gradient_clip_val=training_config['gradient_clip_val']
    )

    dataset = CMUDictDataset(config)
    train_size = int(config['training']['train_ratio'] * len(dataset))
    val_size = len(dataset) - train_size
    train_dataset, val_dataset = random_split(dataset, [train_size, val_size])

    train_loader = DataLoader(
        train_dataset,
        batch_size=config['training']['batch_size'],
        shuffle=True,
        pin_memory=True,
        collate_fn=collate_fn
    )

    val_loader = DataLoader(
        val_dataset,
        batch_size=config['training']['batch_size'],
        shuffle=False,
        pin_memory=True,
        collate_fn=collate_fn
    )

    trainer.fit(model, train_loader, val_loader)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='LSTM G2P train')
    parser.add_argument('--config_path', type=str, default='config.yaml', help='config file path')

    args = parser.parse_args()
    train(args.config_path)
