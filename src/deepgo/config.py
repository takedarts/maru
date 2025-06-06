################################################################
# 全体の設定
################################################################
# プログラム名
NAME = 'Maru'
# バージョン番号
VERSION = '8.0'

################################################################
# 碁盤の設定
################################################################
# 黒石の値(変更不可)
BLACK = 1
# 白石の値(変更不可)
WHITE = -1
# 空白の値(変更不可)
EMPTY = 0
# 盤面の端の値(変更不可)
EDGE = 9
# パスを意味する座標(変更不可)
PASS = (-1, -1)

################################################################
# ルールの設定値
################################################################
# 中国ルール
RULE_CH = 0
# 日本ルール
RULE_JP = 1
# コンピュータ対戦ルール
RULE_COM = 2

################################################################
# モデルの設定値
################################################################
# モデルに入力する盤面特徴量の大きさ(盤面の大きさ)
MODEL_SIZE = 19
# モデルに入力する盤面特徴量の数
MODEL_FEATURES = 32
# モデルに入力するゲーム特徴量の数
MODEL_INFOS = 7
# モデルが出力する盤面予測値の数
MODEL_PREDICTIONS = 6
# モデルが出力するゲーム予測値の数
MODEL_VALUES = 3

# モデルに入力するデータの大きさ
MODEL_INPUT_SIZE = (MODEL_FEATURES + 1) * MODEL_SIZE * MODEL_SIZE + MODEL_INFOS
# モデルが出力するデータの大きさ
MODEL_OUTPUT_SIZE = MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE + MODEL_VALUES

################################################################
# デフォルト設定
################################################################
# デフォルトの碁盤の大きさ
DEFAULT_SIZE = 19
# デフォルトのコミの数
DEFAULT_KOMI = 7.5

################################################################
# ロギングの設定
################################################################
# ログの形式
LOGGING_FORMAT = '%(asctime)s [%(levelname)-5.5s] %(message)s (%(module)s.%(funcName)s:%(lineno)s)'
# 時刻表示の形式
LOGGING_DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
