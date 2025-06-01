from setuptools import setup, find_packages, Command
import os
import sys
import subprocess
import platform
import shutil
from pathlib import Path
import inquirer
import git
import json
import time
from inquirer import themes
from blessed import Terminal

EYE_LOGO = r"""
く__,.ヘヽ.　　　　/　,ー､ 〉
　　　　　＼ ', !-─‐-i　/　/´
　　　 　 ／｀ｰ'　　　 L/／｀ヽ､
　　 　 /　 ／,　 /|　 ,　 ,　　　 ',
　　　ｲ 　/ /-‐/　ｉ　L_ ﾊ ヽ!　 i
　　　 ﾚ ﾍ 7ｲ｀ﾄ　 ﾚ'ｧ-ﾄ､!ハ|　 |
　　　　 !,/7 '0'　　 ´0iソ| 　 |　　　
　　　　 |.从"　　_　　 ,,,, / |./ 　 |
　　　　 ﾚ'| i＞.､,,__　_,.イ / 　.i 　|
　　　　　 ﾚ'| | / k_７_/ﾚ'ヽ,　ﾊ.　|
　　　　　　 | |/i 〈|/　 i　,.ﾍ |　i　|
　　　　　　.|/ /　ｉ： 　 ﾍ!　　＼　|
　　　 　 　 kヽ>､ﾊ 　 _,.ﾍ､ 　 /､!
　　　　　　 !'〈//｀Ｔ´', ＼ ｀'7'ｰr'
　　　　　　 ﾚ'ヽL__|___i,___,ンﾚ|ノ
　　　　　 　　　ﾄ-,/　|___./
　　　　　 　　　'ｰ'　　!_,.: 
"""

# Используем стандартную тему
custom_theme = themes.BlueComposure()

def print_step(message):
    print(f"\n=== {message} ===")

def run_command(command, shell=True):
    try:
        subprocess.run(command, shell=shell, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Ошибка при выполнении команды: {e}")
        return False

def check_vcpkg():
    print_step("Проверка vcpkg")
    
    # Проверяем, установлен ли vcpkg
    vcpkg_path = os.environ.get('VCPKG_ROOT')
    if not vcpkg_path:
        print("vcpkg не найден в переменных окружения.")
        return False
    
    # Проверяем наличие vcpkg.exe
    vcpkg_exe = os.path.join(vcpkg_path, 'vcpkg.exe')
    if not os.path.exists(vcpkg_exe):
        print(f"vcpkg.exe не найден по пути: {vcpkg_exe}")
        return False
    
    return True

def install_vcpkg():
    print_step("Установка vcpkg")
    
    # Создаем директорию для vcpkg
    vcpkg_dir = os.path.join(os.path.expanduser("~"), "vcpkg")
    if not os.path.exists(vcpkg_dir):
        os.makedirs(vcpkg_dir)
    
    # Клонируем репозиторий vcpkg
    if not run_command(f"git clone https://github.com/microsoft/vcpkg.git {vcpkg_dir}"):
        return False
    
    # Запускаем bootstrap
    if not run_command(f"{vcpkg_dir}\\bootstrap-vcpkg.bat"):
        return False
    
    # Добавляем путь в переменные окружения
    os.environ['VCPKG_ROOT'] = vcpkg_dir
    os.environ['PATH'] = f"{vcpkg_dir};{os.environ['PATH']}"
    
    return True

def install_dependencies():
    print_step("Установка зависимостей")
    
    # Устанавливаем Boost
    if not run_command("vcpkg install boost:x64-windows"):
        return False
    
    # Интегрируем vcpkg с Visual Studio
    if not run_command("vcpkg integrate install"):
        return False
    
    return True

def build_project():
    print_step("Сборка проекта")
    
    # Создаем директорию build
    build_dir = "build"
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    os.makedirs(build_dir)
    
    # Получаем путь к vcpkg
    vcpkg_root = os.environ.get('VCPKG_ROOT')
    if not vcpkg_root:
        print("VCPKG_ROOT не найден в переменных окружения")
        return False
    
    # Запускаем CMake
    cmake_command = f"cmake .. -DCMAKE_TOOLCHAIN_FILE={vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
    if not run_command(f"cd {build_dir} && {cmake_command}"):
        return False
    
    # Собираем проект
    if not run_command(f"cd {build_dir} && cmake --build . --config Release"):
        return False
    
    return True

def run_tests():
    print_step("Запуск тестов")
    
    # Запускаем unit-тесты
    test_path = os.path.join("build", "bin", "Release", "graph_test.exe")
    if not os.path.exists(test_path):
        print(f"Ошибка: Файл теста не найден по пути: {test_path}")
        return False
        
    if not run_command(f'"{test_path}"'):
        return False
    
    # Запускаем тесты производительности
    perf_test_path = os.path.join("build", "bin", "Release", "performance_test.exe")
    if not os.path.exists(perf_test_path):
        print(f"Ошибка: Файл теста производительности не найден по пути: {perf_test_path}")
        return False
        
    if not run_command(f'"{perf_test_path}"'):
        return False
    
    return True

def run_benchmarks():
    print_step("Запуск бенчмарков")
    
    # Запускаем бенчмарки
    if not run_command("python benchmarks/run_benchmarks.py"):
        return False
    
    return True

def uninstall_project():
    clear_screen()
    print("\n" + "="*50)
    print("Удаление проекта".center(50))
    print("="*50 + "\n")
    
    try:
        # Получаем путь к текущей директории
        current_dir = os.path.dirname(os.path.abspath(__file__))
        
        # Список файлов и папок для удаления
        items_to_remove = [
            'build',
            'results',
            'vcpkg',
            'CMakeFiles',
            'CMakeCache.txt',
            'cmake_install.cmake',
            'Makefile',
            'GraphBaseTool.sln',
            'x64',
            'Debug',
            'Release',
            'GraphBaseTool.vcxproj',
            'GraphBaseTool.vcxproj.filters',
            'GraphBaseTool.vcxproj.user',
            'GraphBaseTool.exe',
            'graph.cpp',
            'graph.h',
            'main.cpp',
            'run_benchmarks.ps1',
            'setup.py',
            '.git',
            '.gitignore'
        ]
        
        # Удаляем каждый элемент
        for item in items_to_remove:
            item_path = os.path.join(current_dir, item)
            if os.path.exists(item_path):
                if os.path.isfile(item_path):
                    os.remove(item_path)
                    print(f"Удален файл: {item}")
                elif os.path.isdir(item_path):
                    shutil.rmtree(item_path)
                    print(f"Удалена папка: {item}")
        
        print("\nПроект успешно удален!")
        time.sleep(2)
        
        # Закрываем окно PowerShell
        if os.name == 'nt':
            os.system('exit')
        else:
            os.system('kill -9 $$')
            
    except Exception as e:
        print(f"\nОшибка при удалении: {str(e)}")
        time.sleep(2)
        return

def update():
    clear_screen()
    print("\n" + "="*50)
    print("Обновление проекта".center(50))
    print("="*50 + "\n")
    
    try:
        # Проверяем, является ли текущая директория Git репозиторием
        if not os.path.exists('.git'):
            print("Ошибка: Текущая директория не является Git репозиторием")
            time.sleep(2)
            return False
            
        # Получаем текущий репозиторий
        repo = git.Repo(".")
        
        # Проверяем, есть ли удаленный репозиторий
        if not repo.remotes:
            print("Ошибка: Не найден удаленный репозиторий")
            time.sleep(2)
            return False
            
        print("Получение последних изменений...")
        # Получаем последние изменения
        origin = repo.remotes.origin
        origin.fetch()
        
        # Проверяем, есть ли локальные изменения
        if repo.is_dirty():
            print("Обнаружены локальные изменения. Сначала сохраните или отмените их.")
            time.sleep(2)
            return False
            
        # Получаем текущую ветку
        current_branch = repo.active_branch
        
        print(f"Обновление ветки {current_branch.name}...")
        # Обновляем текущую ветку
        origin.pull(current_branch.name)
        
        print("Пересборка проекта...")
        # Пересобираем проект
        if not build_project():
            print("Ошибка при пересборке проекта")
            time.sleep(2)
            return False
        
        print("\nПроект успешно обновлен!")
        time.sleep(2)
        return True
        
    except git.GitCommandError as e:
        print(f"Ошибка при обновлении: {e}")
        time.sleep(2)
        return False
    except Exception as e:
        print(f"Неожиданная ошибка: {e}")
        time.sleep(2)
        return False

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def show_menu():
    while True:
        clear_screen()
        # Выводим логотип
        print(EYE_LOGO)
        print("\nВыберите действие:\n")
        
        questions = [
            inquirer.List('action',
                message="",
                choices=[
                    'Полная установка',
                    'Тесты производительности',
                    'Полное удаление',
                    'Обновление',
                    'Выход'
                ]
            ),
        ]
        answers = inquirer.prompt(questions, theme=custom_theme)
        clear_screen()
        if answers is None:
            # Если пользователь нажал Ctrl+C или Esc
            show_farewell()
            break
        action = answers['action']
        if action == 'Полная установка':
            if not check_vcpkg():
                print("Установка vcpkg...")
                if not install_vcpkg():
                    print("Ошибка при установке vcpkg")
                    input("Нажмите Enter для возврата в меню...")
                    continue
            if not install_dependencies():
                print("Ошибка при установке зависимостей")
                input("Нажмите Enter для возврата в меню...")
                continue
            if not build_project():
                print("Ошибка при сборке проекта")
                input("Нажмите Enter для возврата в меню...")
                continue
            if not run_tests():
                print("Ошибка при запуске тестов")
                input("Нажмите Enter для возврата в меню...")
                continue
            print("\n=== Установка успешно завершена! ===")
            print("\nТеперь вы можете использовать GraphBaseTool в своих проектах.")
            print("Пример использования можно найти в README.md")
            input("Нажмите Enter для возврата в меню...")
        elif action == 'Тесты производительности':
            if not run_benchmarks():
                print("Ошибка при запуске бенчмарков")
                input("Нажмите Enter для возврата в меню...")
                continue
            print("\n=== Бенчмарки успешно завершены! ===")
            print("Результаты сохранены в папке 'results'")
            input("Нажмите Enter для возврата в меню...")
        elif action == 'Полное удаление':
            if not uninstall_project():
                print("Ошибка при удалении")
                input("Нажмите Enter для возврата в меню...")
                continue
            print("\n=== Удаление успешно завершено! ===")
            input("Нажмите Enter для возврата в меню...")
        elif action == 'Обновление':
            if not update():
                print("Ошибка при обновлении")
                input("Нажмите Enter для возврата в меню...")
                continue
            print("\n=== Обновление успешно завершено! ===")
            input("Нажмите Enter для возврата в меню...")
        elif action == 'Выход':
            show_farewell()
            break

def show_farewell():
    clear_screen()
    farewell_logo = r"""
く__,.ヘヽ.　　　　/　,ー､ 〉
　　　　　＼ ', !-─‐-i　/　/´
　　　 　 ／｀ｰ'　　　 L/／｀ヽ､
　　 　 /　 ／,　 /|　 ,　 ,　　　 ',
　　　ｲ 　/ /-‐/　ｉ　L_ ﾊ ヽ!　 i
　　　 ﾚ ﾍ 7ｲ｀ﾄ　 ﾚ'ｧ-ﾄ､!ハ|　 |
　　　　 !,/7 '0'　　 ´0iソ| 　 |　　　
　　　　 |.从"　　_　　 ,,,, / |./ 　 |
　　　　 ﾚ'| i＞.､,,__　_,.イ / 　.i 　|
　　　　　 ﾚ'| | / k_７_/ﾚ'ヽ,　ﾊ.　|
　　　　　　 | |/i 〈|/　 i　,.ﾍ |　i　|
　　　　　　.|/ /　ｉ： 　 ﾍ!　　＼　|
　　　 　 　 kヽ>､ﾊ 　 _,.ﾍ､ 　 /､!
　　　　　　 !'〈//｀Ｔ´', ＼ ｀'7'ｰr'
　　　　　　 ﾚ'ヽL__|___i,___,ンﾚ|ノ
　　　　　 　　　ﾄ-,/　|___./
　　　　　 　　　'ｰ'　　!_,.: 
"""
    width = os.get_terminal_size().columns
    for line in farewell_logo.splitlines():
        print(line.center(width))
    print("\n" + "До свидания!".center(width) + "\n")
    time.sleep(5)
    # Закрыть окно PowerShell (только для Windows)
    if os.name == 'nt':
        os.system('exit')
    else:
        os.system('kill -9 $$')

def main():
    print("=== GraphBaseTool - Установка и настройка ===")
    
    # Проверяем версию Python
    if sys.version_info < (3, 8):
        print("Требуется Python 3.8 или выше")
        return False
    
    # Проверяем операционную систему
    if platform.system() != "Windows":
        print("В данный момент поддерживается только Windows")
        return False
    
    show_menu()
    return True

# Получаем путь к CMake
def get_cmake_path():
    if sys.platform == "win32":
        return "cmake"
    return "cmake"

# Класс для запуска тестов
class TestCommand(Command):
    description = "Run tests"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        # Создаем директорию для сборки
        if not os.path.exists("build"):
            os.makedirs("build")
        
        # Запускаем CMake
        cmake_cmd = [get_cmake_path(), ".."]
        subprocess.run(cmake_cmd, cwd="build", check=True)
        
        # Собираем проект
        build_cmd = [get_cmake_path(), "--build", ".", "--config", "Release"]
        subprocess.run(build_cmd, cwd="build", check=True)
        
        # Запускаем тесты
        test_exe = os.path.join("build", "bin", "Release", "graph_test.exe" if sys.platform == "win32" else "graph_test")
        subprocess.run([test_exe], check=True)
        
        perf_exe = os.path.join("build", "bin", "Release", "performance_test.exe" if sys.platform == "win32" else "performance_test")
        subprocess.run([perf_exe], check=True)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        if not main():
            sys.exit(1)
    else:
        setup(
            name="GraphBaseTool",
            version="1.0.0",
            packages=find_packages(),
            install_requires=[
                "boost",
            ],
            python_requires=">=3.7",
            cmdclass={
                "test": TestCommand,
            },
            author="HateByFate",
            author_email="hatebyfate@gmail.com",
            description="A graph processing library with various algorithms",
            long_description=open("README.md", encoding='utf-8').read(),
            long_description_content_type="text/markdown",
            url="https://github.com/HateByFate/GraphBaseTool",
            classifiers=[
                "Development Status :: 4 - Beta",
                "Intended Audience :: Developers",
                "License :: OSI Approved :: MIT License",
                "Programming Language :: C++",
                "Topic :: Software Development :: Libraries :: C++ Modules",
            ],
        ) 
