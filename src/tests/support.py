'''Small CPU models and independent input decoding for engine regression tests.'''

from pathlib import Path

import numpy as np
import torch

from deepgo.config import (
    MODEL_BOARD_SIZE, MODEL_INPUT_SIZE, MODEL_OUTPUT_SIZE, MODEL_POLICY_SIZE,
    MODEL_TERRITORY_SIZE, MODEL_VALUE_SCALE,
)


def unpack(inputs: np.ndarray) -> np.ndarray:
    '''Decode packed inputs (ndarray) into model features (float32 ndarray).
    Returns:
        np.ndarray: Computed result.
    '''
    # Decode binary features independently of the C++ and training implementations.
    bits = (inputs[:, :-2, None].astype(np.uint32) >> np.arange(32)) & 1
    expanded = bits.reshape(len(inputs), -1)[:, :MODEL_INPUT_SIZE].astype(np.float32)
    expanded[:, 11917:11919] = inputs[:, -2:] / MODEL_VALUE_SCALE
    return expanded


class ProbeModel(torch.nn.Module):
    '''Expose a contiguous range of input features through the native output buffer.'''

    def __init__(self, offset: int) -> None:
        '''Select the input offset (int); return None.
        Returns:
            None: No return value.
        '''
        super().__init__()
        self.offset = offset

    def forward(self, inputs: torch.Tensor) -> torch.Tensor:
        '''Map input features (Tensor) into an output-sized window (Tensor).
        Args:
            inputs (torch.Tensor): Inputs.
        Returns:
            torch.Tensor: Computed result.
        '''
        return inputs[:, self.offset:self.offset + MODEL_OUTPUT_SIZE].contiguous()


class SearchModel(torch.nn.Module):
    '''Return predictable policy, territory, and value outputs for CPU search tests.'''

    def forward(self, inputs: torch.Tensor) -> torch.Tensor:
        '''Map inputs (Tensor) to deterministic engine predictions (Tensor).
        Returns:
            torch.Tensor: Computed result.
        '''
        # Prefer pass and give all on-board moves positive probability.
        base = inputs[:, :1] * 0
        policy = base.expand(-1, MODEL_POLICY_SIZE) + 0.001
        policy = policy.clone()
        policy[:, MODEL_BOARD_SIZE**2] = 0.99
        # Predict neutral territory and a fixed score from the side-to-move perspective.
        territories = base.expand(-1, MODEL_TERRITORY_SIZE).clone()
        territories[:, 361:722] = 1.0
        values = torch.cat((base + 0.75, base, base + 0.3), dim=1)
        return torch.cat((policy, territories, values), dim=1)


def save_search_model(path: Path) -> None:
    '''Save a test TorchScript model at path (Path); return None.
    Returns:
        None: No return value.
    '''
    # Keep model generation independent of training code and trained weights.
    model = SearchModel().eval()
    inputs = torch.zeros((1, MODEL_INPUT_SIZE))
    torch.jit.trace(model, inputs).save(str(path))
