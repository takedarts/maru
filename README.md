# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## Overview
Maru is a computer Go program developed using deep reinforcement learning from randomly generated game records. The deep learning model of Maru incorporates nested-bottleneck convolutions and multi-head attention, enabling it to efficiently grasp the overall state of the board. Its reinforcement learning procedure is inspired by approaches used in Katago and Gumbel AlphaZero, allowing it to efficiently learn a wide range of patterns.

Maru is a sibling program of the computer Shogi program [Gokaku](https://github.com/takedarts/gokaku). Maru shares the same deep learning model architecture, search algorithm, and reinforcement learning methodology as Gokaku.

You can check the improvement of Maru's playing strength through reinforcement learning on [this page](https://takeda-lab.jp/maru/).
The model files are available for download from [this release page](https://github.com/takedarts/maru/releases/tag/v8.1).

Maru version 8.1 will be the final release. From now on, only model file additions and bug fixes are planned.

## How to Run
Maru can be run using one of the following methods:

- [Running from Source Files](#running-from-source-files)
- [Running with Docker](#running-with-docker)

For Windows 11 (x64, CUDA 12) environments, you can also use the [executable file for Windows 11](https://drive.usercontent.google.com/download?id=1lMcRwy05qFr-MWkiOMvw1e4bHbWTtAWb).

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
python src/build.py
```

If compilation is successful, the compiled Cython module will be generated in `src/deepgo/native`.

You can delete the generated files by running the following command:
```
python src/build.py --clean
```

### Running the Program
You can launch Maru by running the launch script `src/run.py`.
At runtime, you need to specify the model file as a command-line argument (You can download the model file from [here](https://github.com/takedarts/maru/releases/tag/v8.1)):
```
python src/run.py <model_file>
```


Maru operates via the GTP (Go Text Protocol).
Here is a simple example of usage:
```
% python src/run.py b4c128-250.model
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
If you have CUDA installed on your system, you can retrieve and run the Maru Docker image using the following commands  (You can download the model file from [here](https://github.com/takedarts/maru/releases/tag/v8.1)):
```
docker pull takedarts/maru:v8.1-cuda12.6
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:v8.1-cuda12.6 /opt/run.sh <model_file>
```
Use the `--gpus` option to specify the GPUs to use, and mount the current directory to the container's `/workspace` using `-v .:/workspace`.
Place the model file in the current directory and specify its path as `<model_file>`.

To see the available options, use the `--help` flag:
```
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:v8.1-cuda12.6 /opt/run.sh --help
```

For environments without GPU (CPU only), use the following commands:
```
docker pull takedarts/maru:v8.1-cpu
docker run -it --rm -v .:/workspace takedarts/maru:v8.1-cpu /opt/run.sh <model_file>
```

## Execution Options
When running the startup script `src/run.py`, you can specify the following options:

| Option                 | Description                                                    | Default Value       |
|------------------------|----------------------------------------------------------------|---------------------|
| `--help`               | Display a list of available options                            |                     |
| `--visits <N>`         | Number of searches (number of nodes in the search tree)        | 50                  |
| `--playouts <N>`       | Number of playouts (number of leaves in the search tree)       | 0                   |
| `--search <S>`         | Criterion for selecting search nodes (`ucb1` or `pucb`)        | `pucb`              |
| `--temperature <R>`    | Temperature parameter for search                               | 1.0                 |
| `--randomness <R>`     | Variation in search count during exploration (0.0–1.0)         | 0.0                 |
| `--criterion <S>`      | Criterion for selecting a move (`lcb` or `visits`)             | `lcb`               |
| `--rule <R>`           | Rule set for determining the winner (`ch`, `jp`, or `com`)     | `ch`                |
| `--boardsize <N>`      | Board size (e.g., 9, 13, 19)                                   | 19                  |
| `--komi <K>`           | Komi value                                                     | 7.5                 |
| `--superko`            | Enable superko                                                 | False               |
| `--eval-leaf-only`     | Evaluate only leaf nodes during search                         | False               |
| `--timelimit <N>`      | Maximum thinking time (in seconds)                             | 120                 |
| `--ponder`             | Enable pondering                                               | False               |
| `--resign <R>`         | Predicted win rate threshold for resignation                   | 0.02                |
| `--min-score <R>`      | Minimum predicted score difference for resignation             | 0.0                 |
| `--min-turn <N>`       | Minimum number of turns before resignation is allowed          | 100                 |
| `--initial-turn <N>`   | Number of opening turns with random moves                      | 4                   |
| `--client-name <S>`    | Client name to display                                         | `Maru`              |
| `--client-version <S>` | Version information to display                                 | `8.1`               |
| `--threads <N>`        | Number of threads to use for search                            | 16                  |
| `--display <S>`        | Command to display the board                                   |                     |
| `--sgf <S>`            | SGF file to load as the initial position                       |                     |
| `--batch-size <N>`     | Maximum batch size for board evaluation                        | 2048                |
| `--gpu <N>`            | GPU ID(s) to use (comma-separated for multiple GPUs)           |                     |
| `--fp16`               | Use half-precision floating point (FP16)                       | False               |
| `--verbose`            | Enable log output to standard error                            | False               |

#### Relationship between Number of Visits, Number of Playouts, and Thinking Time
The termination condition of the search is determined by the values specified with the `--visits`, `--playouts`, and `--timelimit` options. The search ends either when both the number of visits and the number of playouts exceed their specified values, or when the elapsed thinking time exceeds the specified number of seconds.

#### Temperature Parameter
The `--temperature` option specifies the temperature parameter used to adjust the probability distribution output by the Policy Network. Increasing the temperature broadens the range of exploration, while decreasing it narrows the range.

#### Rules for Determining the Winner
The `--rule` option can be set to `ch`, `jp`, or `com`. When `ch` is specified, the moves follow Chinese rules. When `jp` is specified, the moves assume Japanese rules. When `com` is specified, the moves basically follow Chinese rules but differ in that play continues until dead stones are removed; this setting is intended for games between computers, such as the CGOS server.

#### Command to Display the Board
By specifying a display program such as gogui-display with the `--display` option, the board can be displayed. The display program receives the play command of the GTP protocol.

## Execution Examples
To start Maru using the model file `b4c128-250.model`, run the following command:
```
python src/run.py b4c128-250.model
```

To start Maru with the number of visits set to 1000 and the maximum thinking time set to 5 seconds, run the following command:
```
python src/run.py b4c128-250.model --visits 1000 --timelimit 5
```

To start Maru assuming a game under Japanese rules, run the following command:
```
python src/run.py b4c128-250.model --rule jp
```

To play on a [CGOS server](http://yss-aya.com/cgos/), configure the client (e.g., [CGOS-Client](https://github.com/zakki/cgos)) with the following command:
```
python src/run.py b4c128-250.model --rule com
```

When using Maru with [Lizzie](https://github.com/featurecat/lizzie), set the following command as the GTP engine executed from Lizzie.
By setting the client name to `KataGo`, you can enable Lizzie’s evaluation display feature.
```
python src/run.py b4c128-250.model --client-name KataGo
```

## Compatibility with Maru version 8.0
Although the model files of Maru version 8.1 and version 8.0 share the same input/output structure, they handle ladder situations differently. Therefore, using a version 8.0 model file with version 8.1 will not work correctly. If you wish to use a version 8.0 model file, please use Maru version 8.0.
