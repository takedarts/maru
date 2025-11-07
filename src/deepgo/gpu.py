import logging
from typing import List, Sequence, Tuple

import torch

LOGGER = logging.getLogger(__name__)


def get_default_gpus(
    gpus: Sequence[int] | None,
    fp16: bool,
) -> Tuple[List[int], bool]:
    '''Return a list of appropriate GPU IDs and FP16 availability for the execution environment.
    If gpus is None, returns a list of available GPU IDs.
    If gpus is specified, returns that list.
    If -1 is included in gpus, disables FP16 usage.
    If invalid GPU IDs are included, shows a warning and ignores them.
    Args:
        gpus (Sequence[int] | None): List of GPU IDs
        fp16 (bool): Whether to use FP16
    Returns:
        Tuple[List[int], bool]: List of GPU IDs and FP16 availability
    '''
    # If CUDA is available
    if torch.cuda.is_available():
        # If the list of GPU IDs is not specified, set the list of available GPU IDs
        if gpus is None:
            new_gpus = list(range(torch.cuda.device_count()))
        # Otherwise, exclude unavailable GPU IDs
        else:
            new_gpus = [gpu for gpu in gpus if gpu < torch.cuda.device_count()]
            # If unavailable GPU IDs are included, show a warning
            if len(new_gpus) != len(gpus):
                LOGGER.warning(
                    'Invalid GPU ID is ignored: %s',
                    [gpu for gpu in gpus if gpu not in new_gpus])
    # If MPS is available
    elif torch.backends.mps.is_available():
        # If the list of GPU IDs is not specified, set [0]
        if gpus is None:
            new_gpus = [0]
        # Otherwise, exclude GPU IDs other than 0
        else:
            new_gpus = [gpu for gpu in gpus if gpu == 0]
            # If GPU IDs other than 0 are included, show a warning
            if len(new_gpus) != len(gpus):
                LOGGER.warning(
                    'Invalid GPU ID is ignored: %s',
                    [gpu for gpu in gpus if gpu not in new_gpus])
    # If neither is available
    else:
        # If the list of GPU IDs is not specified, set [-1]
        if gpus is None:
            new_gpus = [-1]
        # Otherwise, exclude GPU IDs other than -1
        else:
            new_gpus = [gpu for gpu in gpus if gpu == -1]
            # If GPU IDs other than -1 are included, show a warning
            if len(new_gpus) != len(gpus):
                LOGGER.warning(
                    'Invalid GPU ID is ignored: %s',
                    [gpu for gpu in gpus if gpu not in new_gpus])

    # If the created list of GPU IDs is empty, set [-1]
    if len(new_gpus) == 0:
        new_gpus = [-1]

    # If -1 is included in the list of GPU IDs, disable FP16 usage
    if -1 in new_gpus:
        new_fp16 = False
    # Otherwise, set FP16 availability
    else:
        new_fp16 = fp16

    return new_gpus, new_fp16
