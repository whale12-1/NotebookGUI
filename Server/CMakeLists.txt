cmake_minimum_required(VERSION 3.16)

# Принудительно заставляем CMake использовать родной компилятор MSVC только на Windows
if(WIN32)
    set(CMAKE_CXX_COMPILER "cl.exe")
    set(CMAKE_C_COMPILER "cl.exe")
endif()

project(Server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------
# 1. СКАЧИВАНИЕ И ПОДКЛЮЧЕНИЕ CROW ИЗ GITHUB
# -------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG master
)
FetchContent_MakeAvailable(asio)

set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/include" CACHE PATH "Path to Asio headers" FORCE)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Crow)

# -------------------------------------------------------------------
# 2. ПОДКЛЮЧЕНИЕ OPENSSL (РАЗДЕЛЬНОЕ ДЛЯ WIN/LINUX)
# -------------------------------------------------------------------
if(WIN32)
    # Твоя проверенная конфигурация для Windows
    set(OPENSSL_ROOT_DIR "C:/OpenSSL-Win64")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libcrypto.lib")
    set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libssl.lib")

    if(NOT EXISTS "${OPENSSL_CRYPTO_LIBRARY}")
        set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libcrypto.lib")
        set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libssl.lib")
    endif()

    include_directories(${OPENSSL_INCLUDE_DIR})
else()
    # Конфигурация для Linux (Docker) — находим OpenSSL автоматически в системе
    find_package(OpenSSL REQUIRED)
    set(OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
    set(OPENSSL_CRYPTO_LIBRARY OpenSSL::Crypto)
    set(OPENSSL_SSL_LIBRARY OpenSSL::SSL)
endif()

# Для отладки выведем пути в лог
message(STATUS "=== [SERVER] Using OpenSSL Headers: ${OPENSSL_INCLUDE_DIR} ===")

# -------------------------------------------------------------------
# 3. ОБЪЯВЛЕНИЕ ИСПОЛНЯЕМОГО ФАЙЛА СЕРВЕРА И ЛИНКОВКА
# -------------------------------------------------------------------
add_executable(Server main.cpp)

target_link_libraries(Server PRIVATE 
    Crow::Crow 
    "${OPENSSL_CRYPTO_LIBRARY}"
    "${OPENSSL_SSL_LIBRARY}"
)

# -------------------------------------------------------------------
# 4. АВТОКОПИРОВАНИЕ DLL OPENSSL (ТОЛЬКО ДЛЯ WINDOWS)
# -------------------------------------------------------------------
if(WIN32)
    set(OPENSSL_BIN_DIR "${OPENSSL_ROOT_DIR}/bin")

    add_custom_command(TARGET Server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENSSL_BIN_DIR}/libcrypto-4-x64.dll"
        "${OPENSSL_BIN_DIR}/libssl-4-x64.dll"
        $<TARGET_FILE_DIR:Server>
    )
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()