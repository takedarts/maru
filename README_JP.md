# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## 概要
Maruはランダム着手の棋譜からの深層強化学習で作成されたコンピュータ囲碁プログラムです。
深層学習モデルはBottleneck構造のMulti-Head Attentionを使用し、
強化学習手順はKataGoやGumbel AlphaZeroを参考にしています。

Maruはコンピュータ将棋プログラム[Gokaku](https://github.com/takedarts/gokaku)の兄弟プログラムです。
モデル構造・探索アルゴリズム・強化学習手順に同じ手法を使っています。
棋力向上の経過は[こちらのページ](https://takeda-lab.jp/maru/)で確認できます。

Maruを実行するためにはモデルファイルが必要です。
TorchScriptモデルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.3)からダウンロードできます。
TensorRTモデルは`src/compile.py`を使ってTorchScriptモデルから作成してください。

**注意** Maru version 8.3で、出力の形式を含めたモデルの仕様が変更されたため、Maru version 8.2以前のモデルは使用できません。

## 実行方法

- [ソースファイルからの実行](#ソースファイルからの実行)
- [Dockerを使用した実行](#dockerを使用した実行)

ソースファイルから実行する場合はネイティブ拡張のビルドが必要です。
TensorRTを使用する場合は、Torch-TensorRTをインストールした環境でソースからビルドしてください。

## ソースファイルからの実行

### ビルド方法
このプログラムの大部分はCythonとC++によって記述されています。
Maruを実行するためにはプログラムをビルドして、探索や盤面評価を実行するモジュールを作成する必要があります。

まず、ビルドと実行のために必要となるモジュールをインストールします。
```
pip install numpy cython cmake
```

上記のモジュールに加えて、ビルドと実行のためにPyTorchが必要となります。
環境にインストールされているCUDAのバージョンなどを確認し、実行環境に応じたPyTorchをインストールしてください。
ROCmがインストールされている環境でも、ROCm対応のPyTorchをインストールすることでMaruを実行できるはずです（ROCm環境での動作検証は行っていません・ROCm環境ではTensorRTを使用できません）。
```
pip install torch
```

TensorRTを使用してMaruを実行する場合は、Torch-TensorRTもインストールしてください。
Torch-TensorRTがインストールされていない場合は、TensorRTをサポートしないモジュールがビルドされます。
```
pip install torch-tensorrt
```

次に、`src/build.py`を実行してCythonとC++のコードをビルドします。
Linux環境やMacOS環境では`make`が必要となります。
Windows環境では`MSBuild`が必要となります（`MSBuild`はVisual Studioに含まれています）。
```
python src/build.py
```

コンパイルに成功すると`src/deepgo/native`にコンパイルされたCythonモジュールが生成されます。

なお、オプション`--clean`を指定して`src/build.py`を実行すると生成されたファイルを削除します。
```
python src/build.py --clean
```

### 実行方法
起動スクリプト`src/run.py`を実行することでMaruを起動できます。
引数にはTorchScriptモデルかTensorRTモデルを指定します（TorchScriptモデルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.3)からダウンロードできます。
TensorRTモデルは`src/compile.py`を使って作成してください）。
```
python src/run.py <model_file>
```

MaruはGTP（Go Text Protocol）を介して操作します。
以下に簡単な操作例を示します。
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

MaruはGTPに準拠しているため、[Gogui](https://github.com/Remi-Coulom/gogui)や[Lizzie](https://github.com/featurecat/lizzie)などのGTPに対応したGUIを使用することもできます。

起動スクリプトに`--help`オプションを指定すると、指定可能なオプションの一覧が表示されます。
```
python src/run.py --help
```

### TensorRTを使用した実行
Torch-TensorRTがインストールされた環境でビルドした場合、TensorRTを使用してMaruを実行できます。
まず、TensorRTを使用するために、Maruの推論モデルをTensorRTモデルへコンパイルする必要があります。
以下のコマンドを実行して、モデルファイルをTensorRTモデルへコンパイルしてください。
```
python src/compile.py <torch-script-file> <tensorrt-file>
```

`<torch-script-file>`にはTorchScriptモデルを指定してください。
上記のコマンドを実行すると、`<tensorrt-file>`にTensorRTモデルが生成されます。

作成されたTensorRTモデルを引数に指定して起動スクリプトを実行することで、TensorRTを使用してMaruを起動できます。
```
python src/run.py <tensorrt-file>
```

ただし、TensorRTを使用してMaruを実行する場合、TensorRTモデルのコンパイルオプションと`src/run.py`の実行オプションを同じものにする必要があります。
TensorRTモデルをコンパイルする際に`--fp16`や`--batch-size`オプションを指定した場合、`src/run.py`の実行コマンドにも同じオプションを指定してください。
以下はTensorRTモデルを半精度浮動小数点数（FP16）でバッチサイズ16に設定してコンパイルし、そのモデルを使用してMaruを起動する例です（TensorRTモデルのコンパイルは1度だけで行います）。
```
python src/compile.py --fp16 --batch-size 16 b16c256-645.model b16c256-645.rt.model
python src/run.py --fp16 --batch-size 16 b16c256-645.rt.model
```

## Dockerを使用した実行

### CUDAを使用できる環境での実行
Maruを実行できるDockerイメージが用意されており、これを使用することで簡単にMaruを実行できます（TensorRTはサポートしていません）。

GPUを使用してDocker版のMaruを実行するためには、Version 12.6以降のCUDAに対応したNVIDIAドライバとNVIDIA Container Toolkitがインストールされている必要があります。
以下のコマンドを実行してDockerからGPUにアクセスできるのであれば、GPUを使用してDocker版のMaruを実行できます。
```
docker run --rm -i --gpus all pytorch/pytorch:2.12.0-cuda12.6-cudnn9-runtime nvidia-smi
```

最初に実行する際にはDockerイメージをダウンロードする必要があります。
CUDAを使用することを想定したDockerイメージ`takedarts/maru:v8.3-cuda12.6`は、イメージサイズが約4GBとなっているため、ダウンロードに時間がかかる場合があります。
以下のコマンドを実行して、あらかじめDockerイメージをダウンロードしておくことをおすすめします。
```
docker pull takedarts/maru:v8.3-cuda12.6
```

CUDAを使用できる環境で以下のコマンドを実行することで、MaruのDockerイメージを実行できます。
```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.3-cuda12.6 /opt/run.sh <model_file>
```

オプション`--gpus`で使用するGPUを指定し、`-v .:/workspace`でカレントディレクトリをコンテナ内の`/workspace`にマウントします。
モデルファイルをカレントディレクトリ以下に置き、`<model_file>`にそのファイルパスを指定してください。

実行コマンドに続けてオプションを指定することもできます。 実行コマンドに--helpを指定すると、指定可能なオプションの一覧が表示されます。
```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.3-cuda12.6 /opt/run.sh --help
```

### CPUでの実行
CPU（AMD64）での実行を想定したDockerイメージ`takedarts/maru:v8.3-cpu`も用意されています（イメージサイズはCUDA版よりも小さくなっています）。
CPUで計算を実行する場合は以下のコマンドを実行してください。
```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.3-cpu /opt/run.sh <model_file>
```

ARM64アーキテクチャのCPUで実行する場合は`takedarts/maru:v8.3-arm`のDockerイメージを使用してください。
```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.3-arm /opt/run.sh <model_file>
```

## 実行オプション

| オプション | 説明 | デフォルト値 |
|---|---|---|
| `--help` | 利用可能なオプション一覧を表示 |  |
| `--visits <N>` | 探索の目標訪問数 | 50 |
| `--extends <N>` | 再探索回数の上限 | 0 |
| `--max-visits <N>` | 探索の最大訪問数 | 1,000,000 |
| `--criterion <S>` | 候補手の優先基準：`value`または`visits` | `value` |
| `--temperature <R>` | 探索の温度パラメータ | 1.0 |
| `--randomness <R>` | 目標訪問数の変動幅 | 0.0 |
| `--rule <R>` | 対局ルール：`ch`、`jp`、`com` | `ch` |
| `--boardsize <N>` | 盤面サイズ | 19 |
| `--komi <K>` | コミ | 7.5 |
| `--superko` | スーパーコウを有効にする | False |
| `--pucb-constant-init <R>` | PUCB定数の初期値 | 0.6 |
| `--pucb-constant-base <R>` | PUCB定数の基数 | 1600.0 |
| `--pucb-min-visits-rate <R>` | PUCBで優先する子ノードの最低訪問率 | 0.0 |
| `--timelimit <N>` | 思考時間の上限（秒） | 120 |
| `--ponder` | 相手の手番でも探索を継続する | False |
| `--resign <R>` | 投了する予想勝率の閾値 | 0.02 |
| `--min-score <R>` | 投了する最小目数差 | 0.0 |
| `--min-turn <N>` | 投了する最小ターン数 | 100 |
| `--initial-turn <N>` | ランダム着手する初期ターン数 | 4 |
| `--client-name <S>` | 表示するエンジン名 | `Maru` |
| `--client-version <S>` | 表示するバージョン | `8.3` |
| `--threads <N>` | 探索スレッド数 | 16 |
| `--display <S>` | GTP盤面表示コマンド |  |
| `--sgf <S>` | 初期局面として読み込むSGFファイル |  |
| `--batch-size <N>` | 推論バッチサイズ | 32 |
| `--gpus <N,...>` | GPUのID一覧。CPUは`-1` | 利用可能な全GPU、なければCPU |
| `--fp16` | FP16で推論する | False |
| `--threads-per-gpu <N>` | GPUごとの推論スレッド数 | 2 |
| `--cache-size <N>` | 推論キャッシュの最大件数 | visits |
| `--verbose` | 標準エラー出力へデバッグログを出力 | False |

### 訪問回数・思考時間
- 以下の条件のいずれかを満たした場合に探索は終了します。
  - 訪問数が`--visits`で指定された目標に達した場合
  - 訪問数が`--max-visits`で指定された最大訪問数に達した場合
  - `--timelimit`で指定された制限時間に達した場合
- 最も多く訪問された子ノードの訪問数が`--visits`で指定された目標の60%を超えた場合は、探索を早期に終了します。
- `--extends N`を指定すると、選択手の勝率が5%より大きく95%未満で、別候補の訪問数が選択手の2/3以上の場合に、最大N回再探索します。再探索ごとに目標訪問数を最初の指定値の半分ずつ増やします。制限時間が残り1秒未満となった場合は再探索を行いません。
- 探索木は着手後も再利用されます。

### ルールと温度パラメータ
- ルールに`ch`を指定すると、中国ルールで計算を行います。
- ルールに`jp`を指定すると、日本ルールで計算を行います。
- ルールに`com`を指定すると、中国ルールを基本として最後に死に石を打ち上げます。
- 探索の温度パラメータを大きくすると探索幅が広がり、小さくすると探索を絞ります。
- `--display`で指定した表示コマンドにはGTPの`play`を送信します。

## 実行例
モデルファイル`b16c256-645.model`を使用してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b16c256-645.model
```

訪問回数を1000回に設定し、思考時間の上限を5秒に設定してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b16c256-645.model --visits 1000 --timelimit 5
```

## テスト
ビルド後にCPUの盤面・推論・探索・GTP・SGFテストと型チェックを実行します。
```
pip install mypy
PYTHONPATH=src python -m unittest discover -s src/tests -p '*_test.py' -v
MYPYPATH=src python -m mypy --explicit-package-bases src
```

テストは小さなモデルを生成して使用するため、学習済みモデルやGPUは不要です。
詳しくは[src/tests/README.md](src/tests/README.md)を参照してください。

## ライセンス
MaruはMIT Licenseで提供しています。
一部のCMake補助スクリプトはApache License 2.0で提供されています。
