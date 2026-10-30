cmake_minimum_required(VERSION 3.14)
project(cube_solver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(cubecore src/cubie.cpp src/coord.cpp src/solver.cpp)
target_include_directories(cubecore PUBLIC include)
if(MSVC)
  target_compile_options(cubecore PRIVATE /W4)
else()
  target_compile_options(cubecore PRIVATE -Wall -Wextra)
endif()

add_executable(cubesolver src/main.cpp)
target_link_libraries(cubesolver PRIVATE cubecore)

enable_testing()
add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE cubecore)
add_test(NAME tests COMMAND tests)
