################################################################
# Global settings
################################################################
# Program name
NAME = 'Maru'
# Version number
VERSION = '8.2'

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
# Size of input data to the model when embedded as int32
MODEL_INPUT_PACK_SIZE = (MODEL_INPUT_SIZE + 31) // 32 + 1
# Size of data output by model
MODEL_OUTPUT_SIZE = MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE + MODEL_VALUES
# Size of model output mask when embedded as int32
MODEL_OUTPUT_PACK_SIZE = (MODEL_OUTPUT_SIZE + 31) // 32

################################################################
# Default settings
################################################################
# Default board size
DEFAULT_SIZE = 19
# Default komi value
DEFAULT_KOMI = 7.5
# Default maximum visits for MCTS
DEFAULT_MAX_VISITS = 1_000_000
# Initial value applied to PUCB upper confidence bound
DEFAULT_PUCB_CONSTANT_INIT = 0.6
# Base value applied to PUCB upper confidence bound
DEFAULT_PUCB_CONSTANT_BASE = 1600.0
# Default number of threads per GPU
DEFAULT_THREADS_PER_GPU = 2
# Default batch size for board evaluation calculation
DEFAULT_BATCH_SIZE = 32

################################################################
# Logging settings
################################################################
# Log format
LOGGING_FORMAT = '%(asctime)s [%(levelname)-5.5s] %(message)s (%(module)s.%(funcName)s:%(lineno)s)'
# Date format for log timestamps
LOGGING_DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
