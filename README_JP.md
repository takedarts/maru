# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## 概要
Maruはランダム着手の棋譜からの深層強化学習を用いて作成されたコンピュータ囲碁プログラムです。Maruの深層学習モデルはNested-Bottleneck構造のConvolutionやMulti-Head Attentionを含んでおり、盤面全体の状況を効率よく把握できる構造となっています。また、Maruの強化学習手順はKatagoやGumbel AlphaZeroを参考にして設計されており、効率よく多くのパターンを学習できるように設計されています。

Maruはコンピュータ将棋プログラム[Gokaku](https://github.com/takedarts/gokaku)の兄弟プログラムです。Maruの深層学習モデル・探索アルゴリズム・強化学習手順はGokakuと同じ手法を用いています。

強化学習によるMaruの棋力向上を[こちらのページ](https://takeda-lab.jp/maru/)で確認できます。
また、モデルファイルは[こちらのリリースページ](https://github.com/takedarts/maru/releases/tag/v8.2)からダウンロードできます。

## 実行方法
Maruは以下のいずれかの方法で実行できます。
- [ソースファイルからの実行](#ソースファイルからの実行)
- [Dockerを使用した実行](#dockerを使用した実行)

バイナリファイルは提供していないため、ソースファイルからビルドして実行するか、Dockerイメージを使用して実行してください。
ただし、TensorRTを使用してMaruを実行する場合は、ソースファイルからビルドする必要があります。

## ソースファイルからの実行
### ビルド方法
このプログラムの大部分はCythonとC++によって記述されています。
Maruを実行するためにはプログラムをビルドして、探索や盤面評価を実行するモジュールを作成する必要があります。

まず、ビルドして実行するために必要となるモジュールをインストールします。
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

次に、`src/build.py`を実行してCythonとC++のコードをコンパイルします。
Linux環境やmacOS環境では`make`が必要となります。
Windows環境では`MSBuild`が必要となります（MSBuildはVisual Studioに含まれています）。
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
このとき、実行コマンドの引数としてモデルファイルを指定する必要があります（モデルファイルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.2)からダウンロードできます）。
```
python src/run.py <model_file>
```

Maruの操作はGTP（Go Text Protocol）を介して行います。
以下に簡単な操作例を示します。
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

`<torch-script-file>`には[こちら](https://github.com/takedarts/maru/releases/tag/v8.2)からダウンロードできるモデルファイルを指定してください。
上記のコマンドを実行すると、`<tensorrt-file>`にTensorRT形式のモデルファイルが生成されます。

作成されたTensorRTモデルファイルを引数に指定して起動スクリプトを実行することで、TensorRTを使用してMaruを起動できます。
```
python src/run.py <tensorrt-file>
```

ただし、TensorRTを使用してMaruを実行する場合、TensorRTモデルのコンパイルオプションと`run.py`の実行オプションを同じものにする必要があります。
TensorRTモデルをコンパイルする際に`--fp16`や`--batch-size`オプションを指定した場合、`run.py`の実行コマンドにも同じオプションを指定してください。
以下はTensorRTモデルを半精度浮動小数点数（FP16）でバッチサイズ16に設定してコンパイルし、そのモデルを使用してMaruを起動する例です（TensorRTモデルのコンパイルは1度だけで十分です）。
```
python src/compile.py --fp16 --batch-size 16 b4c128-645.model b4c128-645.rt.model
python src/run.py --fp16 --batch-size 16 b4c128-645.rt.model
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
CUDAを使用することを想定したDockerイメージ`takedarts/maru:v8.2-cuda12.6`は、イメージサイズが約4GBとなっているため、ダウンロードに時間がかかる場合があります。
以下のコマンドを実行して、あらかじめDockerイメージをダウンロードしておくことをおすすめします。
```
docker pull takedarts/maru:v8.2-cuda12.6
```

CUDAが使用できる環境で以下のコマンドを実行することで、MaruのDockerイメージを実行できます（モデルファイルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.2)からダウンロードできます）。
```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.2-cuda12.6 /opt/run.sh <model_file>
```
オプション`--gpus`で使用するGPUを指定し、`-v .:/workspace`でカレントディレクトリをコンテナ内の`/workspace`にマウントします。
モデルファイルをカレントディレクトリ以下に置き、`<model_file>`にそのファイルパスを指定してください。

実行コマンドに続けてオプションを指定することもできます。
実行コマンドに`--help`を指定すると、指定可能なオプションの一覧が表示されます。
```
docker run -iq --rm --gpus all -v .:/workspace takedarts/maru:v8.2-cuda12.6 /opt/run.sh --help
```

### CPUでの実行
CPU（AMD64）での実行を想定したDockerイメージ`takedarts/maru:v8.2-cpu`も用意されています（イメージサイズはCUDA版よりも小さくなっています）。
CPUで計算を実行する場合は以下のコマンドを実行してください。
```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.2-cpu /opt/run.sh <model_file>
```

ARM64アーキテクチャのCPUで実行する場合は`takedarts/maru:v8.2-arm`のDockerイメージを使用してください。
```
docker run -iq --rm -v .:/workspace takedarts/maru:v8.2-arm /opt/run.sh <model_file>
```

## 実行オプション
起動スクリプト`src/run.py`を実行する際に以下のオプションを指定できます。

| オプション              | 説明                                              | デフォルト値          |
|-------------------------|---------------------------------------------------|-----------------------|
| `--help`                | 利用可能なオプション一覧を表示                    |                       |
| `--visits <N>`          | 探索回数（探索木のノード数）                      | 50                    |
| `--playouts <N>`        | プレイアウト回数（探索木のリーフ数）              | 0                     |
| `--criterion <S>`       | 着手を選択する基準（`value`または`visits`）       | `value`               |
| `--temperature <R>`     | 検索時の温度パラメータ                            | 1.0                   |
| `--randomness <R>`      | 探索時の探索数の変動（0.0〜1.0）                  | 0.0                   |
| `--rule <R>`            | 勝敗決定ルール（`ch`、`jp`、`com`のいずれか）     | `ch`                  |
| `--boardsize <N>`       | 盤面サイズ（9, 13, 19など）                       | 19                    |
| `--komi <K>`            | コミの値                                          | 7.5                   |
| `--superko`             | スーパーコウを有効にする                          | False                 |
| `--pucb-constant-init`  | PUCBの定数項の初期値                              | 0.8                   |
| `--pucb-constant-base`  | PUCBの定数項の基数                                | 9000.0                |
| `--timelimit <N>`       | 思考時間の上限を指定（秒）                        | 120                   |
| `--ponder`              | 先読みを有効にする                                | False                 |
| `--resign <R>`          | 投了するときの予想勝率                            | 0.02                  |
| `--min-score <R>`       | 投了するときの最小予想目数差                      | 0.0                   |
| `--min-turn <N>`        | 投了するときの最小ターン数                        | 100                   |
| `--initial-turn <N>`    | ランダムに着手する初期ターン数                    | 4                     |
| `--client-name <S>`     | 表示するクライアント名                            | `Maru`                |
| `--client-version <S>`  | 表示するバージョン情報                            | `8.2`                 |
| `--threads <N>`         | 探索に使用するスレッド数                          | 16                    |
| `--display <S>`         | 盤面を表示するコマンド                            |                       |
| `--sgf <S>`             | 初期局面として読み込むSGFファイル                 |                       |
| `--batch-size <N>`      | 盤面評価のバッチサイズ                            | 32                    |
| `--gpus <N,...>`        | 使用するGPUのID（複数指定する場合はコンマ区切り） | 使用可能なすべてのGPU |
| `--fp16`                | 半精度浮動小数点数（FP16）を使用する              | False                 |
| `--threads-per-gpu <N>` | GPUごとに使用する推論スレッド数                   | 2                     |
| `--cache-size <N>`      | 推論結果のキャッシュサイズ                        | max(visits, playouts) |
| `--verbose`             | 標準エラー出力へのログ出力を有効にする            | False                 |

### 訪問回数・プレイアウト回数・思考時間の関係
`--visits`オプション・`--playouts`オプション・`--timelimit`オプションのそれぞれで指定された値によって探索の終了条件が決まります。
探索は「訪問回数とプレイアウト回数の両方が指定された回数を超えた場合」もしくは「思考時間が指定された秒数を超えた場合」のいずれかが満たされた時点で終了します。

### 温度パラメータ
`--temperature`オプションにはPolicyNetworkから出力される確率分布を調整するための温度パラメータを指定します。温度パラメータを大きくすると探索幅が広がり、小さくすると狭まります。

### 勝敗決定ルール
`--rule`オプションには`ch`、`jp`、`com`のいずれかを指定できます。`ch`を指定すると中国ルール、`jp`を指定すると日本ルールを想定した着手を行います。`com`を指定すると中国ルールを基本とした着手を行いますが、死に石を打ち上げるまで打ち続ける点が異なります（CGOSなどのコンピュータ同士の対局で使用されることを想定しています）。

### 盤面を表示するコマンド
`--display`オプションにgogui-displayなどの盤面を表示するプログラムを指定することで盤面を表示できます。表示用のプログラムにはGTPの`play`コマンドが送信されます。

### 訪問回数とプレイアウト回数
Maruでは、訪問回数を「探索木のノード数」、プレイアウト回数を「探索木のリーフ数」として定義しています。
訪問回数の定義は、LeelaZeroやKataGoなどの他のプログラムと同様です。
一方で、プレイアウト回数の定義は、他のプログラムとは異なります。

候補手の数が少ない局面では、ノードを展開したとしてもリーフ数が増加しにくくなります。
そのため、プレイアウト回数を基準に探索木の大きさを制御すると、訪問回数を基準とした場合よりも大きな探索木が作成されやすくなります。
このようにしているのは、決まった手順が続く局面では、その手順の途中にある局面を評価する重要性は低く、手順が途切れた後の局面を評価することが重要だと考えているためです。
訪問回数を指定した場合と比べると、同じ値のプレイアウト回数を指定した場合は探索時間が長くなる傾向があります。
一方で、より深い探索木を作成できるため、棋力の向上につながる可能性があります。

なお、Maruでは常に探索木を再利用します。そのため、プレイアウト回数を指定して探索を実行した場合でも、局面が予想どおりに進行したときは、短時間で探索が終了することがあります。

## 実行例
モデルファイル`b4c128-645.model`を使用してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b4c128-645.model
```

訪問回数を1000回に設定し、思考時間の上限を5秒に設定してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b4c128-645.model --visits 1000 --timelimit 5
```

日本ルールでの対局を想定してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b4c128-645.model --rule jp
```

[CGOSサーバ](http://yss-aya.com/cgos/)で対局する場合、[CGOS-Client](https://github.com/zakki/cgos)などのクライアントに対して以下のコマンドを設定します。
```
python src/run.py b4c128-645.model --rule com
```

[Lizzie](https://github.com/featurecat/lizzie)でMaruを使用する場合、Lizzieから実行されるGTPエンジンとして以下のコマンドを設定します。
クライアント名を`KataGo`に設定することでLizzieの評価表示機能を使用できます。
```
python src/run.py b4c128-645.model --client-name KataGo
```

## Maru version 8.0/8.1との互換性
- Maru version 8.0のモデルファイルはシチョウの扱いが異なるため、Maru version 8.1では正しく動作しません。
- Maru version 8.1のモデルファイルはTensorRTモデルへのコンパイルをサポートしていません。

## ライセンス
Maruのソースコードの利用はMIT Licenseのもとで許可されています。
ただし、cmakeのビルドスクリプトの一部はApache License 2.0で提供されているスクリプトを使用しています。
