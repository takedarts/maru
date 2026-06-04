# Find python package
find_package( Python3 COMPONENTS Interpreter REQUIRED )

# Get the path to python
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find the include path
find_path ( TORCH_TENSORRT_INCLUDE_DIRS
  NAMES .
  HINTS "${PYTHON_SITE_PACKAGES}/torch_tensorrt/include"
  NO_DEFAULT_PATH
)

# Find the libraries
find_library( TORCH_TORCHRT_LIB
  NAMES torchtrt
  HINTS "${PYTHON_SITE_PACKAGES}/torch_tensorrt/lib"
  NO_DEFAULT_PATH
)

find_library( TORCH_GLOBAL_DEPS_LIB
    NAMES torch_global_deps
    HINTS "${PYTHON_SITE_PACKAGES}/torch/lib"
    NO_DEFAULT_PATH
)

file(GLOB TORCH_LIBNVINFER_FILES "${PYTHON_SITE_PACKAGES}/tensorrt_libs/libnvinfer.*")
if( TORCH_LIBNVINFER_FILES )
  list( GET TORCH_LIBNVINFER_FILES 0 TORCH_LIBNVINFER_LIB )
else()
  set( TORCH_LIBNVINFER_LIB "TORCH_LIBNVINFER_LIB-NOTFOUND" )
endif()

file(GLOB TORCH_LIBNVINFER_PLUGIN_FILES "${PYTHON_SITE_PACKAGES}/tensorrt_libs/libnvinfer_plugin.*")
if( TORCH_LIBNVINFER_PLUGIN_FILES )
  list( GET TORCH_LIBNVINFER_PLUGIN_FILES 0 TORCH_LIBNVINFER_PLUGIN_LIB )
else()
  set( TORCH_LIBNVINFER_PLUGIN_LIB "TORCH_LIBNVINFER_PLUGIN_LIB-NOTFOUND" )
endif()

if( TORCH_TORCHRT_LIB AND TORCH_GLOBAL_DEPS_LIB AND
    TORCH_LIBNVINFER_LIB AND TORCH_LIBNVINFER_PLUGIN_LIB )
  message( STATUS "TorchTensorRT: found")
  message( STATUS "TorchTensorRT: torchtrt: ${TORCH_TORCHRT_LIB}")
  message( STATUS "TorchTensorRT: torch_global_deps: ${TORCH_GLOBAL_DEPS_LIB}")
  message( STATUS "TorchTensorRT: libnvinfer: ${TORCH_LIBNVINFER_LIB}")
  message( STATUS "TorchTensorRT: libnvinfer_plugin: ${TORCH_LIBNVINFER_PLUGIN_LIB}")
  set( USE_TORCH_TENSORRT ON )
  set( TORCH_TENSORRT_LIBRARIES ${TORCH_TORCHRT_LIB} ${TORCH_GLOBAL_DEPS_LIB})
  set( TORCH_TENSORRT_LIBRARIES ${TORCH_TENSORRT_LIBRARIES} ${TORCH_LIBNVINFER_LIB} )
  set( TORCH_TENSORRT_LIBRARIES ${TORCH_TENSORRT_LIBRARIES} ${TORCH_LIBNVINFER_PLUGIN_LIB} )
else()
  message( STATUS "TorchTensorRT: not found, Compiling without TorchTensorRT supported")
endif()
