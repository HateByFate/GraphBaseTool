# Добавляем поддержку vcpkg (эта строка должна быть в самом начале)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

cmake_minimum_required(VERSION 3.15)
project(GraphBaseTool VERSION 1.0.0 LANGUAGES CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем генерацию compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Находим пакет Boost
find_package(Boost REQUIRED COMPONENTS system graph)

# Добавляем директории с заголовочными файлами
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

# Создаем библиотеку
add_library(GraphBaseTool
    src/graph.cpp
)

# Линкуем с Boost
target_link_libraries(GraphBaseTool PRIVATE Boost::system Boost::graph)

# Устанавливаем свойства библиотеки
set_target_properties(GraphBaseTool PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "include/GraphBaseTool/graph.hpp"
)

# Создаем исполняемые файлы для тестов
add_executable(graph_test tests/graph_test.cpp)
target_link_libraries(graph_test PRIVATE GraphBaseTool)

add_executable(performance_test tests/performance_test.cpp)
target_link_libraries(performance_test PRIVATE GraphBaseTool)

# Добавляем бенчмарк для Boost
add_executable(boost_benchmark benchmarks/boost_benchmark.cpp)
target_link_libraries(boost_benchmark PRIVATE GraphBaseTool)

# Устанавливаем выходные директории
set_target_properties(GraphBaseTool graph_test performance_test boost_benchmark
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Устанавливаем библиотеку
install(TARGETS GraphBaseTool
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/GraphBaseTool
)

# Устанавливаем заголовочные файлы
install(DIRECTORY include/
    DESTINATION include/GraphBaseTool
    FILES_MATCHING PATTERN "*.hpp"
)

# Добавляем тесты в CTest
enable_testing()
add_test(NAME performance_test COMMAND performance_test)
add_test(NAME graph_test COMMAND graph_test)
