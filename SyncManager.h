#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "DatabaseManager.h" // Нужен для получения структуры заметок

class SyncManager : public QObject {
    Q_OBJECT

public:
    // Получение единственного экземпляра класса (Singleton)
    static SyncManager& instance();

    // Главные методы API
    void loginToServer(const QString& username, const QString& authHash);
    void syncDatabase(const QString& sqlitePath);
    void checkAndUploadMissingFiles(const std::vector<Note>& notesList);
    void cleanOrphanedFilesOnServer(const std::vector<Note>& notesList);
    void deleteFilesFromServer(const QJsonArray& hashesToDelete);
signals:
    // Сигналы для UI, чтобы показывать прогресс или ошибки
    void loginStatusChanged(bool success, QString message);
    void syncFinished(bool success, QString message);
    void fileUploadProgress(QString fileName, int current, int total);

private:
    SyncManager() = default;
    ~SyncManager() = default;
    SyncManager(const SyncManager&) = delete;
    SyncManager& operator=(const SyncManager&) = delete;

    // Вспомогательные методы отправки файлов
    void uploadSingleFile(const QString& fileName);

    QNetworkAccessManager* m_networkManager = nullptr;
    QString m_authToken = ""; // Храним JWT-токен сессии в памяти
};