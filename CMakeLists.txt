cmake_minimum_required(VERSION 3.16)
project(NoteBookGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Заставляем MinGW принудительно читать файлы в UTF-8, даже если они сохранены в ANSI
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# Твой путь к Qt6
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/mingw_64")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

# --- ПРАВИЛЬНЫЙ ПОИСК OPENSSL ДЛЯ WINDOWS ---
# Задаем базовую директорию установки
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")

# Используем встроенный поиск CMake, подсказывая ему точные пути (HINTS)
find_package(OpenSSL REQUIRED HINTS "${OPENSSL_ROOT_DIR}" PATHS "${OPENSSL_ROOT_DIR}")

add_executable(NoteBookGUI 
    NoteBookGUI.cpp 
    CryptoManager.cpp
    CryptoManager.h
    MainWindow.h
    MainWindow.cpp 
    CreateNoteDialog.h
    CreateNoteDialog.cpp 
    FileManager.h 
    FileManager.cpp 
    DecryptDialog.h 
    DecryptDialog.cpp 
    ChangePass.h 
    ChangePass.cpp 
    AuthWindow.h 
    AuthWindow.cpp
    SecureString.h
    json.hpp
    NotesModel.h
    resources.qrc
    CustomFilterProxyModel.h 
    CustomFilterProxyModel.cpp
    DatabaseManager.h 
    DatabaseManager.cpp
    ProtectedKey.h
    TwoFactorDialog.h 
    TwoFactorDialog.cpp
    PasswordInput.h    
    PasswordInput.cpp  
    SettingsDialog.h
    SettingsDialog.cpp
    WinThemeUtils.h
    WinThemeUtils.cpp
    ModernMessage.h
    SyncManager.h 
    SyncManager.cpp
)

# Линкуем Qt6
target_link_libraries(NoteBookGUI PRIVATE 
    Qt6::Widgets 
    Qt6::Sql 
    Qt6::Network
    OpenSSL::SSL 
    OpenSSL::Crypto
)
if(WIN32)
    target_link_libraries(NoteBookGUI PRIVATE dwmapi)
endif()
# Линкуем автоматически найденный OpenSSL
if(OpenSSL_FOUND)
    # Нам нужен только Crypto-модуль для AES
    target_link_libraries(NoteBookGUI PRIVATE OpenSSL::Crypto)
    message(STATUS "OpenSSL Found successfully!")
    message(STATUS "Includes: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "Libraries: ${OPENSSL_CRYPTO_LIBRARY}")
else()
    message(FATAL_ERROR "OpenSSL was not found by CMake standard module")
endif()