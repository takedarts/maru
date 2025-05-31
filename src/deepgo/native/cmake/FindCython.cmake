#=============================================================================
# This is imported from https://github.com/thewtex/cython-cmake-example .
#
# This code is modified to be compatible with Cmake version 3.12 or later.
# Modules `FindPythonInterp` and `FindPythonLibs` are deprecated in CMake 3.12.
# So, `Python3` is used instead of `PythonInterp` or `PythonLibs`.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# Atsushi TAKEDA, 2024
#=============================================================================

# Find the Cython compiler.
#
# This code sets the following variables:
#
#  CYTHON_EXECUTABLE
#
# See also UseCython.cmake

#=============================================================================
# Copyright 2011 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

# Use the Cython executable that lives next to the Python executable
# if it is a local installation.
find_package( Python3 COMPONENTS Interpreter REQUIRED )

get_filename_component( _python_path ${Python3_EXECUTABLE} PATH )
find_program( CYTHON_EXECUTABLE
  NAMES cython cython.bat cython3
  HINTS ${_python_path}
)

mark_as_advanced( CYTHON_EXECUTABLE )
