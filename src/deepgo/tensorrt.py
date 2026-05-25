from pathlib import Path

import torch
import torch.nn as nn

from .config import DEFAULT_BATCH_SIZE, MODEL_INPUT_SIZE

try:
    import torch_tensorrt  # type: ignore
except (ImportError, OSError) as e:
    torch_tensorrt = None  # type: ignore
    TENSORRT_IMPORT_ERROR: Exception | None = e
else:
    TENSORRT_IMPORT_ERROR = None


def is_tensorrt_available() -> bool:
    '''Returns True if the environment supports creating TensorRT models.'''
    return torch_tensorrt is not None


def save_tensorrt_model(
    path: str | Path,
    model: nn.Module,
    *,
    batch_size: int = DEFAULT_BATCH_SIZE,
    dtype: torch.dtype = torch.float16,
    require_full_compilation: bool = False,
    truncate_long_and_double: bool = False,
) -> None:
    '''Convert a PyTorch/TorchScript model to a TensorRT model and save it.
    Args:
        path: File path to save the model
        model: PyTorch model to convert
        batch_size: Batch size
        dtype: Data type
        require_full_compilation: Whether to require full compilation
        truncate_long_and_double: Whether to truncate long and double types to float
    '''
    if torch_tensorrt is None:
        raise ImportError(f'torch_tensorrt is not available: {TENSORRT_IMPORT_ERROR}')

    if not torch.cuda.is_available():
        raise RuntimeError('CUDA is not available.')

    device = torch.device('cuda:0')
    model.to(device, dtype)
    model.eval()

    # To pass to torch_tensorrt.compile(ir='torchscript'), a plain nn.Module must
    # first be converted to TorchScript to fix the input shape.
    if not isinstance(model, torch.jit.ScriptModule):
        inputs = torch.zeros((batch_size, MODEL_INPUT_SIZE), device=device, dtype=dtype)

        with torch.inference_mode():
            model = torch.jit.trace(model, inputs)

    input_spec = torch_tensorrt.Input(
        shape=(batch_size, MODEL_INPUT_SIZE),
        dtype=dtype)

    with torch.inference_mode():
        trt_model = torch_tensorrt.compile(
            model,
            ir='torchscript',
            inputs=[input_spec],
            enabled_precisions={dtype},
            require_full_compilation=require_full_compilation,
            truncate_long_and_double=truncate_long_and_double,
        )

    torch.jit.save(trt_model, path)
