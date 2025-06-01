@echo off
chcp 65001 > nul
setlocal

:: Проверяем наличие Python и устанавливаем зависимости
echo Проверка зависимостей Python...
python -m pip install -r requirements.txt

:: Запускаем C++ бенчмарк
echo.
echo Запуск C++ бенчмарка (Boost.Graph)...
call run_benchmark.bat

:: Запускаем Python бенчмарки
echo.
echo Запуск Python бенчмарков...
python networkx_benchmark.py
python igraph_benchmark.py

:: Запускаем скрипт для создания графиков
echo.
echo Создание графиков сравнения...
python run_benchmarks.py

:: Копируем результаты из results в корень, если нужно
if exist ..\results\benchmark_results.csv copy ..\results\benchmark_results.csv ..\benchmark_results.csv
if exist ..\results\benchmark_time_ms.png copy ..\results\benchmark_time_ms.png ..\benchmark_time_ms.png
if exist ..\results\benchmark_memory_mb.png copy ..\results\benchmark_memory_mb.png ..\benchmark_memory_mb.png
if exist ..\results\benchmark_results.md copy ..\results\benchmark_results.md ..\benchmark_results.md

echo.
echo Все бенчмарки завершены!
echo Результаты сохранены в файлы:
echo - ..\benchmark_results.csv
echo - ..\benchmark_time_ms.png
echo - ..\benchmark_memory_mb.png
echo - ..\benchmark_results.md 
