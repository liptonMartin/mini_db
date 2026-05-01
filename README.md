### Задание

Описание задания лежит в `docs/requirements.pdf`

### Архитектура приложения

![Архитектура приложения](docs/Application%20architecture.jpg)

### Сборка и запуск

Проект собирается с помощью `cmake` и `vcpkg`.

Убедитесь, что у вас установлены эти инструменты. В случае,
если они не установлены, то воспользуйтесь официальными инструкциями
для установки.

Перед сборкой, нам необходимо узнать, где находится файл vcpkg. 

Если вы используете Windows, то один из способов это в PowerShell
ввести такую команду:
```bash
dir -Path C:\ -Filter vcpkg.cmake -Recurse -ErrorAction SilentlyContinue -Force | Select-Object -ExpandProperty FullName
```

#### Терминал 
Для запуска через терминал, используйте следующие команды
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/путь/к/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

Для удобства запуска можно использовать IDE.

#### Clion

File | Settings | Build, Execution, Deployment | CMake

В поле `CMake Options` добавить:
```bash
-DCMAKE_TOOLCHAIN_FILE=/путь/к/vcpkg/scripts/buildsystems/vcpkg.cmake
```


