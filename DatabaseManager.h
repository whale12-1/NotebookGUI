#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVector>
#include "SecureString.h"
#include "CryptoManager.h" // Нужна для структуры Note

class DatabaseManager {
public:
    // Получение единственного экземпляра класса (Паттерн Синглтон)
    static DatabaseManager& instance();

    // Открытие базы данных и создание таблиц
    bool initDatabase(const QString& dbName = "safe_data.db");

    // Закрытие соединения
    void closeDatabase();
    //Функции работы с 2FA
    bool Is2FAEnabled();

    bool Enable2FA(const std::string& encryptedSecret);

    bool Disable2FA();

    // Получить зашифрованный секрет из базы данных
    std::string GetEncrypted2FASecret();
    int GetDatabaseVersion();
    // --- ФУНКЦИИ ДЛЯ РАБОТЫ С ЗАМЕТКАМИ ---
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    bool InsertNoteRaw(Note& note);

    bool IncrementDatabaseVersion();
    bool ValidateIntegrity();
    bool LoadCryptoKeys(QString& MasterSalt, QString& EncryptedDek, QString& DekHash);
    bool SaveCryptoKeys(const QString& MasterSalt, const QString& EncryptedDek, const QString& DekHash);
    bool HasCryptoKeys();
    bool AddNote(Note& note);
    bool RemoveNote(long long noteId);
    // Функция получения всех заметок (пока в вектор, для плавного перехода)
    std::vector<Note> getAllNotes();

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
};