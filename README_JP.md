# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## 概要
Maruはランダム着手の棋譜からの深層強化学習を用いて作成されたコンピュータ囲碁プログラムです。Maruの深層学習モデルは大きいカーネルのDepthwise ConvolutionやMulti-Head Attentionを含んでおり、盤面全体の状況を効率よく把握できる構造となっています。また、Gokakuの強化学習手順はKatagoやGumbel AlphaZeroを参考にして設計されており、効率よく多くのパターンを学習できるように設計されています。

Maruはコンピュータ将棋プログラムGokakuの兄弟プログラムです。Maruの深層学習モデル・探索アルゴリズム・強化学習手順はGokakuと同じ手法を用いています。

強化学習によるMaruの棋力向上を[こちらのページ](https://takeda-lab.jp/maru/)で確認できます。
また、モデルファイルは[こちらのリリースページ](https://github.com/takedarts/maru/releases/tag/v8.1)からダウンロードできます。

Maru version 8.1は最終版となります。今後はモデルファイル追加とバグ修正のみを予定しています。

## 実行方法
Maruは以下のいずれかの方法で実行できます。
- [ソースファイルからの実行](#ソースファイルからの実行)
- [Dockerを使用した実行](#dockerを使用した実行)

Windows11（x64, Cuda12）環境の場合は[Windows11用の実行ファイル](https://drive.usercontent.google.com/download?id=1lMcRwy05qFr-MWkiOMvw1e4bHbWTtAWb)を使用することもできます。

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
python src/build.py
```

コンパイルに成功すると`src/deepgo/native`にコンパイルされたCythonモジュールが生成されます。

なお、以下のコマンドを実行すると生成されたファイルを削除します。
```
python src/build.py --clean
```

### 実行方法
起動スクリプト`src/run.py`を実行することでMaruを起動できます。
このとき、実行コマンドの引数としてモデルファイルを指定する必要があります（モデルファイルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.1)からダウンロードできます）。
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
CUDAがインストールされている環境で以下のコマンドを実行することで、MaruのDockerイメージを取得して実行できます（モデルファイルは[こちら](https://github.com/takedarts/maru/releases/tag/v8.1)からダウンロードできます）。
```
docker pull takedarts/maru:v8.1-cuda12.6
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:v8.1-cuda12.6 /opt/run.sh <model_file>
```
オプション`--gpus`で使用するGPUを指定し、`-v .:/workspace`でカレントディレクトリをコンテナ内の`/workspace`にマウントします。
モデルファイルをカレントディレクトリ以下に置き、`<model_file>`にそのファイルパスを指定してください。

実行コマンドに`--help`を指定すると、指定可能なオプションの一覧が表示されます。
```
docker run -it --rm --gpus all -v .:/workspace takedarts/maru:v8.1-cuda12.6 /opt/run.sh --help
```

CPUのみの環境では以下のコマンドを実行してください。
```
docker pull takedarts/maru:v8.1-cpu
docker run -it --rm -v .:/workspace takedarts/maru:v8.1-cpu /opt/run.sh <model_file>
```

## 実行オプション
起動スクリプト`src/run.py`を実行する際に以下のオプションを指定できます。

| オプション             | 説明                                              | デフォルト値        |
|------------------------|---------------------------------------------------|---------------------|
| `--help`               | 利用可能なオプション一覧を表示                    |                     |
| `--visits <N>`         | 探索回数（探索木のノード数）                      | 50                  |
| `--playouts <N>`       | プレイアウト回数（探索木のリーフ数）              | 0                   |
| `--search <S>`         | 探索手法を指定（`ucb1`または`pucb`）              | `pucb`              |
| `--temperature <R>`    | 検索時の温度パラメータ                            | 1.0                 |
| `--randomness <R>`     | 探索時の探索数の変動（0.0〜1.0）                  | 0.0                 |
| `--criterion <S>`      | 候補手から着手を決める基準（`lcb`または`visits`） | `lcb`               |
| `--rule <R>`           | 勝敗決定ルール（`ch`、`jp`、`com`のいずれか）     | `ch`                |
| `--boardsize <N>`      | 盤面サイズ（9, 13, 19など）                       | 19                  |
| `--komi <K>`           | コミの値                                          | 7.5                 |
| `--superko`            | スーパーコウを有効にする                          | False               |
| `--eval-leaf-only`     | 探索時のリーフノードのみを評価対象とする          | False               |
| `--timelimit <N>`      | 思考時間の上限を指定（秒）                       | 10                  |
| `--ponder`             | 先読みを有効にする                                | False               |
| `--resign <R>`         | 投了するときの予想勝率                            | 0.02                |
| `--min-score <R>`      | 投了するときの最小予想目数差                      | 0.0                 |
| `--min-turn <N>`       | 投了するときの最小ターン数                        | 100                 |
| `--initial-turn <N>`   | ランダムに着手する初期ターン数                    | 4                   |
| `--client-name <S>`    | 表示するクライアント名                            | `Maru`              |
| `--client-version <S>` | 表示するバージョン情報                            | `8.1`               |
| `--threads <N>`        | 探索に使用するスレッド数                          | 16                  |
| `--display <S>`        | 盤面を表示するコマンド                            | None                |
| `--sgf <S>`            | 初期局面として読み込むSGFファイル                 | None                |
| `--batch-size <N>`     | 盤面評価のバッチサイズの最大値                    | 1                   |
| `--gpu <N>`            | 使用するGPUのID（複数指定する場合はコンマ区切り） | 0                   |
| `--fp16`               | 半精度浮動小数点数（FP16）を使用する              | False               |
| `--verbose`            | 標準エラー出力へのログ出力を有効にする            | False               |

#### 訪問回数・プレイアウト回数・思考時間の関係
`--visits`オプション・`--playouts`オプション・`--timelimit`オプションのそれぞれで指定された値によって探索の終了条件が決まります。探索は「訪問回数とプレイアウト回数の両方が指定された回数を超えた場合」もしくは「思考時間が指定された秒数を超えた場合」のいずれかが満たされた時点で終了します。

#### 温度パラメータ
`--temperature`オプションにはPolicyNetworkにから出力される確率分布を調整するための温度パラメータを指定します。温度パラメータを大きくすると探索幅が広がり、小さくすると狭まります。

#### 勝敗決定ルール
`--rule`オプションには`ch`、`jp`、`com`のいずれかを指定できます。`ch`を指定すると中国ルール、`jp`を指定すると日本ルールを想定した着手を行います。`com`を指定すると中国ルールを基本とした着手を行いますが、死に石を打ち上げるまで打ち続ける点が異なります（CGOSなどのコンピュータ同士の対局で使用されることを想定しています）。

#### 盤面を表示するコマンド
`--display`オプションにgogui-displayなどの盤面を表示するプログラムを指定することで盤面を表示できます。表示用のプログラムにはGTPプロトコルの`play`コマンドが送信されます。

## 実行例
モデルファイル`b4c128-250.model`を使用してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b4c128-250.model
```

訪問回数を1000回に設定し、思考時間の上限を5秒に設定してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b4c128-250.model --visits 1000 --timelimit 5
```

日本ルールでの対局を想定してMaruを起動する場合、以下のコマンドを実行します。
```
python src/run.py b4c128-250.model --rule jp
```

[CGOSサーバ](http://yss-aya.com/cgos/)で対局する場合、[CGOS-Client](https://github.com/zakki/cgos)などのクライアントに対して以下のコマンドを設定します。
```
python src/run.py b4c128-250.model --rule com
```

[Lizzie](https://github.com/featurecat/lizzie)でMaruを使用する場合、Lizzieから実行されるGTPエンジンとして以下のコマンドを設定します。
クライアント名を`KataGo`に設定することでLizzieの評価表示機能を使用できます。
```
python src/run.py b4c128-250.model --client-name KataGo
```

## Maru version 8.0との互換性
Maru version 8.1とversion 8.0のモデルファイルは同じ入出力構造を持っていますが、シチョウの扱いが異なるため、version 8.0のモデルファイルをversion 8.1で使用すると正しく動作しません。version 8.0のモデルファイルを使用する場合はMaru version 8.0を使用してください。
