'''Check TensorRT compilation plumbing without requiring a CUDA runtime.'''

import importlib
import unittest
from unittest.mock import Mock, patch

import torch

compiler = importlib.import_module('compile')


class CompileTest(unittest.TestCase):
    '''Verify device selection and compilation error handling.'''

    def test_device_and_save(self) -> None:
        '''Forward the selected GPU and flags and save the compiled module; return None.
        Returns:
            None: No return value.
        '''
        model = Mock(spec=torch.jit.ScriptModule)
        backend = Mock()
        device = torch.device('cuda:2')
        # Keep the model scripted so this test does not allocate a real CUDA tensor.
        with patch.object(compiler, 'torch_tensorrt', backend), \
                patch.object(torch.cuda, 'is_available', return_value=True), \
                patch.object(torch.cuda, 'device') as device_context, \
                patch.object(torch.cuda, 'current_device', return_value=2), \
                patch.object(torch.jit, 'save') as save:
            compiler.save_tensorrt_model(
                'output.pt', model, batch_size=8, device=device,
                dtype=torch.float32, require_full_compilation=True,
                truncate_long_and_double=True)
        device_context.assert_called_once_with(device)
        backend.Device.assert_called_once_with(gpu_id=2)
        model.to.assert_called_once_with(device, torch.float32)
        model.eval.assert_called_once_with()
        kwargs = backend.compile.call_args.kwargs
        self.assertEqual(kwargs['device'], backend.Device.return_value)
        self.assertTrue(kwargs['require_full_compilation'])
        self.assertTrue(kwargs['truncate_long_and_double'])
        backend.Input.assert_called_once_with(shape=(8, 11936), dtype=torch.float32)
        save.assert_called_once_with(backend.compile.return_value, 'output.pt')

    def test_unavailable_backend(self) -> None:
        '''Report missing TensorRT or CUDA before allocating a model; return None.
        Returns:
            None: No return value.
        '''
        with patch.object(compiler, 'torch_tensorrt', None), self.assertRaises(ImportError):
            compiler.compile_tensorrt_model(Mock())
        with patch.object(compiler, 'torch_tensorrt', Mock()), \
                patch.object(torch.cuda, 'is_available', return_value=False), \
                self.assertRaisesRegex(RuntimeError, 'CUDA is not available'):
            compiler.compile_tensorrt_model(Mock())
