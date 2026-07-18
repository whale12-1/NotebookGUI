cmake_minimum_required(VERSION 3.16)

# Принудительно заставляем CMake использовать родной компилятор MSVC на Windows
if(WIN32)
    set(CMAKE_CXX_COMPILER "cl.exe")
    set(CMAKE_C_COMPILER "cl.exe")
endif()

project(Server VERSION 1.0 LANGUAGES CXX)

# Устанавливаем стандарт C++ (для сервера возьмем тот же или выше)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------
# 1. СКАЧИВАНИЕ И ПОДКЛЮЧЕНИЕ CROW ИЗ GITHUB
# -------------------------------------------------------------------
include(FetchContent)

# 1. Сначала декларируем и скачиваем Asio
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG master
)
FetchContent_MakeAvailable(asio)

# ЖЕСТКИЙ ХАК ДЛЯ CROW: 
# Вычисляем, куда CMake скачал Asio, и прописываем путь в кэш ДО загрузки Crow.
# У репозитория Asio заголовки лежат в папке: [путь_к_сорцам]/include
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/include" CACHE PATH "Path to Asio headers" FORCE)

# 2. Декларируем и скачиваем Crow
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG master
)
# Теперь Crow при вызове find_package(asio) сразу увидит готовую переменную в кэше
FetchContent_MakeAvailable(Crow)
# -------------------------------------------------------------------
# 2. ПРОВЕРЕННЫЙ ПОИСК OPENSSL (КАК В ОСНОВНОМ ПРОЕКТЕ)
# -------------------------------------------------------------------
# -------------------------------------------------------------------
# 2. ЯВНОЕ ПОДКЛЮЧЕНИЕ OPENSSL (БЕЗ FIND_PACKAGE)
# -------------------------------------------------------------------
set(OPENSSL_ROOT_DIR "C:/OpenSSL-Win64")

# Указываем точный путь к заголовочным файлам
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")

# Указываем точные пути к файлам библиотек для линковки MSVC
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libcrypto.lib")
set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libssl.lib")

# Если вдруг папки /VC/x64/MD/ внутри /lib/ нет (зависит от сборки OpenSSL), 
# CMake возьмет их напрямую из папки /lib/:
if(NOT EXISTS "${OPENSSL_CRYPTO_LIBRARY}")
    set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libcrypto.lib")
    set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libssl.lib")
endif()

# Принудительно подключаем заголовки OpenSSL к проекту
include_directories(${OPENSSL_INCLUDE_DIR})

# Для отладки выведем пути в лог
message(STATUS "=== [SERVER] Using OpenSSL Headers: ${OPENSSL_INCLUDE_DIR} ===")
message(STATUS "=== [SERVER] Using OpenSSL Crypto Lib: ${OPENSSL_CRYPTO_LIBRARY} ===")
message(STATUS "=== [SERVER] Using OpenSSL SSL Lib: ${OPENSSL_SSL_LIBRARY} ===")

# -------------------------------------------------------------------
# 3. ОБЪЯВЛЕНИЕ ИСПОЛНЯЕМОГО ФАЙЛА СЕРВЕРА
# -------------------------------------------------------------------
# Оставь этот вызов ТУТ и убедись, что ниже на строке 88 его больше нет!
add_executable(Server main.cpp)

# -------------------------------------------------------------------
# 4. ЛИНКОВКА БИБЛИОТЕК К СЕРВЕРУ
# -------------------------------------------------------------------
target_link_libraries(Server PRIVATE 
    Crow::Crow 
    "${OPENSSL_CRYPTO_LIBRARY}"
    "${OPENSSL_SSL_LIBRARY}"
)

# Старую проверку if(OpenSSL_FOUND) либо удали, либо замени на эту:
if(EXISTS "${OPENSSL_INCLUDE_DIR}")
    message(STATUS "=== [SERVER] All targets configured successfully! ===")
else()
    message(FATAL_ERROR "=== [SERVER] Configuration failed ===")
endif()

# -------------------------------------------------------------------
# 5. АВТОКОПИРОВАНИЕ DLL OPENSSL В ПАПКУ С EXEC-ФАЙЛОМ
# -------------------------------------------------------------------
# Определяем пути к DLL файлам в установке OpenSSL
set(OPENSSL_BIN_DIR "${OPENSSL_ROOT_DIR}/bin")

# Говорим CMake копировать DLL после успешной сборки
add_custom_command(TARGET Server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${OPENSSL_BIN_DIR}/libcrypto-4-x64.dll"
    "${OPENSSL_BIN_DIR}/libssl-4-x64.dll"
    $<TARGET_FILE_DIR:Server>
)

if(MSVC)
    add_compile_options(/utf-8)
endif()