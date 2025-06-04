# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## 概要
Maruはランダム着手の棋譜からの深層強化学習を用いて作成されたコンピュータ囲碁プログラムです。Maruの深層学習モデルは大きいカーネルのDepthwise ConvolutionやMulti-Head Attentionを含んでおり、盤面全体の状況を効率よく把握できる構造となっています。また、Gokakuの強化学習手順はKatagoやGumbel AlphaZeroを参考にして設計されており、効率よく多くのパターンを学習できるように設計されています。

Maruはコンピュータ将棋プログラムGokakuの兄弟プログラムです。Maruの深層学習モデル・探索アルゴリズム・強化学習手順はGokakuと同じ手法を用いています。

強化学習によるMaruの棋力向上を[こちらのページ](https://takeda-lab.jp/maru/)で確認できます。
また、モデルファイルは[こちらのリリースページ](https://github.com/takedarts/maru/releases/tag/v8.0)からダウンロードできます。

## 実行方法
Maruは以下のいずれかの方法で実行できます。
- [ソースファイルからの実行](#ソースファイルからの実行)
- [Dockerを使用した実行](#dockerを使用した実行)

## ソースファイルからの実行
### ビルド方法
このプログラムの大部分はCythonとC++によって記述されているため、プログラムを動作させるためにはプログラムコードをコンパイルする必要があります。

まず、コンパイルするために必要となるモジュールをインストールします。
```
pip install numpy cython cmake
```

コンパイルするためにPyTorchが必要となりますが、このモジュールはcudaなどの実行環境に応じたものをインストールしてください。
```
pip install torch
```

次に、`src/build.py`を実行してCythonとC++のコードをコンパイルします。
Linux環境やMacOS環境では`make`が必要となります。
Windows環境では`MSBuild`が必要となります（MSBuildはVisual Studioに含まれています）。
```
python -X utf8 src/build.py
```

コンパイルに成功すると`src/deepgo/native`にコンパイルされたCythonモジュールが生成されます。

なお、以下のコマンドを実行すると生成されたファイルを削除します。
```
python src/build.py --clean
```

### 実行方法
起動スクリプト`src/run.py`を実行することでMaruを起動できます。
このとき、実行コマンドの引数としてモデルファイルを指定する必要があります（モデルファイルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.0)からダウンロードできます）。
```
python src/run.py <model_file>
```

Maruの操作はGTPプロトコルを介して行います。
以下に簡単な操作例を示します。
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

MaruはGTPプロトコルに準拠しているため、[Gogui](https://github.com/Remi-Coulom/gogui)や[Lizzie](https://github.com/featurecat/lizzie)などのGTPプロトコルに対応したGUIを使用することもできます。

起動スクリプトに`--help`オプションを指定すると、指定可能なオプションの一覧が表示されます。
```
python src/run.py --help
```

## Dockerを使用した実行
Maruを実行できるDockerイメージが用意されており、これを使用することで簡単にMaruを実行できます。
CUDAがインストールされている環境で以下のコマンドを実行することで、MaruのDockerイメージを取得して実行できます（モデルファイルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.0)からダウンロードできます）。
```
docker pull takedarts/maru:cuda
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:cuda /opt/run.sh <model_file>
```
オプション`--gpus`で使用するGPUを指定し、`-v .:/workspace`でカレントディレクトリをコンテナ内の`/workspace`にマウントします。
モデルファイルをカレントディレクトリ以下に置き、`<model_file>`にそのファイルパスを指定してください。

実行コマンドに`--help`を指定すると、指定可能なオプションの一覧が表示されます。
```
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:cuda /opt/run.sh --help
```

CPUのみの環境では以下のコマンドを実行してください。
```
docker pull takedarts/maru:cpu
docker run -it --rm -v .:/workspace takedarts/maru:cpu /opt/run.sh <model_file>
```
