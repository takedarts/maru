# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## Overview
Maru is a computer Go program developed using deep reinforcement learning from randomly generated game records. The deep learning model of Maru incorporates large-kernel depthwise convolutions and multi-head attention, enabling it to efficiently grasp the overall state of the board. Its reinforcement learning procedure is inspired by approaches used in Katago and Gumbel AlphaZero, allowing it to efficiently learn a wide range of patterns.

Maru is a sibling program of the computer Shogi program Gokaku. Maru shares the same deep learning model architecture, search algorithm, and reinforcement learning methodology as Gokaku.

## Build Instructions
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

## How to Run
You can launch Maru by running the launch script `src/run.py`.
At runtime, you need to specify the model file as a command-line argument (You can download the model file from [here](https://github.com/takedarts/maru/releases/tag/v0.0)):
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
