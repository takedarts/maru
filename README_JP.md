# Maru
[English](./README.md) | [Japanese](./README_JP.md)

## 概要
Maruはランダム着手の棋譜からの深層強化学習を用いて作成されたコンピュータ囲碁プログラムです。Maruの深層学習モデルは大きいカーネルのDepthwise ConvolutionやMulti-Head Attentionを含んでおり、盤面全体の状況を効率よく把握できる構造となっています。また、Gokakuの強化学習手順はKatagoやGumbel AlphaZeroを参考にして設計されており、効率よく多くのパターンを学習できるように設計されています。

Maruはコンピュータ将棋プログラムGokakuの兄弟プログラムです。Maruの深層学習モデル・探索アルゴリズム・強化学習手順はGokakuと同じ手法を用いています。

## ビルド方法
このプログラムの大部分はCythonとC++によって記述されているため、プログラムを動作させるためにはプログラムコードをコンパイルする必要があります。

最初に、コンパイルするために必要となるモジュールをインストールします。
```
pip install -r requirements.txt
```

次に、`src/build.py`を実行してCythonとC++のコードをコンパイルします。
```
python src/build.py
```

コンパイルに成功すると`src/deepgo/native`にコンパイルされたCythonモジュールが生成されます。

なお、以下のコマンドを実行すると生成されたファイルを削除します。
```
python src/build.py --clean
```

## 実行方法
起動スクリプト`src/run.py`を実行することでMaruを起動できます。
このとき、実行コマンドの引数としてモデルファイルを指定する必要があります。
```
python src/run.py <model_file>
```

Maruの操作はGTPプロトコルを介して行います。
以下に簡単な操作例を示します。
```
% python src/run.py b5n1c96.model
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
