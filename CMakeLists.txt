cmake_minimum_required(VERSION 3.31)
project(untitled75)

set(CMAKE_CXX_STANDARD 20)
find_package(PostgreSQL REQUIRED)
find_package(fmt REQUIRED)

add_executable(untitled75 main.cpp data_base.cpp)

target_link_libraries(untitled75 ${PostgreSQL_LIBRARIES} fmt::fmt)

