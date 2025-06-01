cmake_minimum_required(VERSION 3.10)
project(boost_benchmark)

# Добавляем поддержку vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Добавляем политику для Boost
cmake_policy(SET CMP0167 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим Boost через vcpkg
find_package(Boost 1.88.0 REQUIRED COMPONENTS system graph)

# Добавляем исполняемый файл
add_executable(boost_benchmark boost_benchmark.cpp)

# Подключаем Boost
target_include_directories(boost_benchmark PRIVATE
    ${Boost_INCLUDE_DIRS}
)

# Добавляем флаги компиляции
if(MSVC)
    target_compile_options(boost_benchmark PRIVATE
        /bigobj
        /EHsc
        /utf-8
    )
endif()

# Подключаем библиотеки
target_link_libraries(boost_benchmark PRIVATE
    psapi
    ${Boost_LIBRARIES}
)
