# Добавляем поддержку vcpkg (эта строка должна быть в самом начале)
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows")
else()
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")
endif()

# Включаем поддержку манифеста vcpkg
set(VCPKG_MANIFEST_MODE ON)

cmake_minimum_required(VERSION 3.15)
project(GraphBaseTool VERSION 1.0.0 LANGUAGES CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем генерацию compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Устанавливаем пути для выходных файлов
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Включаем оптимизации
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /Gy /Gw")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    # Отключаем проверки времени выполнения
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
else()
    add_compile_options(-O3 -march=native -ffast-math -flto)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

# Находим пакеты
find_package(OpenMP REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

if(OpenMP_CXX_FOUND)
    add_compile_options(${OpenMP_CXX_FLAGS})
endif()

# Добавляем директории с заголовочными файлами
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_PREFIX_PATH}/include
)

# Добавляем исходные файлы
set(SOURCES
    src/graph.cpp
)

# Создаем библиотеку
add_library(GraphBaseTool STATIC ${SOURCES})

# Добавляем заголовочные файлы
target_include_directories(GraphBaseTool
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Линкуем с зависимостями
if(OpenMP_CXX_FOUND)
    target_link_libraries(GraphBaseTool PRIVATE OpenMP::OpenMP_CXX)
endif()

target_link_libraries(GraphBaseTool PRIVATE nlohmann_json::nlohmann_json)

# Устанавливаем свойства библиотеки
set_target_properties(GraphBaseTool PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "include/GraphBaseTool/graph.hpp"
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Создаем исполняемые файлы для тестов
add_executable(graph_test tests/main.cpp)
target_link_libraries(graph_test PRIVATE GraphBaseTool Catch2::Catch2WithMain)
target_include_directories(graph_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Создаем исполняемый файл для тестов производительности
add_executable(performance_test benchmarks/performance_test.cpp)
target_link_libraries(performance_test PRIVATE GraphBaseTool)
target_include_directories(performance_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    target_link_libraries(performance_test PRIVATE psapi)
endif()

# Добавляем бенчмарк для Boost
add_executable(boost_benchmark benchmarks/boost_benchmark.cpp)
target_link_libraries(boost_benchmark PRIVATE GraphBaseTool)
target_include_directories(boost_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Устанавливаем выходные директории для исполняемых файлов
set_target_properties(graph_test performance_test boost_benchmark
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
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
