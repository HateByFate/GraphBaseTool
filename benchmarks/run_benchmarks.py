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
from collections import defaultdict

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
    print(f"\nЗапуск бенчмарка для {script_path}...")
    
    # Определяем тип бенчмарка и имя библиотеки
    if script_path.endswith('.cpp') or script_path.endswith('.exe'):
        # Для C++ бенчмарков ищем исполняемый файл
        if script_path.endswith('.cpp'):
            exe_name = os.path.splitext(os.path.basename(script_path))[0] + '.exe'
            exe_path = find_executable(exe_name, root_dir)
            library_name = os.path.splitext(os.path.basename(script_path))[0]
        else:
            exe_path = script_path
            library_name = os.path.splitext(os.path.basename(script_path))[0]
            
        if not exe_path or not os.path.exists(exe_path):
            print(f"Ошибка при выполнении {script_path}: Не найден исполняемый файл. Сначала соберите проект.")
            return None
            
        try:
            # Запускаем исполняемый файл и получаем вывод
            result = subprocess.run(f'"{exe_path}"', shell=True, capture_output=True, text=True, encoding='utf-8')
            if result.returncode != 0:
                print(f"Ошибка при выполнении {script_path}: {result.stderr}")
                return None
            return result.stdout, library_name
        except Exception as e:
            print(f"Ошибка при выполнении {script_path}: {str(e)}")
            return None
    else:
        # Для Python скриптов
        try:
            # Устанавливаем переменные окружения для корректной работы с кодировкой
            env = os.environ.copy()
            env['PYTHONIOENCODING'] = 'utf-8'
            result = subprocess.run(f'python "{script_path}"', shell=True, capture_output=True, text=True, encoding='utf-8', env=env)
            if result.returncode != 0:
                print(f"Ошибка при выполнении {script_path}: {result.stderr}")
                return None
            library_name = os.path.splitext(os.path.basename(script_path))[0].replace('_benchmark', '')
            return result.stdout, library_name
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
    print(f"[DEBUG] Создана директория для результатов: {results_dir}")
    
    # Список бенчмарков
    benchmarks = [
        os.path.join(root_dir, "benchmarks", "networkx_benchmark.py"),
        os.path.join(root_dir, "benchmarks", "igraph_benchmark.py"),
        os.path.join(root_dir, "benchmarks", "boost_benchmark.cpp"),
        os.path.join(root_dir, "benchmarks", "performance_test.cpp")
    ]
    
    # Инициализирую словарь operations перед использованием
    operations = {}

    # Добавляю проверку на наличие данных для параллельного алгоритма Флойда-Уоршелла
    if 'floyd_warshall_parallel' not in operations:
        operations['floyd_warshall_parallel'] = []

    # Проверяю, что все бенчмарки для параллельного алгоритма Флойда-Уоршелла включены
    if 'performance_test.cpp' not in benchmarks:
        benchmarks.append(os.path.join(root_dir, 'benchmarks', 'performance_test.cpp'))
    
    # Запускаем бенчмарки
    results = []
    print("\nНачинаем выполнение бенчмарков...")
    with tqdm(total=len(benchmarks), desc="Выполнение бенчмарков", bar_format='{l_bar}{bar}| {n_fmt}/{total_fmt} [{elapsed}<{remaining}]') as pbar:
        for script in benchmarks:
            output = run_benchmark(script, root_dir)
            if output:
                output_text, library_name = output
                try:
                    # Парсим JSON из вывода
                    for line in output_text.strip().split('\n'):
                        if line.strip():
                            try:
                                result = json.loads(line)
                                result['library'] = library_name
                                results.append(result)
                            except json.JSONDecodeError as e:
                                print(f"Ошибка при парсинге строки: {line}")
                                print(f"Ошибка: {str(e)}")
                except Exception as e:
                    print(f"Ошибка при обработке результатов {script}: {str(e)}")
            pbar.update(1)
    
    if not results:
        print("Нет результатов для анализа")
        return
    
    print("\nОбработка результатов...")
    
    # Сохраняем результаты
    results_file = os.path.join(results_dir, "benchmark_results.json")
    with open(results_file, 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=2)
    
    # Создаем графики
    print("Создание графиков...")
    create_plots(results, results_dir)
    
    # Создаем отчет
    print("Создание отчета...")
    create_report(results, results_dir)
    
    print("\nРезультаты сохранены в директории 'results':")
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
        sizes = [d['size'] for d in data if 'time_ms' in d]
        times = [d['time_ms'] for d in data if 'time_ms' in d]
        if sizes and times:  # Проверяем, что есть данные для построения
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
        sizes = [d['size'] for d in data if 'memory_mb' in d]
        memory = [d['memory_mb'] for d in data if 'memory_mb' in d]
        if sizes and memory:  # Проверяем, что есть данные для построения
            plt.plot(sizes, memory, marker='o', label=op)
    plt.xlabel('Размер графа')
    plt.ylabel('Память (МБ)')
    plt.title('Использование памяти')
    plt.legend()
    plt.grid(True)
    plt.savefig(os.path.join(results_dir, 'benchmark_memory_mb.png'))
    plt.close()

def get_russian_op_name(op):
    mapping = {
        'graph_creation': 'Создание графа',
        'dijkstra': 'Алгоритм Дейкстры',
        'a_star': 'Алгоритм A*',
        'floyd_warshall': 'Алгоритм Флойда-Уоршелла',
        'floyd_warshall_parallel': 'Параллельный Флойд-Уоршелл',
        'negative_cycle': 'Поиск отрицательных циклов',
        'memory_profile': 'Профилирование памяти',
    }
    return mapping.get(op, op)

def create_report(results, results_dir):
    """Создает отчет в формате Markdown по строгому шаблону пользователя."""
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    report = f"# Результаты бенчмарков\n\nВремя выполнения: {current_time}\n\n"

    # Операции и порядок
    op_order = [
        'graph_creation',
        'dijkstra',
        'a_star',
        'floyd_warshall',
        'floyd_warshall_parallel',
        'negative_cycle'
    ]
    op_libs = {
        'graph_creation': ['networkx', 'boost_benchmark', 'igraph', 'GraphBaseTool'],
        'dijkstra': ['boost_benchmark', 'GraphBaseTool', 'networkx', 'igraph'],
        'a_star': ['boost_benchmark', 'GraphBaseTool', 'networkx', 'igraph'],
        'floyd_warshall': ['boost_benchmark', 'igraph', 'GraphBaseTool', 'networkx'],
        'floyd_warshall_parallel': ['GraphBaseTool'],
        'negative_cycle': ['GraphBaseTool'],
    }
    sizes = [100, 200, 500, 1000]
    size_to_edges = {100: 200, 200: 400, 500: 1000, 1000: 2000}

    def norm_lib(lib):
        if lib.lower() in ['performance_test', 'graphbasetool']:
            return 'GraphBaseTool'
        if lib.lower() in ['boost_benchmark', 'boost']:
            return 'boost_benchmark'
        if lib.lower() in ['networkx']:
            return 'networkx'
        if lib.lower() in ['igraph']:
            return 'igraph'
        return lib

    grouped = defaultdict(lambda: defaultdict(dict))
    for r in results:
        op = r['operation']
        size = r['size']
        lib = norm_lib(r.get('library', 'Unknown'))
        grouped[op][size][lib] = r

    for op in op_order:
        report += f"## {get_russian_op_name(op)}\n\n"
        for size in sizes:
            report += f"### Граф с {size} вершинами\n\n"
            report += "| {:<15} | {:<12} | {:<12} | {:<12} | {:<12} |\n".format(
                "Библиотека", "Вершины", "Рёбра", "Время (мс)", "Память (МБ)")
            report += "|{:-<17}|{:-<14}|{:-<14}|{:-<14}|{:-<14}|\n".format('', '', '', '', '')
            
            # Получаем все результаты для текущей операции и размера
            current_results = []
            for lib in op_libs[op]:
                res = grouped[op][size].get(lib)
                if res:
                    current_results.append((lib, res))
            
            # Сортируем по времени выполнения, если оно есть
            current_results.sort(key=lambda x: x[1].get('time_ms', float('inf')))
            
            # Выводим результаты
            for lib, res in current_results:
                time_ms = res.get('time_ms', 0)
                memory_mb = res.get('memory_mb', 0)
                report += "| {:<15} | {:<12} | {:<12} | {:<12.2f} | {:<12.2f} |\n".format(
                    lib, size, size_to_edges[size], time_ms, memory_mb
                )
            
            # Добавляем библиотеки без результатов
            missing_libs = set(op_libs[op]) - {lib for lib, _ in current_results}
            for lib in missing_libs:
                report += "| {:<15} | {:<12} | {:<12} | {:<12} | {:<12} |\n".format(
                    lib, size, size_to_edges[size], '-', '-')
            
            report += "\n"

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
