#include "FileManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

using json = nlohmann::json;

FileManager::FileManager(const std::string& filePath) : m_FilePath(filePath) {}

// --- ЭКСПОРТ БЭКАПА ---
bool FileManager::SaveBackup(
    const std::vector<Note>& userNotes,
    const std::string& encryptedDEK,
    const std::string& salt,
    const std::string& dekHash
) {
    std::string tempFilename = m_FilePath + ".tmp";
    std::string targetFilename = m_FilePath;

    // Формируем JSON-структуру строго в нужном порядке
    json fileJson;
    fileJson["salt"] = salt;
    fileJson["encrypted_dek"] = encryptedDEK;
    fileJson["dek_hash"] = dekHash;
    fileJson["notes"] = userNotes; // Библиотека автоматически вызовет to_json из CryptoManager.h

    // Запись во временный файл
    std::ofstream file(tempFilename);
    if (!file.is_open()) return false;

    // dump(4) делает JSON человекочитаемым (отступы в 4 пробела)
    file << fileJson.dump(4);
    file.close();

    if (file.fail()) {
        std::remove(tempFilename.c_str());
        return false;
    }

    // Атомарное переименование для предотвращения повреждения файлов
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

    return true;
}

// --- ИМПОРТ БЭКАПА ---
bool FileManager::LoadBackup(
    std::vector<Note>& userNotes,
    std::string& encryptedDEK,
    std::string& salt,
    std::string& dekHash
) {
    std::ifstream file(m_FilePath);
    if (!file.is_open()) return false;

    json fileJson;
    try {
        file >> fileJson; // Парсим файл
    }
    catch (const json::parse_error& e) {
        file.close();
        return false; // Файл поврежден или это не JSON
    }
    file.close();

    try {
        // Извлекаем поля
        salt = fileJson.at("salt").get<std::string>();
        encryptedDEK = fileJson.at("encrypted_dek").get<std::string>();
        dekHash = fileJson.at("dek_hash").get<std::string>();

        // Автоматически конвертируем массив JSON в вектор заметок через from_json[cite: 4]
        userNotes = fileJson.at("notes").get<std::vector<Note>>();
    }
    catch (const json::out_of_range& e) {
        // Если отсутствует какое-то обязательное поле структуры бэкапа
        userNotes.clear();
        return false;
    }

    return true;
}

