# Copyright (c) 2017, Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

set(IGDRCL_SRCS_mt_tests_mocks
    #local files
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
    #necessary dependencies from igdrcl_tests
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_context.cpp"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_context.h"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_deferrable_deletion.cpp"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_deferrable_deletion.h"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_deferred_deleter.h"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_device.h"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_kernel.cpp"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_kernel.h"
    "${IGDRCL_SOURCE_DIR}/unit_tests/mocks/mock_memory_manager.h"
    PARENT_SCOPE
)
