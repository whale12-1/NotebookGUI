#pragma once

#include <string>
#include <vector>
#include "CryptoManager.h" // Для структуры Note

class FileManager {
public:
    explicit FileManager(const std::string& filePath);

    // Экспорт (сохранение) бэкапа в зашифрованный JSON
    bool SaveBackup(
        const std::vector<Note>& userNotes,
        const std::string& encryptedDEK,
        const std::string& salt,
        const std::string& dekHash
    );

    // Импорт (загрузка) бэкапа из файла
    bool LoadBackup(
        std::vector<Note>& userNotes,
        std::string& encryptedDEK,
        std::string& salt,
        std::string& dekHash
    );

private:
    std::string m_FilePath;
};