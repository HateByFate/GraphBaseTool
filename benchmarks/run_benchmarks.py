import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from tqdm import tqdm
import os
import sys
import locale
import json
from datetime import datetime
import shutil
import glob

# Устанавливаем кодировку для вывода
sys.stdout.reconfigure(encoding='utf-8')
plt.rcParams['font.family'] = 'DejaVu Sans'  # Шрифт с поддержкой кириллицы

def find_project_root():
    current = os.path.abspath(os.getcwd())
    while True:
        if os.path.exists(os.path.join(current, "CMakeLists.txt")):
            print(f"[DEBUG] Найден корень проекта: {current}")
            return current
        parent = os.path.dirname(current)
        if parent == current:
            raise RuntimeError("Не удалось найти корень проекта (CMakeLists.txt)")
        current = parent

PROJECT_ROOT = find_project_root()
RESULTS_DIR = os.path.join(PROJECT_ROOT, 'results')

# Удаляем папку results из benchmarks, если она есть
benchmarks_results = os.path.join(os.path.dirname(__file__), 'results')
if os.path.exists(benchmarks_results):
    print(f"Удаляю лишнюю папку: {benchmarks_results}")
    shutil.rmtree(benchmarks_results)

print(f"[DEBUG] Папка results будет создана здесь: {RESULTS_DIR}")
os.makedirs(RESULTS_DIR, exist_ok=True)

def find_performance_test_exe():
    """Ищет performance_test.exe во всех подкаталогах build."""
    for root, dirs, files in os.walk(os.path.join(PROJECT_ROOT, 'build')):
        for file in files:
            if file == 'performance_test.exe':
                return os.path.join(root, file)
    return None

def run_benchmark(script_name):
    print(f"\nЗапуск бенчмарка для {script_name}...")
    try:
        # Если это python-скрипт
        if script_name.endswith('.py'):
            script_path = os.path.join('benchmarks', script_name)
            result = subprocess.run(
                ['python', script_path],
                capture_output=True,
                text=True,
                encoding=locale.getpreferredencoding(False),
                errors='replace'
            )
        # Если это boost_benchmark.cpp
        elif script_name == 'boost_benchmark.cpp':
            exe_path = os.path.join('benchmarks', 'build', 'Release', 'boost_benchmark.exe')
            if not os.path.exists(exe_path):
                raise FileNotFoundError(f"Не найден {exe_path}. Сначала соберите проект.")
            result = subprocess.run([exe_path],
                                  capture_output=True,
                                  text=True,
                                  encoding='utf-8')
        # Если это GraphBaseTool (или другой внешний exe)
        elif script_name == os.path.join(PROJECT_ROOT, 'build', 'Release', 'performance_test.exe'):
            exe_path = os.path.normpath(script_name)
            if not os.path.exists(exe_path):
                raise FileNotFoundError(f"Не найден {exe_path}. Сначала соберите проект.")
            result = subprocess.run([exe_path],
                                  capture_output=True,
                                  text=True,
                                  encoding='utf-8')
        else:
            # Для других возможных C++ бенчмарков
            exe_path = os.path.join('benchmarks', script_name.replace('.cpp', '.exe'))
            if not os.path.exists(exe_path):
                raise FileNotFoundError(f"Не найден {exe_path}. Сначала соберите проект.")
            result = subprocess.run([exe_path],
                                  capture_output=True,
                                  text=True,
                                  encoding='utf-8')
        if result.stdout:
            return result.stdout
        else:
            print(f"Предупреждение: нет вывода от {script_name}")
            return ""
    except Exception as e:
        print(f"Ошибка при выполнении {script_name}: {str(e)}")
        return ""

def parse_results(output):
    if not output:
        return []
    
    results = []
    
    for line in output.split('\n'):
        try:
            if line.strip().startswith('{'):
                result = json.loads(line)
                results.append(result)
        except json.JSONDecodeError:
            continue
        except Exception as e:
            print(f"Ошибка при разборе строки: {line}")
            print(f"Ошибка: {str(e)}")
            continue
    
    return results

def create_markdown_report(results, output_file):
    """Создает markdown отчет с результатами бенчмарков"""
    try:
        # Создаем директорию, если она не существует
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            # Заголовок
            f.write(f"# Результаты бенчмарков\n\n")
            f.write(f"Дата выполнения: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            if not results:
                f.write("Нет данных для отображения\n")
                return
            
            # Словарь для перевода операций на русский язык
            operation_translations = {
                "graph_creation": "Создание графа",
                "dijkstra": "Алгоритм Дейкстры",
                "floyd_warshall": "Алгоритм Флойда-Уоршелла"
            }
            
            # Определяем порядок операций
            operation_order = ["graph_creation", "dijkstra", "floyd_warshall"]
            
            # Сортируем результаты по операции, размеру графа и времени выполнения
            sorted_results = sorted(results, key=lambda x: (x['operation'], x['size'], x['time_ms']))
            
            # Фиксированная ширина столбцов
            column_widths = {
                'library': 15,  # Библиотека
                'size': 12,     # Число вершин
                'edges': 12,    # Число рёбер
                'time': 12,     # Время (мс)
                'memory': 12    # Память (МБ)
            }
            
            # Выводим таблицы для каждой операции в заданном порядке
            for operation in operation_order:
                operation_results = [r for r in sorted_results if r['operation'] == operation]
                if not operation_results:
                    continue
                    
                f.write(f"## {operation_translations.get(operation, operation)}\n\n")
                
                # Выводим таблицы для каждого размера графа
                for size in sorted(set(r['size'] for r in operation_results)):
                    f.write(f"### Размер {size} вершин\n\n")
                    size_results = [r for r in operation_results if r['size'] == size]
                    
                    # Создаем заголовок таблицы с фиксированной шириной
                    f.write("| " + "Библиотека".ljust(column_widths['library']) + " | " +
                           "Число вершин".ljust(column_widths['size']) + " | " +
                           "Число рёбер".ljust(column_widths['edges']) + " | " +
                           "Время (мс)".ljust(column_widths['time']) + " | " +
                           "Память (МБ)".ljust(column_widths['memory']) + " |\n")
                    
                    # Создаем разделитель
                    f.write("|" + "-" * (column_widths['library'] + 2) + "|" +
                           "-" * (column_widths['size'] + 2) + "|" +
                           "-" * (column_widths['edges'] + 2) + "|" +
                           "-" * (column_widths['time'] + 2) + "|" +
                           "-" * (column_widths['memory'] + 2) + "|\n")
                    
                    # Заполняем таблицу данными
                    for result in size_results:
                        f.write("| " + str(result['library']).ljust(column_widths['library']) + " | " +
                               str(result['size']).ljust(column_widths['size']) + " | " +
                               str(result['edges']).ljust(column_widths['edges']) + " | " +
                               f"{result['time_ms']:.2f}".ljust(column_widths['time']) + " | " +
                               f"{result['memory_mb']:.2f}".ljust(column_widths['memory']) + " |\n")
                    
                    f.write("\n")
    except Exception as e:
        print(f"Ошибка при создании отчета: {str(e)}")

def plot_results(results, metric, title, filename):
    if not results:
        print(f"Нет данных для создания графика {filename}")
        return
        
    try:
        # Преобразуем результаты в DataFrame для построения графика
        df = pd.DataFrame(results)
        
        plt.figure(figsize=(15, 8))
        sns.set_style("whitegrid")
        
        # Создаем график
        g = sns.lineplot(data=df, x='size', y=metric, hue='library', style='operation', marker='o')
        
        # Настраиваем внешний вид
        plt.title(title, fontsize=14, pad=20)
        plt.xlabel('Размер графа', fontsize=12)
        plt.ylabel(metric.replace('_', ' ').title(), fontsize=12)
        plt.xticks(rotation=45)
        
        # Настраиваем легенду
        plt.legend(title='Библиотека', bbox_to_anchor=(1.05, 1), loc='upper left')
        
        # Добавляем сетку
        plt.grid(True, linestyle='--', alpha=0.7)
        
        # Настраиваем отступы
        plt.tight_layout()
        
        # Создаем директорию, если она не существует
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        
        # Сохраняем график
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        plt.close()
    except Exception as e:
        print(f"Ошибка при создании графика {filename}: {str(e)}")
        print(f"Данные для графика: {results}")

def cleanup_old_files():
    """Удаляет старые файлы результатов"""
    files_to_remove = [
        os.path.join(PROJECT_ROOT, 'benchmark_results.csv'),
        os.path.join(PROJECT_ROOT, 'benchmark_results.md'),
        os.path.join(PROJECT_ROOT, 'benchmark_time_ms.png'),
        os.path.join(PROJECT_ROOT, 'benchmark_memory_mb.png'),
        os.path.join(PROJECT_ROOT, 'run_benchmarks.ps1')
    ]
    
    for file in files_to_remove:
        if os.path.exists(file):
            try:
                os.remove(file)
                print(f"Удален файл: {file}")
            except Exception as e:
                print(f"Ошибка при удалении файла {file}: {str(e)}")

def main():
    # Список скриптов для бенчмарков
    scripts = [
        'networkx_benchmark.py',
        'igraph_benchmark.py',
        'boost_benchmark.cpp',
    ]
    perf_test_path = find_performance_test_exe()
    if perf_test_path:
        scripts.append(perf_test_path)
    else:
        print("[WARNING] performance_test.exe не найден в build. Будет пропущен.")

    # Создаем список для хранения результатов
    all_results = []
    
    # Запускаем бенчмарки с прогресс-баром
    for script in tqdm(scripts, desc="Выполнение бенчмарков"):
        output = run_benchmark(script)
        results = parse_results(output)
        if results:
            # Определяем название библиотеки
            if script.endswith('performance_test.exe'):
                library_name = 'GraphBaseTool'
            else:
                library_name = script.replace('_benchmark.py', '').replace('_benchmark.cpp', '')
            
            # Добавляем название библиотеки к каждому результату
            for result in results:
                result['library'] = library_name
            
            all_results.extend(results)
    
    if all_results:
        # Сохраняем результаты в JSON
        json_path = os.path.join(RESULTS_DIR, 'benchmark_results.json')
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(all_results, f, indent=2, ensure_ascii=False)
        
        # Создаем графики
        plot_results(all_results, 'time_ms', 'Время выполнения алгоритмов', 
                    os.path.join(RESULTS_DIR, 'benchmark_time_ms.png'))
        plot_results(all_results, 'memory_mb', 'Использование памяти', 
                    os.path.join(RESULTS_DIR, 'benchmark_memory_mb.png'))
        
        # Создаем markdown отчет
        md_path = os.path.join(RESULTS_DIR, 'benchmark_results.md')
        create_markdown_report(all_results, md_path)
        
        # Удаляем старые файлы
        cleanup_old_files()
        
        print("\nБенчмарки завершены!")
        print("Результаты сохранены в директории 'results':")
        print(f"- {json_path}")
        print(f"- {os.path.join(RESULTS_DIR, 'benchmark_time_ms.png')}")
        print(f"- {os.path.join(RESULTS_DIR, 'benchmark_memory_mb.png')}")
        print(f"- {md_path}")
    else:
        print("\nОшибка: не удалось получить результаты бенчмарков")

if __name__ == "__main__":
    main() 
