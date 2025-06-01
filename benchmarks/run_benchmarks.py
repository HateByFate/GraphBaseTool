import os
import sys
import json
import time
import subprocess
import argparse
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np
from tqdm import tqdm
import pandas as pd
import seaborn as sns
import locale
import shutil
import glob
from datetime import datetime

# Устанавливаем кодировку для вывода
sys.stdout.reconfigure(encoding='utf-8')
plt.rcParams['font.family'] = 'DejaVu Sans'  # Шрифт с поддержкой кириллицы

def find_executable(name, root_dir):
    """Ищет исполняемый файл в различных возможных местах."""
    possible_paths = [
        os.path.join(root_dir, "build", "bin", "Release", name),
        os.path.join(root_dir, "build", "Release", name),
        os.path.join(root_dir, "build", name),
        os.path.join(root_dir, name)
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            return path
    return None

def run_benchmark(script_path, root_dir):
    """Запускает бенчмарк и возвращает его результаты."""
    print(f"Запуск бенчмарка для {script_path}...")
    
    # Определяем тип бенчмарка
    if script_path.endswith('.cpp'):
        # Для C++ бенчмарков ищем исполняемый файл
        exe_name = os.path.splitext(os.path.basename(script_path))[0] + '.exe'
        exe_path = find_executable(exe_name, root_dir)
        if not exe_path:
            print(f"Ошибка при выполнении {script_path}: Не найден {exe_name}. Сначала соберите проект.")
            return None
        cmd = f'"{exe_path}"'
    else:
        # Для Python скриптов
        cmd = f'python "{script_path}"'
    
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Ошибка при выполнении {script_path}: {result.stderr}")
            return None
        return result.stdout
    except Exception as e:
        print(f"Ошибка при выполнении {script_path}: {str(e)}")
        return None

def main():
    parser = argparse.ArgumentParser(description='Запуск бенчмарков')
    parser.add_argument('--root-dir', type=str, help='Корневая директория проекта')
    args = parser.parse_args()
    
    # Определяем корневую директорию
    root_dir = args.root_dir if args.root_dir else os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    print(f"[DEBUG] Найден корень проекта: {root_dir}")
    
    # Создаем директорию для результатов
    results_dir = os.path.join(root_dir, "results")
    if not os.path.exists(results_dir):
        os.makedirs(results_dir)
    print(f"[DEBUG] Папка results будет создана здесь: {results_dir}")
    
    # Список бенчмарков
    benchmarks = [
        os.path.join(root_dir, "benchmarks", "networkx_benchmark.py"),
        os.path.join(root_dir, "benchmarks", "igraph_benchmark.py"),
        os.path.join(root_dir, "benchmarks", "boost_benchmark.cpp"),
        os.path.join(root_dir, "build", "bin", "Release", "performance_test.exe")
    ]
    
    # Запускаем бенчмарки
    results = []
    for script in tqdm(benchmarks, desc="Выполнение бенчмарков"):
        output = run_benchmark(script, root_dir)
        if output:
            try:
                # Парсим JSON из вывода
                for line in output.strip().split('\n'):
                    if line.strip():
                        result = json.loads(line)
                        results.append(result)
            except json.JSONDecodeError as e:
                print(f"Ошибка при парсинге результатов {script}: {str(e)}")
    
    if not results:
        print("Нет результатов для анализа")
        return
    
    # Сохраняем результаты
    results_file = os.path.join(results_dir, "benchmark_results.json")
    with open(results_file, 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=2)
    
    # Создаем графики
    create_plots(results, results_dir)
    
    # Создаем отчет
    create_report(results, results_dir)
    
    print("\nБенчмарки завершены!")
    print("Результаты сохранены в директории 'results':")
    print(f"- {results_file}")
    print(f"- {os.path.join(results_dir, 'benchmark_time_ms.png')}")
    print(f"- {os.path.join(results_dir, 'benchmark_memory_mb.png')}")
    print(f"- {os.path.join(results_dir, 'benchmark_results.md')}")

def create_plots(results, results_dir):
    """Создает графики на основе результатов."""
    # Группируем результаты по операциям
    operations = {}
    for result in results:
        op = result['operation']
        if op not in operations:
            operations[op] = []
        operations[op].append(result)
    
    # Создаем график времени выполнения
    plt.figure(figsize=(12, 6))
    for op, data in operations.items():
        sizes = [d['size'] for d in data]
        times = [d['time_ms'] for d in data]
        plt.plot(sizes, times, marker='o', label=op)
    plt.xlabel('Размер графа')
    plt.ylabel('Время (мс)')
    plt.title('Время выполнения операций')
    plt.legend()
    plt.grid(True)
    plt.savefig(os.path.join(results_dir, 'benchmark_time_ms.png'))
    plt.close()
    
    # Создаем график использования памяти
    plt.figure(figsize=(12, 6))
    for op, data in operations.items():
        sizes = [d['size'] for d in data]
        memory = [d['memory_mb'] for d in data]
        plt.plot(sizes, memory, marker='o', label=op)
    plt.xlabel('Размер графа')
    plt.ylabel('Память (МБ)')
    plt.title('Использование памяти')
    plt.legend()
    plt.grid(True)
    plt.savefig(os.path.join(results_dir, 'benchmark_memory_mb.png'))
    plt.close()

def create_report(results, results_dir):
    """Создает отчет в формате Markdown."""
    # Группируем результаты по операциям
    operations = {}
    for result in results:
        op = result['operation']
        if op not in operations:
            operations[op] = []
        operations[op].append(result)
    
    # Создаем отчет
    report = "# Результаты бенчмарков\n\n"
    
    for op, data in operations.items():
        report += f"## {op}\n\n"
        report += "| Размер | Время (мс) | Память (МБ) | Рёбра |\n"
        report += "|--------|------------|-------------|--------|\n"
        
        for result in sorted(data, key=lambda x: x['size']):
            report += f"| {result['size']} | {result['time_ms']:.2f} | {result['memory_mb']:.2f} | {result['edges']} |\n"
        
        report += "\n"
    
    # Сохраняем отчет
    with open(os.path.join(results_dir, 'benchmark_results.md'), 'w', encoding='utf-8') as f:
        f.write(report)

def cleanup_old_files():
    """Удаляет старые файлы результатов"""
    files_to_remove = [
        os.path.join(root_dir, 'benchmark_results.csv'),
        os.path.join(root_dir, 'benchmark_results.md'),
        os.path.join(root_dir, 'benchmark_time_ms.png'),
        os.path.join(root_dir, 'benchmark_memory_mb.png'),
        os.path.join(root_dir, 'run_benchmarks.ps1')
    ]
    
    for file in files_to_remove:
        if os.path.exists(file):
            try:
                os.remove(file)
                print(f"Удален файл: {file}")
            except Exception as e:
                print(f"Ошибка при удалении файла {file}: {str(e)}")

if __name__ == "__main__":
    main() 
