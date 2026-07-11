#pragma once

#include <string>
#include <vector>
#include "CryptoManager.h" // Для структуры Note

class FileManager {
public:
    explicit FileManager(const std::string& filePath);

    // Сохранение всей сессии в файл
    bool SaveSession(const std::vector<Note>& UserNotes,       // Добавили ссылку & для скорости
        const std::string& EncryptedDEK,
        const std::string& Salt,
        std::string& expectedMerkleRoot,
        const std::string& DEKHash
    );
    // Загрузка сессии из файла (параметры передаются по ссылкам для заполнения)
    bool LoadSession(
        std::vector<Note>& UserNotes,
        std::string& EncryptedDEK, 
        std::string& PreviousHash, 
        std::string& Salt,
        std::string& DEKHash
    );

private:
    std::string m_FilePath;
};