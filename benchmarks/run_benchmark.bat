@echo off
chcp 65001 > nul
setlocal

:: Создаем директорию для сборки
if not exist build mkdir build
cd build

:: Генерируем проект
cmake -G "Visual Studio 17 2022" -A x64 .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

:: Собираем проект
cmake --build . --config Release

:: Запускаем бенчмарк
echo Запуск бенчмарка...
Release\boost_benchmark.exe

cd .. 
