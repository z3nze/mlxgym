cmake_minimum_required(VERSION 3.24)
project(mlxgym LANGUAGES CXX OBJCXX)

include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT APPLE)
  message(FATAL_ERROR "mlxgym requires macOS and a Metal-capable Apple GPU")
endif()

find_program(XCRUN_EXECUTABLE xcrun REQUIRED)
execute_process(COMMAND xcodebuild -showComponent MetalToolchain -json
  RESULT_VARIABLE METAL_RESULT OUTPUT_VARIABLE METAL_COMPONENT_JSON
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT METAL_RESULT EQUAL 0)
  message(FATAL_ERROR
    "The optional Xcode Metal Toolchain is missing. Install it in Xcode > Settings > Components, or run: xcodebuild -downloadComponent metalToolchain")
endif()
string(JSON METAL_TOOLCHAIN_ID GET "${METAL_COMPONENT_JSON}" toolchainIdentifier)
message(STATUS "Using Metal toolchain: ${METAL_TOOLCHAIN_ID}")

add_library(mlxgym_framework STATIC framework/src/metal_context.mm framework/src/suite.cpp)
target_include_directories(mlxgym_framework PUBLIC framework/include)
target_compile_options(mlxgym_framework PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>
  $<$<COMPILE_LANGUAGE:OBJCXX>:-Wall;-Wextra;-Wpedantic;-fobjc-arc>)
target_link_libraries(mlxgym_framework PUBLIC "-framework Foundation" "-framework Metal")

include(cmake/MetalTask.cmake)

set(MLXGYM_TASKS
  vector_add matrix_multiplication matrix_transpose color_inversion
  matrix_addition convolution_1d reverse_array relu leaky_relu rainbow_table
  matrix_copy sigmoid_linear_unit swish_gated_linear_unit value_clipping
  interleave_arrays gaussian_error_gated_linear_unit rgb_to_grayscale
  sigmoid_activation reduction softmax softmax_attention convolution_2d batch_normalization)

foreach(task IN LISTS MLXGYM_TASKS)
  add_subdirectory(problems/${task})
endforeach()

add_custom_target(all_shaders)
foreach(task IN LISTS MLXGYM_TASKS)
  add_dependencies(all_shaders ${task}_shader)
endforeach()
