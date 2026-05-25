import argparse
import logging
from pathlib import Path

import torch
from deepgo.config import DEFAULT_BATCH_SIZE
from deepgo.log import start_logging
from deepgo.tensorrt import save_tensorrt_model

LOGGER = logging.getLogger(__name__)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Compile the TorchScript model to TensorRT model.')
    parser.add_argument('input', type=str, help='Path to the input TorchScript model.')
    parser.add_argument('output', type=str, help='Path to the output TensorRT model.')
    parser.add_argument(
        '--batch-size', type=int, default=DEFAULT_BATCH_SIZE,
        help=f'Batch size for the TensorRT model. (default: {DEFAULT_BATCH_SIZE})')
    parser.add_argument(
        '--gpu', type=int, default=0,
        help='GPU device index to use for compilation. (default: 0)')
    parser.add_argument(
        '--fp16', action='store_true', default=False,
        help='Use FP16 precision for the TensorRT model.')
    parser.add_argument(
        '--no-require-full-compilation', action='store_true',
        help='Do not require the full compilation of the model.')
    parser.add_argument(
        '--truncate-long-and-double', action='store_true',
        help='Truncate long and double data types to float.')
    parser.add_argument(
        '--verbose', default=False, action='store_true', help='Print debug information')
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    # Configure logging output
    start_logging(debug=args.verbose)

    # Check if GPU is available
    if not torch.cuda.is_available():
        LOGGER.error('CUDA is not available. Please check your GPU and CUDA installation.')
        return

    # Check the output file and exit if it already exists
    output_path = Path(args.output)

    if output_path.exists():
        LOGGER.error('Output file %s already exists. Please choose a different path.', output_path)
        return

    # Load the model
    device = torch.device(f'cuda:{args.gpu}')
    dtype = torch.float16 if args.fp16 else torch.float32

    model = torch.jit.load(args.input, map_location=device)
    model.to(device, dtype)
    model.eval()

    # Convert to TensorRT model and save
    try:
        save_tensorrt_model(
            path=output_path,
            model=model,
            batch_size=args.batch_size,
            dtype=dtype,
            require_full_compilation=not args.no_require_full_compilation,
            truncate_long_and_double=args.truncate_long_and_double,
        )
    except (ImportError, RuntimeError) as e:
        LOGGER.error('Failed to compile TensorRT model: %s', e)
        return

    LOGGER.info('saved: %s', output_path)


if __name__ == '__main__':
    main()
