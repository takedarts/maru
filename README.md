# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## Overview
Maru is a computer Go program developed using deep reinforcement learning from randomly generated game records. The deep learning model of Maru incorporates large-kernel depthwise convolutions and multi-head attention, enabling it to efficiently grasp the overall state of the board. Its reinforcement learning procedure is inspired by approaches used in Katago and Gumbel AlphaZero, allowing it to efficiently learn a wide range of patterns.

Maru is a sibling program of the computer Shogi program Gokaku. Maru shares the same deep learning model architecture, search algorithm, and reinforcement learning methodology as Gokaku.

You can check the improvement of Maru's playing strength through reinforcement learning on [this page](https://takeda-lab.jp/maru/).
The model files are available for download from [this release page](https://github.com/takedarts/maru/releases/tag/v8.0).

## How to Run
Maru can be run using one of the following methods:

- [Running from Source Files](#running-from-source-files)
- [Running with Docker](#running-with-docker)

## Running from Source Files
### Build Instructions
Since most parts of this program are written in Cython and C++, the program code must be compiled before it can be run.

First, install the required modules for compilation:
```
pip install numpy cython cmake
```

PyTorch is required for compilation, so please install the appropriate version of this module according to your execution environment, such as CUDA.
```
pip install torch
```

Next, run `src/build.py` to compile the Cython and C++ code.
On Linux or macOS environments, `make` is required.
On Windows environments, `MSBuild` is required (MSBuild is included with Visual Studio).
```
python -X utf8 src/build.py
```

If compilation is successful, the compiled Cython module will be generated in `src/deepgo/native`.

You can delete the generated files by running the following command:
```
python src/build.py --clean
```

### Running the Program
You can launch Maru by running the launch script `src/run.py`.
At runtime, you need to specify the model file as a command-line argument (You can download the model file from [here](https://github.com/takedarts/maru/releases/tag/v8.0)):
```
python src/run.py <model_file>
```


Maru operates via the GTP (Go Text Protocol).
Here is a simple example of usage:
```
% python src/run.py b5n1c96-059f8d56.model
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

Since Maru supports the GTP protocol, you can also use GUI applications that support GTP, such as [Gogui](https://github.com/Remi-Coulom/gogui) and [Lizzie](https://github.com/featurecat/lizzie).

To see the available options, run the script with the --help flag:
```
python src/run.py --help
```

## Running with Docker
A Docker image is available for running Maru easily.
If you have CUDA installed on your system, you can retrieve and run the Maru Docker image using the following commands  (You can download the model file from [here](https://github.com/takedarts/maru/releases/tag/v8.0)):
```
docker pull takedarts/maru:cuda
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:cuda /opt/run.sh <model_file>
```
Use the `--gpus` option to specify the GPUs to use, and mount the current directory to the container's `/workspace` using `-v .:/workspace`.
Place the model file in the current directory and specify its path as `<model_file>`.

To see the available options, use the `--help` flag:
```
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:cuda /opt/run.sh --help
```

For environments without GPU (CPU only), use the following commands:
```
docker pull takedarts/maru:cpu
docker run -it --rm -v .:/workspace takedarts/maru:cpu /opt/run.sh <model_file>
```
