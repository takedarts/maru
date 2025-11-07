################################################################
# Global settings
################################################################
# Program name
NAME = 'Maru'
# Version number
VERSION = '8.1'

################################################################
# Board settings
################################################################
# Value for black stone (do not change)
BLACK = 1
# Value for white stone (do not change)
WHITE = -1
# Value for empty (do not change)
EMPTY = 0
# Value for board edge (do not change)
EDGE = 9
# Coordinates meaning pass (do not change)
PASS = (-1, -1)

################################################################
# Rule settings
################################################################
# Chinese rule
RULE_CH = 0
# Japanese rule
RULE_JP = 1
# Computer match rule
RULE_COM = 2

################################################################
# Model settings
################################################################
# Board feature size for model input (board size)
MODEL_SIZE = 19
# Number of board features for model input
MODEL_FEATURES = 32
# Number of game features for model input
MODEL_INFOS = 7
# Number of board predictions output by model
MODEL_PREDICTIONS = 6
# Number of game predictions output by model
MODEL_VALUES = 3

# Size of data input to model
MODEL_INPUT_SIZE = (MODEL_FEATURES + 1) * MODEL_SIZE * MODEL_SIZE + MODEL_INFOS
# Size of data output by model
MODEL_OUTPUT_SIZE = MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE + MODEL_VALUES

################################################################
# Default settings
################################################################
# Default board size
DEFAULT_SIZE = 19
# Default komi value
DEFAULT_KOMI = 7.5

################################################################
# Logging settings
################################################################
# Log format
LOGGING_FORMAT = '%(asctime)s [%(levelname)-5.5s] %(message)s (%(module)s.%(funcName)s:%(lineno)s)'
# Date format for log timestamps
LOGGING_DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
