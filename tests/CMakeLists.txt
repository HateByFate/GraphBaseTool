cmake_minimum_required(VERSION 3.10)
project(performance_tests)

# Добавляем поддержку vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим Catch2 и Boost
find_package(Catch2 CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS graph)

# Добавляем исполняемый файл
add_executable(performance_test performance_test.cpp ../src/graph.cpp)
add_executable(boost_benchmark ../benchmarks/boost_benchmark.cpp)

# Подключаем заголовочные файлы
target_include_directories(performance_test PRIVATE
    ${CMAKE_SOURCE_DIR}/../include
    C:/vcpkg/installed/x64-windows/include
)

target_include_directories(boost_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/../include
    C:/vcpkg/installed/x64-windows/include
    ${Boost_INCLUDE_DIRS}
)

# Добавляем флаги компиляции
if(MSVC)
    target_compile_options(performance_test PRIVATE
        /bigobj
        /EHsc
        /utf-8
    )
    target_compile_options(boost_benchmark PRIVATE
        /bigobj
        /EHsc
        /utf-8
    )
endif()

# Подключаем библиотеки
if(WIN32)
    target_link_libraries(performance_test PRIVATE psapi)
    target_link_libraries(boost_benchmark PRIVATE psapi)
endif()

# Подключаем Catch2 и Boost
target_link_libraries(performance_test PRIVATE Catch2::Catch2)
target_link_libraries(boost_benchmark PRIVATE ${Boost_LIBRARIES}) 
