# === ЭТАП 1: СБОРКА СЕРВЕРА ===
FROM ubuntu:24.04 AS builder

# Устанавливаем необходимые инструменты для сборки
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Создаем рабочую директорию
WORKDIR /app

# Копируем исходный код сервера в контейнер
COPY . .

# Конфигурируем и собираем проект (под Linux используем GCC/Clang)
# Обрати внимание: FetchContent внутри CMakeLists.txt сам скачает Crow и Asio!
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release

# === ЭТАП 2: МИНИМАЛЬНЫЙ ОБРАЗ ДЛЯ ЗАПУСКА ===
FROM ubuntu:24.04

WORKDIR /app

# Копируем только скомпилированный бинарник из первого этапа
COPY --from=builder /app/build/Server .

# Устанавливаем только runtime-зависимость OpenSSL, необходимую для работы
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Открываем порт, который слушает Crow
EXPOSE 8080

# Команда для запуска сервера
CMD ["./Server"]