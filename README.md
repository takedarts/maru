# Maru

[English](./README.md) | [Japanese](./README_JP.md)

## Overview

Maru is a computer Go program developed through deep reinforcement learning from game records
of random moves. Its deep learning models use multi-head attention with a bottleneck structure,
and its reinforcement learning procedure draws on KataGo and Gumbel AlphaZero.

Maru is a sibling of the computer Shogi program [Gokaku](https://github.com/takedarts/gokaku).
They use the same methods for model architecture, search algorithms, and reinforcement learning.
Progress in playing strength is available on [this page](https://takeda-lab.jp/maru/).

A model file is required to run Maru.
TorchScript models can be downloaded from [here](https://github.com/takedarts/maru/releases/tag/v8.3).
Use `src/compile.py` to create TensorRT models from TorchScript models.

## How to Run

- [Running from Source Files](#running-from-source-files)
- [Running with Docker](#running-with-docker)

Running from source requires building the native extension.
For TensorRT support, build from source in an environment with Torch-TensorRT installed.

## Running from Source Files

### Build Instructions

Most of this program is written in Cython and C++.
To run Maru, you need to build the program to create the module that performs search and
board evaluation.

First, install the modules required to build and run the program.

```
pip install numpy cython cmake
```

In addition to these modules, PyTorch is required to build and run the program.
Check the CUDA version installed in your environment and other requirements, then install
an appropriate PyTorch build. Maru should also run in a ROCm environment with a ROCm-compatible
PyTorch build (ROCm has not been tested, and TensorRT cannot be used with ROCm).

```
pip install torch
```

To run Maru with TensorRT, also install Torch-TensorRT.
If Torch-TensorRT is not installed, the module is built without TensorRT support.

```
pip install torch-tensorrt
```

Next, run `src/build.py` to build the Cython and C++ code.
Linux and macOS require `make`.
Windows requires `MSBuild`, which is included in Visual Studio.

```
python src/build.py
```

After successful compilation, the compiled Cython module is generated in `src/deepgo/native`.

Run `src/build.py` with the `--clean` option to remove the generated files.

```
python src/build.py --clean
```

### Running the Program

Run the startup script `src/run.py` to start Maru.
Pass a TorchScript or TensorRT model as its argument
(TorchScript models can be downloaded from [here](https://github.com/takedarts/maru/releases/tag/v8.3).
Use `src/compile.py` to create TensorRT models).

```
python src/run.py <model_file>
```

Maru is controlled through GTP (Go Text Protocol).
The following is a simple example session.

```
% python src/run.py b16c256-645.model
boardsize 9
=

genmove b
= F6

genmove w
= D4

showboard
=
   A B C D E F G H J
 9 . . . . . . . . . 9
 8 . . . . . . . . . 8
 7 . . . . . . . . . 7
 6 . . . + . X . . . 6
 5 . . . . . . . . . 5
 4 . . . O . . . . . 4
 3 . . . . . . . . . 3
 2 . . . . . . . . . 2     WHITE (O) has captured 0 stones
 1 . . . . . . . . . 1     BLACK (X) has captured 0 stones
   A B C D E F G H J
```

Maru follows GTP, so you can also use GTP-compatible GUIs such as
[GoGui](https://github.com/Remi-Coulom/gogui) and [Lizzie](https://github.com/featurecat/lizzie).

Pass the `--help` option to the startup script to display the available options.

```
python src/run.py --help
```

### Running with TensorRT

If Maru was built in an environment with Torch-TensorRT installed, it can run with TensorRT.
First, compile Maru's inference model into a TensorRT model.
Run the following command to compile the model file into a TensorRT model.

```
python src/compile.py <torch-script-file> <tensorrt-file>
```

Specify a TorchScript model as `<torch-script-file>`.
The command generates a TensorRT model at `<tensorrt-file>`.

To start Maru with TensorRT, pass the generated TensorRT model to the startup script.

```
python src/run.py <tensorrt-file>
```

When running Maru with TensorRT, the TensorRT model compilation options must match the
execution options for `src/run.py`.
If you specify `--fp16` or `--batch-size` when compiling the TensorRT model, specify the same
options when running `src/run.py`.
The following example compiles a TensorRT model with half precision (FP16) and a batch size
of 16, then starts Maru with that model (the TensorRT model only needs to be compiled once).

```
python src/compile.py --fp16 --batch-size 16 b16c256-645.model b16c256-645.rt.model
python src/run.py --fp16 --batch-size 16 b16c256-645.rt.model
```

## Running with Docker

### Running in CUDA-enabled Environments

Docker images are available to make it easy to run Maru (TensorRT is not supported).

To run Maru on a GPU with Docker, you need an NVIDIA driver supporting CUDA 12.6 or later
and NVIDIA Container Toolkit installed.
If the following command can access the GPU from Docker, you can run Maru on a GPU with Docker.

```
docker run --rm -i --gpus all pytorch/pytorch:2.12.0-cuda12.6-cudnn9-runtime nvidia-smi
```

You need to download the Docker image before running it for the first time.
The CUDA image, `takedarts/maru:v8.3-cuda12.6`, is approximately 4 GB, so it may take some time
to download. We recommend downloading it in advance with the following command.

```
docker pull takedarts/maru:v8.3-cuda12.6
```

In an environment that supports CUDA, run Maru's Docker image with the following command.

```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.3-cuda12.6 /opt/run.sh <model_file>
```

The `--gpus` option specifies which GPUs to use, and `-v .:/workspace` mounts the current
directory at `/workspace` inside the container.
Place the model file under the current directory and specify its path as `<model_file>`.

You can also append options to the execution command.
Specify `--help` to display the available options.

```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.3-cuda12.6 /opt/run.sh --help
```

### Running on CPU

The `takedarts/maru:v8.3-cpu` Docker image is also available for CPU execution on AMD64
and is smaller than the CUDA image.
To run on a CPU, use the following command.

```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.3-cpu /opt/run.sh <model_file>
```

For execution on an ARM64 CPU, use the `takedarts/maru:v8.3-arm` Docker image.

```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.3-arm /opt/run.sh <model_file>
```

## Execution Options

| Option | Description | Default |
|---|---|---|
| `--help` | Display available options |  |
| `--visits <N>` | Target search visits | 50 |
| `--max-visits <N>` | Maximum search visits | 1,000,000 |
| `--criterion <S>` | Candidate priority criterion: `value` or `visits` | `value` |
| `--temperature <R>` | Search temperature | 1.0 |
| `--randomness <R>` | Variation range in target visits | 0.0 |
| `--rule <R>` | Game rules: `ch`, `jp`, or `com` | `ch` |
| `--boardsize <N>` | Board size | 19 |
| `--komi <K>` | Komi | 7.5 |
| `--superko` | Enable superko | False |
| `--pucb-constant-init <R>` | Initial PUCB constant | 0.6 |
| `--pucb-constant-base <R>` | Base PUCB constant | 1600.0 |
| `--pucb-min-visits-rate <R>` | Minimum child visit ratio prioritized by PUCB | 0.0 |
| `--timelimit <N>` | Thinking time limit in seconds | 120 |
| `--ponder` | Continue searching during the opponent's turn | False |
| `--resign <R>` | Predicted win probability threshold for resignation | 0.02 |
| `--min-score <R>` | Minimum score difference for resignation | 0.0 |
| `--min-turn <N>` | Minimum turn before resignation | 100 |
| `--initial-turn <N>` | Opening turns with random moves | 4 |
| `--client-name <S>` | Displayed engine name | `Maru` |
| `--client-version <S>` | Displayed version | `8.3` |
| `--threads <N>` | Search threads | 16 |
| `--display <S>` | GTP board display command |  |
| `--sgf <S>` | SGF file to load as the initial position |  |
| `--batch-size <N>` | Inference batch size | 32 |
| `--gpus <N,...>` | GPU IDs; use `-1` for CPU | All available GPUs, or CPU if none are available |
| `--fp16` | Use FP16 inference | False |
| `--threads-per-gpu <N>` | Inference threads per GPU | 2 |
| `--cache-size <N>` | Maximum inference cache entries | visits |
| `--verbose` | Write debug logs to standard error | False |

### Visits and Thinking Time

- Search ends when any of the following conditions is met:
  - The visit count reaches the target specified by `--visits`.
  - The visit count reaches the maximum specified by `--max-visits`.
  - The time limit specified by `--timelimit` is reached.
  - Search ends early if the most visited child's visit count exceeds 60% of the target
    specified by `--visits`.
- The search tree is reused after moves.

### Rules and Temperature

- Specify `ch` to use Chinese rules for calculations.
- Specify `jp` to use Japanese rules for calculations.
- Specify `com` to use Chinese rules with dead-stone cleanup at the end of the game.
- Increasing the search temperature widens exploration; decreasing it narrows exploration.
- GTP `play` commands are sent to the display command specified by `--display`.

## Execution Examples

To start Maru with the model file `b16c256-645.model`, run the following command.

```
python src/run.py b16c256-645.model
```

To start Maru with a visit count of 1000 and a thinking time limit of 5 seconds, run the
following command.

```
python src/run.py b16c256-645.model --visits 1000 --timelimit 5
```

## Compatibility with Earlier Versions

Maru version 8.3 changes the model specification, including the output format, so models
from Maru version 8.2 or earlier cannot be used.

## Tests

After building, run the CPU board, inference, search, GTP, and SGF tests and type checks.

```
pip install mypy
PYTHONPATH=src python -m unittest discover -s src/tests -p '*_test.py' -v
MYPYPATH=src python -m mypy --explicit-package-bases src
```

The tests use small generated models, so no trained models or GPUs are required.
See [src/tests/README.md](src/tests/README.md) for details.

## License

Maru is distributed under the MIT License.
Some CMake helper scripts are provided under the Apache License 2.0.
