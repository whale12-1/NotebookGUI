#include "SyncManager.h"
#include <QDebug>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QCoreApplication>

SyncManager& SyncManager::instance() {
    static SyncManager inst;
    // Автоматически инициализируем менеджер при первом обращении к синглтону
    if (!inst.m_networkManager) {
        inst.m_networkManager = new QNetworkAccessManager(&inst);
    }
    return inst;
}


// 1. Авторизация на сервере и получение JWT
void SyncManager::loginToServer(const QString& username, const QString& authHash) {
    // Защита: если вдруг менеджер не создался (хотя теперь это невозможно)
    if (!m_networkManager) {
        emit loginStatusChanged(false, "Критическая ошибка: Сетевой менеджер не инициализирован.");
        return;
    }

    // ВАЖНО: Если адрес "https://api.yourserver.com" тестовый и сервера нет,
    // QNetworkAccessManager мгновенно вернет ошибку (Host NotFound / Connection Refused),
    // и сработает наш блок else ниже, отпустив интерфейс в офлайн-режим.
    QUrl url("http://127.0.0.1:8080/v1/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["username"] = username;
    json["auth_hash"] = authHash;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            m_authToken = resDoc.object()["token"].toString(); // Сохраняем токен
            emit loginStatusChanged(true, "Авторизация успешна.");
        }
        else {
            emit loginStatusChanged(false, QString("Ошибка авторизации: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
        });
}

// 2. Синхронизация самого файла базы данных SQLite
void SyncManager::syncDatabase(const QString& sqlitePath) {
    if (!m_networkManager || m_authToken.isEmpty()) {
        emit syncFinished(false, "Нет подключения или авторизации.");
        return;
    }

    QFile file(sqlitePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit syncFinished(false, "Не удалось открыть локальную базу данных.");
        return;
    }
    QByteArray encryptedDbData = file.readAll();
    file.close();

    int localVersion = DatabaseManager::instance().GetDatabaseVersion();

    QUrl url("https://api.yourserver.com/v1/sync/upload-db");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    request.setRawHeader("X-DB-Version", QByteArray::number(localVersion));

    QNetworkReply* reply = m_networkManager->post(request, encryptedDbData);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "База данных успешно загружена на сервер.";

            // Сразу после базы запускаем "умную" синхронизацию файлов
            std::vector<Note> notes = DatabaseManager::instance().getAllNotes();
            this->checkAndUploadMissingFiles(notes);
            this->cleanOrphanedFilesOnServer(notes);
            emit syncFinished(true, "Синхронизация завершена успешно.");
        }
        else {
            emit syncFinished(false, QString("Ошибка синхронизации базы: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
        });
}

// 3. Умная проверка и пофайловая загрузка вложений
void SyncManager::checkAndUploadMissingFiles(const std::vector<Note>& notesList) {
    if (!m_networkManager || m_authToken.isEmpty()) return;

    QJsonArray hashArray;
    QMap<QString, QString> hashToFileNameMap;

    // Собираем отпечатки всех локальных файлов
    for (const auto& note : notesList) {
        for (const auto& attach : note.Attachments) {
            QString fileHash = QString::fromStdString(attach.FileHash);
            QString fileName = QString::fromStdString(attach.DiskFileName);

            if (!fileHash.isEmpty()) {
                hashArray.append(fileHash);
                hashToFileNameMap[fileHash] = fileName;
            }
        }
    }

    if (hashArray.isEmpty()) return; // Файлов нет — проверять нечего

    QJsonObject jsonReq;
    jsonReq["hashes"] = hashArray;

    QUrl url("https://api.yourserver.com/v1/sync/check-files");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(jsonReq).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, hashToFileNameMap]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray missingHashes = resDoc.object()["missing_hashes"].toArray();

            // Запускаем отправку ТОЛЬКО тех файлов, хэшей которых нет в PostgreSQL
            for (int i = 0; i < missingHashes.size(); ++i) {
                QString targetHash = missingHashes[i].toString();
                if (hashToFileNameMap.contains(targetHash)) {
                    QString localFileName = hashToFileNameMap[targetHash];
                    this->uploadSingleFile(localFileName);
                }
            }
        }
        else {
            qDebug() << "Ошибка проверки уникальности файлов:" << reply->errorString();
        }
        reply->deleteLater();
        });
}

// 4. Низкоуровневая отправка одного бинарного файла
void SyncManager::uploadSingleFile(const QString& fileName) {
    QString fullPath = QCoreApplication::applicationDirPath() + "/attachments/" + fileName;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Критическая ошибка: Не удалось открыть файл для загрузки:" << fullPath;
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QUrl url("https://api.yourserver.com/v1/sync/upload-file");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    // Сервер должен знать, под каким именем сохранить этот кусок данных
    request.setRawHeader("X-File-Name", QFileInfo(fileName).fileName().toUtf8());

    QNetworkReply* reply = m_networkManager->post(request, fileData);

    connect(reply, &QNetworkReply::finished, [reply, fileName]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Файл успешно загружен в облако:" << fileName;
        }
        else {
            qDebug() << "Ошибка загрузки файла:" << fileName << reply->errorString();
        }
        reply->deleteLater();
        });
}

void SyncManager::cleanOrphanedFilesOnServer(const std::vector<Note>& notesList) {
    if (!m_networkManager || m_authToken.isEmpty()) return;

    // 1. Собираем хэши всех файлов, которые РЕАЛЬНО существуют в локальной базе
    QSet<QString> localHashes;
    for (const auto& note : notesList) {
        for (const auto& attach : note.Attachments) {
            if (!attach.FileHash.empty()) {
                localHashes.insert(QString::fromStdString(attach.FileHash));
            }
        }
    }

    // 2. Запрашиваем у сервера список ВСЕХ файлов, что у него хранятся
    QUrl url("https://api.yourserver.com/v1/sync/list-cloud-hashes");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply, localHashes]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray cloudHashes = resDoc.object()["cloud_hashes"].toArray();

            QJsonArray hashesToDelete;

            // 3. Сверяем: если файл есть на сервере, но его НЕТ в локальной базе — в утиль!
            for (int i = 0; i < cloudHashes.size(); ++i) {
                QString cloudHash = cloudHashes[i].toString();
                if (!localHashes.contains(cloudHash)) {
                    hashesToDelete.append(cloudHash);
                }
            }

            // 4. Если нашли сирот — просим сервер их удалить
            if (!hashesToDelete.isEmpty()) {
                this->deleteFilesFromServer(hashesToDelete);
            }
        }
        reply->deleteLater();
        });
}

// Низкоуровневый запрос на удаление
void SyncManager::deleteFilesFromServer(const QJsonArray& hashesToDelete) {
    QUrl url("https://api.yourserver.com/v1/sync/delete-files");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());

    QJsonObject jsonReq;
    jsonReq["hashes"] = hashesToDelete;

    // HTTP DELETE запрос с телом данных (в Qt делается через sendCustomRequest)
    QNetworkReply* reply = m_networkManager->sendCustomRequest(request, "DELETE", QJsonDocument(jsonReq).toJson());

    connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Удаленные файлы успешно стёрты из облака.";
        }
        else {
            qDebug() << "Ошибка очистки облака:" << reply->errorString();
        }
        reply->deleteLater();
        });
}