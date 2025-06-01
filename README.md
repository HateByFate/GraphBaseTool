# GraphBaseTool

Библиотека для работы с графами, реализующая различные алгоритмы поиска кратчайших путей и другие операции с графами.

## Возможности

- Создание и управление графами
- Алгоритм Дейкстры для поиска кратчайших путей
- Алгоритм Беллмана-Форда для работы с отрицательными весами
- Алгоритм Флойда-Уоршелла для поиска кратчайших путей между всеми парами вершин
- Параллельная реализация алгоритма Флойда-Уоршелла
- Поиск отрицательных циклов
- Модульные тесты
- Тесты производительности

## Требования

- C++17
- CMake 3.15 или выше
- Boost
- Python 3.8 или выше (для setup.py)
- Git (для обновления)

## Установка

### Автоматическая установка (рекомендуется)

Для автоматической установки используйте скрипт `setup.py`:

```bash
python setup.py
```

Скрипт предоставляет интерактивное меню со следующими опциями:

1. **Полная установка**
   - Проверяет и устанавливает vcpkg (если не установлен)
   - Устанавливает необходимые зависимости (Boost)
   - Собирает проект
   - Запускает тесты для проверки корректности установки

2. **Тесты производительности**
   - Запускает все бенчмарки
   - Создает папку `results` с результатами
   - Генерирует графики и отчеты

3. **Полное удаление**
   - Удаляет все скомпилированные файлы
   - Удаляет результаты тестов
   - Очищает временные файлы

4. **Обновление**
   - Проверяет наличие обновлений на GitHub
   - Скачивает и устанавливает последнюю версию
   - Пересобирает проект

5. **Выход**
   - Завершает работу скрипта

### Ручная установка

Если вы предпочитаете установку вручную, выполните следующие шаги:

1. Установите [vcpkg](https://github.com/microsoft/vcpkg):
```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat  # для Windows
```

2. Установите необходимые зависимости:
```bash
vcpkg install boost:x64-windows
vcpkg integrate install
```

3. Клонируйте репозиторий:
```bash
git clone https://github.com/HateByFate/GraphBaseTool.git
cd GraphBaseTool
```

4. Соберите проект:
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Использование

```cpp
#include <GraphBaseTool/graph.hpp>

int main() {
    routing::Graph graph;
    
    // Добавляем вершины
    size_t v1 = graph.add_vertex();
    size_t v2 = graph.add_vertex();
    
    // Добавляем ребро
    graph.add_edge({v1, v2, 1.0});
    
    // Находим кратчайший путь
    auto path = graph.dijkstra_shortest_path(v1, v2);
    
    return 0;
}
```

## Запуск тестов

```bash
python setup.py test
```

## Структура проекта

```
GraphBaseTool/
├── include/
│   └── GraphBaseTool/
│       └── graph.hpp
├── src/
│   └── graph.cpp
├── tests/
│   ├── graph_test.cpp
│   └── performance_test.cpp
├── benchmarks/
│   └── run_benchmarks.py
├── CMakeLists.txt
├── setup.py
├── requirements.txt
└── README.md
```

## Лицензия

MIT License

