# Engine port regression tests

Build the native module and run the Python tests from the repository root:

```sh
python src/build.py
PYTHONPATH=src python -m unittest discover -s src/tests -p '*_test.py' -v
MYPYPATH=src python -m mypy --explicit-package-bases src
```

The tests require NumPy and CPU PyTorch. Install `mypy` separately for type checks.
No trained model or GPU is required. `support.py` generates small TorchScript models
in temporary directories. Inference checks cover all 11,936 decoded input values.
Native search and GTP run in subprocesses with timeouts to expose hangs and crashes.
TensorRT tests mock the backend to check GPU selection and error handling;
they do not validate actual TensorRT compilation or inference.

Compile and run the standalone cache test after generating `Config.h` with the build:

```sh
c++ -std=c++20 -O2 -pthread -Isrc/deepgo/native/cpp \
  src/tests/native_cache_test.cpp \
  src/deepgo/native/cpp/{Board,BoardHash,BoardRen,Constant,Move,MoveResult,InferenceHash,InferenceCache,InferenceResult}.cpp \
  -o /tmp/maru_native_cache_test
/tmp/maru_native_cache_test
```

Use equivalent source arguments on shells without brace expansion.
The test checks LRU eviction, disabled caches, duplicate insertion, concurrent access,
and cache keys for stone placement, board size, turn, ko, komi, rule, and superko.

`board_test.py`, `record_test.py`, and the `records/` fixtures come from DeepGo.
Their comments and docstrings are translated into English. The board test uses an
independent NumPy input decoder instead of importing DeepGo's training model code.
The search worker adapts DeepGo's score, pass, maximum-visit, and PUCB regression cases.
The TensorRT argument test is adapted from Gokaku with Maru's input dimensions.

CUDA, MPS, FP16 on accelerators, actual TensorRT execution, and trained-model playing strength
require separate checks on suitable hardware.
