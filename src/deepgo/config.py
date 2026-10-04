################################################################
# Global settings
################################################################
# Program name
NAME = 'Maru'
# Version number
VERSION = '8.3'

################################################################
# Board settings
################################################################
# Value for black stone (do not change)
COLOR_BLACK = 1
# Value for white stone (do not change)
COLOR_WHITE = -1
# Value for empty (do not change)
COLOR_EMPTY = 0
# Value for board edge (do not change)
COLOR_EDGE = 9
# Coordinates meaning pass (do not change)
MOVE_PASS = (-1, -1)

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
MODEL_BOARD_SIZE = 19

# Number of board feature planes input to the model
MODEL_FEATURE_NUM = 32
# Number of game features input to the model
MODEL_INFO_NUM = 23
# Number of policy outputs per move
MODEL_POLICY_NUM = 2
# Number of territory output planes
MODEL_TERRITORY_NUM = 3
# Number of game value outputs
MODEL_VALUE_NUM = 3

# Scale for converting board size and komi to fixed-point values
MODEL_VALUE_SCALE = 0xffff

# Size of the board features input to the model
MODEL_FEATURE_SIZE = MODEL_FEATURE_NUM * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE
# Size of the model input mask
MODEL_MASK_SIZE = MODEL_BOARD_SIZE * MODEL_BOARD_SIZE
# Size of the game features input to the model
MODEL_INFO_SIZE = MODEL_INFO_NUM
# Size of the model policy output
MODEL_POLICY_SIZE = MODEL_POLICY_NUM * (MODEL_BOARD_SIZE * MODEL_BOARD_SIZE + 1)
# Size of the model territory output
MODEL_TERRITORY_SIZE = MODEL_TERRITORY_NUM * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE
# Size of the model game value output
MODEL_VALUE_SIZE = MODEL_VALUE_NUM

# Offset of the mask in model inputs
MODEL_MASK_OFFSET = MODEL_FEATURE_SIZE
# Offset of game information in model inputs
MODEL_INFO_OFFSET = MODEL_MASK_OFFSET + MODEL_MASK_SIZE
# Size of the model input
MODEL_INPUT_SIZE = MODEL_INFO_OFFSET + MODEL_INFO_SIZE
# Size of the model input packed into int32 words
# Represent one-hot board features and the mask with one bit per value
# Represent the real-valued komi and board size with 32 bits each
MODEL_INPUT_PACK_SIZE = (MODEL_INPUT_SIZE + 31) // 32 + 2

# Offset of predicted territories in model outputs
MODEL_TERRITORY_OFFSET = MODEL_POLICY_SIZE
# Offset of predicted game values in model outputs
MODEL_VALUE_OFFSET = MODEL_TERRITORY_OFFSET + MODEL_TERRITORY_SIZE
# Size of the model output
MODEL_OUTPUT_SIZE = MODEL_VALUE_OFFSET + MODEL_VALUE_SIZE

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
# Default minimum child visit ratio prioritized by PUCB
DEFAULT_PUCB_MIN_VISITS_RATE = 0.0
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
