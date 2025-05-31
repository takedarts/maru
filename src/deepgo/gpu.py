import logging
from typing import List, Sequence, Tuple

import torch

LOGGER = logging.getLogger(__name__)


def get_default_gpus(
    gpus: Sequence[int] | None,
    fp16: bool,
) -> Tuple[List[int], bool]:
    '''実行環境に適切なGPUのIDのリストとFP16の利用可否を返す。
    gpusがNoneの場合は利用可能なGPUのIDのリストを返す。
    gpusが指定されている場合はそのリストを返す。
    ただし、gpusに-1が含まれている場合はFP16の利用を無効化する。
    無効なGPUのIDが含まれている場合は警告を表示して無視する。
    Args:
        gpus (Sequence[int] | None): GPUのIDのリスト
        fp16 (bool): FP16の利用可否
    Returns:
        Tuple[List[int], bool]: GPUのIDのリストとFP16の利用可否
    '''
    # CUDAが利用可能の場合
    if torch.cuda.is_available():
        # GPUのIDのリストが指定されていない場合は利用可能なGPUのIDのリストを設定する
        if gpus is None:
            new_gpus = list(range(torch.cuda.device_count()))
        # そうでない場合は使用不可能なGPUのIDを除外する
        else:
            new_gpus = [gpu for gpu in gpus if gpu < torch.cuda.device_count()]
            # 使用不可能なGPUのIDが含まれている場合は警告を表示する
            if len(new_gpus) != len(gpus):
                LOGGER.warning(
                    'Invalid GPU ID is ignored: %s',
                    [gpu for gpu in gpus if gpu not in new_gpus])
    # MPSが利用可能の場合
    elif torch.backends.mps.is_available():
        # GPUのIDのリストが指定されていない場合は[0]を設定する
        if gpus is None:
            new_gpus = [0]
        # そうでない場合は0以外のGPUのIDを除外する
        else:
            new_gpus = [gpu for gpu in gpus if gpu == 0]
            # 0以外のGPUのIDが含まれている場合は警告を表示する
            if len(new_gpus) != len(gpus):
                LOGGER.warning(
                    'Invalid GPU ID is ignored: %s',
                    [gpu for gpu in gpus if gpu not in new_gpus])
    # どちらも利用不可能の場合
    else:
        # GPUのIDのリストが指定されていない場合は[-1]を設定する
        if gpus is None:
            new_gpus = [-1]
        # そうでない場合は[-1]以外のGPUのIDを除外する
        else:
            new_gpus = [gpu for gpu in gpus if gpu == -1]
            # -1以外のGPUのIDが含まれている場合は警告を表示する
            if len(new_gpus) != len(gpus):
                LOGGER.warning(
                    'Invalid GPU ID is ignored: %s',
                    [gpu for gpu in gpus if gpu not in new_gpus])

    # 作成したGPUのIDのリストが空の場合は[-1]を設定する
    if len(new_gpus) == 0:
        new_gpus = [-1]

    # GPUのIDのリストに-1が含まれている場合はFP16の利用を無効化する
    if -1 in new_gpus:
        new_fp16 = False
    # そうでない場合はFP16の利用可否を設定する
    else:
        new_fp16 = fp16

    return new_gpus, new_fp16
