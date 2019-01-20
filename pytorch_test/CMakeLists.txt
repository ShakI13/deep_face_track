cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(pytorch_test)

find_package(Torch REQUIRED)

add_executable(pytorch_test pytorch_test.cpp)
target_link_libraries(pytorch_test "${TORCH_LIBRARIES}")
set_property(TARGET pytorch_test PROPERTY CXX_STANDARD 11)