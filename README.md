# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## Overview
Maru is a computer Go program developed using deep reinforcement learning from randomly generated game records. The deep learning model of Maru incorporates nested-bottleneck convolutions and multi-head attention, enabling it to efficiently grasp the overall state of the board. Its reinforcement learning procedure is inspired by approaches used in Katago and Gumbel AlphaZero, allowing it to efficiently learn a wide range of patterns.

Maru is a sibling program of the computer Shogi program [Gokaku](https://github.com/takedarts/gokaku). Maru shares the same deep learning model architecture, search algorithm, and reinforcement learning methodology as Gokaku.

You can check the improvement of Maru's playing strength through reinforcement learning on [this page](https://takeda-lab.jp/maru/).
The model files are available for download from [this release page](https://github.com/takedarts/maru/releases/tag/v8.2).

## How to Run
Maru can be run using one of the following methods:
- [Running from Source Files](#running-from-source-files)
- [Running with Docker](#running-with-docker)

Since no binary files are provided, please either build and run the source code yourself or run it using the Docker image.
However, if you want to run Maru with TensorRT, you need to build from source files.

## Running from Source Files
### Build Instructions
Most parts of this program are written in Cython and C++.
To run Maru, you need to build the program and create the modules that perform search and board evaluation.

First, install the required modules for building and running:
```
pip install numpy cython cmake
```

In addition to the above modules, PyTorch is required for building and running.
Please check the CUDA version installed in your environment and install the appropriate version of PyTorch for your execution environment.
Even in environments with ROCm installed, you should be able to run Maru by installing a ROCm-compatible version of PyTorch (ROCm environment operation has not been verified, and TensorRT cannot be used in ROCm environments).
```
pip install torch
```

If you want to run Maru using TensorRT, please also install Torch-TensorRT.
If Torch-TensorRT is not installed, a module that does not support TensorRT will be built.
```
pip install torch-tensorrt
```

Next, run `src/build.py` to compile the Cython and C++ code.
On Linux or macOS environments, `make` is required.
On Windows environments, `MSBuild` is required (MSBuild is included with Visual Studio).
```
python src/build.py
```

If compilation is successful, the compiled Cython module will be generated in `src/deepgo/native`.

You can delete the generated files by running `src/build.py` with the `--clean` option:
```
python src/build.py --clean
```

### Running the Program
You can launch Maru by running the launch script `src/run.py`.
At runtime, you need to specify the model file as a command-line argument (You can download the model file from [here](https://github.com/takedarts/maru/releases/tag/v8.2)):
```
python src/run.py <model_file>
```

Maru operates via the GTP (Go Text Protocol).
Here is a simple example of usage:
```
% python src/run.py b4c128-645.model
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

Since Maru supports the GTP, you can also use GUI applications that support GTP, such as [Gogui](https://github.com/Remi-Coulom/gogui) and [Lizzie](https://github.com/featurecat/lizzie).

To see the available options, run the launch script with the `--help` option:
```
python src/run.py --help
```

### Running with TensorRT
If you build in an environment where Torch-TensorRT is installed, you can run Maru using TensorRT.
First, to use TensorRT, you need to compile Maru's inference model into a TensorRT model.
Run the following command to compile the model file into a TensorRT model:
```
python src/compile.py <torch-script-file> <tensorrt-file>
```

For `<torch-script-file>`, specify the model file that can be downloaded from [here](https://github.com/takedarts/maru/releases/tag/v8.2).
When you run the above command, a TensorRT format model file will be generated at `<tensorrt-file>`.

You can launch Maru using TensorRT by running the launch script with the created TensorRT model file specified as an argument:
```
python src/run.py <tensorrt-file>
```

When running Maru with TensorRT, you need to use the same compilation options for the TensorRT model and the execution options for `run.py`.
If you specify the `--fp16` or `--batch-size` options when compiling the TensorRT model, specify the same options in the `run.py` execution command as well.
The following example shows how to compile a TensorRT model with half-precision floating point (FP16) and batch size 16, and then launch Maru using that model (TensorRT model compilation only needs to be done once):
```
python src/compile.py --fp16 --batch-size 16 b4c128-645.model b4c128-645.rt.model
python src/run.py --fp16 --batch-size 16 b4c128-645.rt.model
```

## Running with Docker
### Running in CUDA-enabled Environments
A Docker image for running Maru is available, which makes it easy to use Maru (TensorRT is not supported).

To run the Docker version of Maru using GPU, you need to have NVIDIA drivers compatible with CUDA Version 12.6 or later and NVIDIA Container Toolkit installed.
If you can access the GPU from Docker by running the following command, you can run the Docker version of Maru using GPU:
```
docker run --rm -i --gpus all pytorch/pytorch:2.12.0-cuda12.6-cudnn9-runtime nvidia-smi
```

When running it for the first time, you need to download the Docker image.
The Docker image intended for use with CUDA, `takedarts/maru:v8.2-cuda12.6`, is about 4 GB in size, so the download may take some time.
I recommend downloading the Docker image in advance by running the following command:
```
docker pull takedarts/maru:v8.2-cuda12.6
```

You can run the Maru Docker image by executing the following command in an environment where CUDA is available (You can download the model file from [here](https://github.com/takedarts/maru/releases/tag/v8.2)):
```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.2-cuda12.6 /opt/run.sh <model_file>
```
Use the `--gpus` option to specify the GPUs to use, and mount the current directory to the container's `/workspace` using `-v .:/workspace`.
Place the model file in the current directory and specify its path as `<model_file>`.

You can also specify options after the execution command.
If you add `--help` to the execution command, a list of available options will be displayed:
```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.2-cuda12.6 /opt/run.sh --help
```

### Running on CPU
A Docker image intended for CPU (AMD64) execution, `takedarts/maru:v8.2-cpu`, is also available (its image size is smaller than the CUDA version).
If you want to run computations on the CPU, execute the following command:
```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.2-cpu /opt/run.sh <model_file>
```

If you want to run on an ARM64 CPU architecture, use the Docker image `takedarts/maru:v8.2-arm`:
```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.2-arm /opt/run.sh <model_file>
```

## Execution Options
When running the launch script `src/run.py`, you can specify the following options:

| Option                  | Description                                                    | Default Value         |
|-------------------------|----------------------------------------------------------------|-----------------------|
| `--help`                | Display a list of available options                            |                       |
| `--visits <N>`          | Number of searches (number of nodes in the search tree)        | 50                    |
| `--playouts <N>`        | Number of playouts (number of leaves in the search tree)       | 0                     |
| `--criterion <S>`       | Criterion for selecting moves (`value` or `visits`)            | `value`               |
| `--temperature <R>`     | Temperature parameter for search                               | 1.0                   |
| `--randomness <R>`      | Variation in search count during exploration (0.0-1.0)         | 0.0                   |
| `--rule <R>`            | Rule set for determining the winner (`ch`, `jp`, or `com`)     | `ch`                  |
| `--boardsize <N>`       | Board size (e.g., 9, 13, 19)                                   | 19                    |
| `--komi <K>`            | Komi value                                                     | 7.5                   |
| `--superko`             | Enable superko                                                 | False                 |
| `--pucb-constant-init`  | Initial value of PUCB constant term                            | 0.8                   |
| `--pucb-constant-base`  | Base value of PUCB constant term                               | 9000.0                |
| `--timelimit <N>`       | Maximum thinking time (in seconds)                             | 120                   |
| `--ponder`              | Enable pondering                                               | False                 |
| `--resign <R>`          | Predicted win rate threshold for resignation                   | 0.02                  |
| `--min-score <R>`       | Minimum predicted score difference for resignation             | 0.0                   |
| `--min-turn <N>`        | Minimum number of turns before resignation is allowed          | 100                   |
| `--initial-turn <N>`    | Number of opening turns with random moves                      | 4                     |
| `--client-name <S>`     | Client name to display                                         | `Maru`                |
| `--client-version <S>`  | Version information to display                                 | `8.2`                 |
| `--threads <N>`         | Number of threads to use for search                            | 16                    |
| `--display <S>`         | Command to display the board                                   |                       |
| `--sgf <S>`             | SGF file to load as the initial position                       |                       |
| `--batch-size <N>`      | Batch size for board evaluation                                | 32                    |
| `--gpus <N,...>`        | GPU ID(s) to use (comma-separated for multiple GPUs)           | All available GPUs    |
| `--fp16`                | Use half-precision floating point (FP16)                       | False                 |
| `--threads-per-gpu <N>` | Number of inference threads per GPU                            | 2                     |
| `--cache-size <N>`      | Cache size for inference results                               | max(visits, playouts) |
| `--verbose`             | Enable log output to standard error                            | False                 |

### Relationship between Visits, Playouts, and Timelimit
The termination condition of the search is determined by the values specified with the `--visits`, `--playouts`, and `--timelimit` options.
The search ends either when both the number of visits and the number of playouts exceed their specified values, or when the elapsed thinking time exceeds the specified number of seconds.

### Temperature Parameter
The `--temperature` option specifies the temperature parameter used to adjust the probability distribution output by the Policy Network. Increasing the temperature broadens the range of exploration, while decreasing it narrows the range.

### Rules for Determining the Winner
The `--rule` option can be set to `ch`, `jp`, or `com`. When `ch` is specified, the moves follow Chinese rules. When `jp` is specified, the moves assume Japanese rules. When `com` is specified, the moves basically follow Chinese rules but differ in that play continues until dead stones are removed; this setting is intended for games between computers, such as the CGOS server.

### Command to Display the Board
By specifying a display program such as gogui-display with the `--display` option, the board can be displayed. The display program receives the `play` command of the GTP.

### Visits and Playouts
In Maru, the number of visits is defined as "the number of nodes in the search tree," and the number of playouts is defined as "the number of leaves in the search tree."
The definition of the number of visits is the same as in other programs such as LeelaZero and KataGo.
On the other hand, the definition of the number of playouts differs from other programs.

In situations where the number of candidate moves is small, the number of leaves tends not to increase even when expanding nodes.
Therefore, when controlling the size of the search tree based on the number of playouts, a larger search tree tends to be created compared to when controlling based on the number of visits.
This is because in situations where a determined sequence of moves continues, it is less important to evaluate situations in the middle of the sequence, and it is more important to evaluate situations after the sequence breaks.
Compared to specifying the number of visits, specifying the same value for the number of playouts tends to result in longer search times.
However, because deeper search trees can be created, this may lead to improved playing strength.

Note that Maru always reuses the search tree. Therefore, even when running a search with the number of playouts specified, if the game progresses as expected, the search may finish in a short time.

## Execution Examples
To start Maru using the model file `b4c128-645.model`, run the following command:
```
python src/run.py b4c128-645.model
```

To start Maru with the number of visits set to 1000 and the maximum thinking time set to 5 seconds, run the following command:
```
python src/run.py b4c128-645.model --visits 1000 --timelimit 5
```

To start Maru assuming a game under Japanese rules, run the following command:
```
python src/run.py b4c128-645.model --rule jp
```

To play on a [CGOS server](http://yss-aya.com/cgos/), configure the client (e.g., [CGOS-Client](https://github.com/zakki/cgos)) with the following command:
```
python src/run.py b4c128-645.model --rule com
```

When using Maru with [Lizzie](https://github.com/featurecat/lizzie), set the following command as the GTP engine executed from Lizzie.
By setting the client name to `KataGo`, you can enable Lizzie's evaluation display feature.
```
python src/run.py b4c128-645.model --client-name KataGo
```

## Compatibility with Maru version 8.0/8.1
- Model files for Maru version 8.0 handle ladders differently, so they do not work correctly with Maru version 8.1.
- Model files for Maru version 8.1 do not support compilation to TensorRT models.

## License
Use of Maru's source code is permitted under the MIT License.
However, some of the cmake build scripts use scripts provided under the Apache License 2.0.
