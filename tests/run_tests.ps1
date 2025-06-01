# Копируем тестовые файлы в директорию build
Copy-Item -Path "test_graph.csv" -Destination "../build/"
Copy-Item -Path "test_graph.json" -Destination "../build/"

# Переходим в директорию build
Set-Location -Path "../build"

# Запускаем тесты
.\Debug\graph_test.exe -s 
