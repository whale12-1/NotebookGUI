#include <fstream>
#include <iostream>
#include "FileManager.h"
#include "json.hpp"// <-- Подключаем библиотеку

#ifdef _WIN32
#include <windows.h>
#endif

// Создаем удобный синоним (alias), чтобы не писать длинное nlohmann::json
using json = nlohmann::json;

FileManager::FileManager(const std::string& filePath) : m_FilePath(filePath) {}

bool FileManager::SaveSession(const std::vector<Note>& UserNotes,       // Добавили ссылку & для скорости
    const std::string& EncryptedDEK,
    const std::string& Salt,
    std::string& expectedMerkleRoot,
    const std::string& DEKHash
)
{
    std::string tempFilename = "data.tmp";
    std::string targetFilename = m_FilePath;

    // 1. ПРОВЕРКА НА ПОДМЕНУ: Читаем старый JSON с диска, если он есть
    std::ifstream checkFile(targetFilename);
    if (checkFile.is_open()) {
        json diskJson;
        try {
            checkFile >> diskJson; // Пробуем распарсить старый файл

            // Достаем старый хэш по ключу
            std::string diskMerkleRoot = diskJson.at("expected_merkle_root").get<std::string>();

            if (diskMerkleRoot != expectedMerkleRoot) {
                checkFile.close();
                return false; // Защита сработала! Файл подменен.
            }
        }
        catch (...) {
            // Если файл пустой, поврежден или там не JSON — не блокируем, 
            // так как это может быть первый запуск приложения
        }
        checkFile.close();
    }
    std::string newMerkleRoot = MerkleRoot(UserNotes);

    // 2. ФОРМИРУЕМ НОВЫЙ JSON
    json fileJson;
    fileJson["salt"] = Salt;
    fileJson["encrypted_dek"] = EncryptedDEK;
    fileJson["expected_merkle_root"] = newMerkleRoot;
    fileJson["DEK hash"] = DEKHash;
    fileJson["notes"] = UserNotes; // Магия: библиотека сама вызовет to_json для каждой заметки!

    // 3. ЗАПИСЬ ВО ВРЕМЕННЫЙ ФАЙЛ
    std::ofstream file(tempFilename);
    if (!file.is_open()) return false;

    // dump(4) форматирует текст с отступами в 4 пробела, чтобы файл было легко читать глазами
    file << fileJson.dump(4);
    file.close();

    if (file.fail()) {
        std::remove(tempFilename.c_str());
        return false;
    }

    // 4. АТОМАРНОЕ ПЕРЕИМЕНОВАНИЕ
#ifdef _WIN32
    if (MoveFileExA(tempFilename.c_str(), targetFilename.c_str(), MOVEFILE_REPLACE_EXISTING) == 0) {
        std::remove(tempFilename.c_str());
        return false;
    }
#else
    if (std::rename(tempFilename.c_str(), targetFilename.c_str()) != 0) {
        std::remove(tempFilename.c_str());
        return false;
    }
#endif

    // Обновляем хэш в памяти MainWindow
    expectedMerkleRoot = newMerkleRoot;
    return true;
}

bool FileManager::LoadSession(
    std::vector<Note>& UserNotes,
    std::string& EncryptedDEK,
    std::string& PreviousHash,
    std::string& Salt,
    std::string& DEKHash
) {
    std::ifstream file(m_FilePath);
    if (!file.is_open()) return false;

    json fileJson;
    try {
        file >> fileJson; // Читаем структуру целиком
    }
    catch (const json::parse_error& e) {
        // Если файл забит мусором или поврежден — корректно выходим
        file.close();
        return false;
    }
    file.close();

    try {
        // Извлекаем данные по ключам
        Salt = fileJson.at("salt").get<std::string>();
        EncryptedDEK = fileJson.at("encrypted_dek").get<std::string>();
        PreviousHash = fileJson.at("expected_merkle_root").get<std::string>();
        DEKHash = fileJson.at("DEK hash").get<std::string>();
        // Извлекаем массив заметок. Благодаря функции from_json вектор заполнится сам!
        UserNotes = fileJson.at("notes").get<std::vector<Note>>();
    }
    catch (const json::out_of_range& e) {
        // Если в JSON не хватает какого-то обязательного поля
        UserNotes.clear();
        return false;
    }

    // Проверяем целостность данных
    if (PreviousHash != MerkleRoot(UserNotes)) {
        UserNotes.clear();
        return false;
    }

    return true;
}