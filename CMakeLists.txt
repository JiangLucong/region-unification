cmake_minimum_required(VERSION 3.16)
project(region_unification LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(region_unification src/region_unification.cpp)
target_include_directories(region_unification PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(region_unification ${OpenCV_LIBS})
