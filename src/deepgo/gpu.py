import logging
from typing import List, Sequence, Tuple

from deepgo.native import NativeInferenceModel

LOGGER = logging.getLogger(__name__)


def get_default_gpus(
    gpus: Sequence[int] | None,
    fp16: bool,
) -> Tuple[List[int], bool]:
    '''Return a list of appropriate GPU IDs and FP16 availability for the execution environment.
    If gpus is None, return a list of available GPU IDs.
    If gpus is specified, return that list.
    If -1 is included in gpus, FP16 usage is disabled.
    If invalid GPU IDs are included, a warning is displayed and they are ignored.
    Args:
        gpus (Sequence[int] | None): List of GPU IDs
        fp16 (bool): Whether to use FP16
    Returns:
        Tuple[List[int], bool]: List of GPU IDs and FP16 availability
    '''
    # Get list of available GPU IDs
    available_gpus = NativeInferenceModel.get_available_gpus()

    # If GPU ID list is not specified, set the list of available GPU IDs
    if gpus is None:
        # If GPU IDs are available, exclude CPU
        if max(available_gpus, default=-1) >= 0:
            new_gpus = [gpu for gpu in available_gpus if gpu >= 0]
        else:
            new_gpus = available_gpus
    # Otherwise, exclude unavailable GPU IDs
    else:
        new_gpus = [gpu for gpu in gpus if gpu in available_gpus]
        # If unavailable GPU IDs are included, display a warning
        if len(new_gpus) != len(gpus):
            LOGGER.warning(
                'Invalid GPU ID is ignored: %s',
                [gpu for gpu in gpus if gpu not in new_gpus])

    # If -1 is included in the GPU ID list, disable FP16 usage
    if -1 in new_gpus:
        new_fp16 = False
    # Otherwise, use the specified FP16 setting
    else:
        new_fp16 = fp16

    return new_gpus, new_fp16
