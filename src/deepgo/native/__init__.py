# If the Torch module is not imported, the LibTorch library may not be found,
# which can cause the import of native modules that require LibTorch to fail.
# Therefore, we first try to import native modules without importing the Torch module,
# and if that fails, we import the Torch module and then try to import native modules again.
try:
    from .modules import NativeBoard  # type: ignore # noqa
    from .modules import NativeInferenceModel  # type: ignore # noqa
    from .modules import NativeInferenceProcessor  # type: ignore # noqa
    from .modules import NativePlayer  # type: ignore  # noqa
except ImportError:
    import torch  # noqa
    from .modules import NativeBoard  # type: ignore # noqa
    from .modules import NativeInferenceModel  # type: ignore # noqa
    from .modules import NativeInferenceProcessor  # type: ignore # noqa
    from .modules import NativePlayer  # type: ignore  # noqa
