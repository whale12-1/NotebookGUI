#include "DatabaseManager.h"
#include <QDebug>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::initDatabase(const QString& dbName) {
    // Подключаем драйвер SQLite
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qDebug() << "Ошибка открытия БД:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;

    // Включаем поддержку внешних ключей (по умолчанию в SQLite она отключена!)
    query.exec("PRAGMA foreign_keys = ON;");

    // Создаем таблицу заметок
    QString createNotesTable =
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "secured_text TEXT NOT NULL, "
        "previous_hash TEXT" // Добавили колонку в БД
        ");";

    if (!query.exec(createNotesTable)) {
        qDebug() << "Ошибка создания таблицы notes:" << query.lastError().text();
        return false;
    }

    // Создаем таблицу тегов с КАСКАДНЫМ удалением
    QString createTagsTable =
        "CREATE TABLE IF NOT EXISTS tags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "note_id INTEGER NOT NULL, "
        "tag_name TEXT NOT NULL, "
        "FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE"
        ");";

    if (!query.exec(createTagsTable)) {
        qDebug() << "Ошибка создания таблицы tags:" << query.lastError().text();
        return false;
    }
    // Добавляем внутри DatabaseManager::InitDatabase()
    QString CreateSystemTable =
        "CREATE TABLE IF NOT EXISTS SystemInfo ("
        "Id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "MasterSalt TEXT NOT NULL, "
        "EncryptedDek TEXT NOT NULL, "
        "DekHash TEXT NOT NULL, "
        "Is2FAEnabled INTEGER DEFAULT 0, "
        "Secret2FA TEXT DEFAULT '', "
        "Version INTEGER DEFAULT 1"  // <-- НАША НОВАЯ КОЛОНКА
        ");";

    if (!query.exec(CreateSystemTable)) {
        qDebug() << "Ошибка создания таблицы SystemInfo:" << query.lastError().text();
        return false;
    }

    QString CreateAttachmentsTable =
        "CREATE TABLE IF NOT EXISTS attachments("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "note_id INTEGER NOT NULL,"
        "original_name_encrypted TEXT NOT NULL, "
        "disk_filename TEXT NOT NULL,"
        "iv BLOB NOT NULL,"
        "file_size INTEGER NOT NULL,"
        "file_hash TEXT NOT NULL DEFAULT ''," // <-- ДОБАВИЛИ КОЛОНКУ
        "FOREIGN KEY(note_id) REFERENCES notes(id) ON DELETE CASCADE"
        ");";

    if (!query.exec(CreateAttachmentsTable)) {
        qDebug() << "Ошибка создания таблицы Attachments:" << query.lastError().text();
        return false;
    }

    return true;
}

void DatabaseManager::closeDatabase() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}
bool DatabaseManager::ValidateIntegrity() {
    QSqlQuery Query;

    // Считываем всё строго по порядку увеличения ID
    if (!Query.exec("SELECT name, secured_text, previous_hash FROM notes ORDER BY id ASC;")) {
        return false;
    }

    std::string ExpectedHash = ""; // Для самой первой записи ждем ""

    while (Query.next()) {
        std::string Name = Query.value(0).toString().toStdString();
        std::string SecuredText = Query.value(1).toString().toStdString();
        std::string PreviousHash = Query.value(2).toString().toStdString();

        // Проверяем: совпадает ли то, что записано в базе, с тем, что мы насчитали
        if (PreviousHash != ExpectedHash) {
            qDebug() << "КРИТИЧЕСКАЯ ОШИБКА: Цепочка хэшей разорвана!";
            return false; // Целостность нарушена! Кто-то ковырялся в файле базы
        }

        // Вычисляем, какой хэш должен быть у СЛЕДУЮЩЕЙ заметки
        std::string DataToHash = Name + SecuredText + PreviousHash;
        ExpectedHash = CalculateSHA256(DataToHash);
    }

    return true; // Всё идеально, база данных чиста и валидна
}
// Обрати внимание: теперь принимаем не const Note&, а просто Note&
// DatabaseManager.cpp

// 1. Низкоуровневая вставка (единственное место с SQL-запросом INSERT)
bool DatabaseManager::InsertNoteRaw(Note& note) {
    QSqlQuery Query;

    Query.prepare("INSERT INTO notes (name, secured_text, previous_hash) "
        "VALUES (:Name, :Text, :PrevHash);");
    Query.bindValue(":Name", QString::fromStdString(note.Name));
    Query.bindValue(":Text", QString::fromStdString(note.SecuredText));
    Query.bindValue(":PrevHash", QString::fromStdString(note.PreviousHash));

    if (!Query.exec()) {
        qDebug() << "InsertNoteRaw error (notes):" << Query.lastError().text();
        return false;
    }

    // Запоминаем сгенерированный БД идентификатор
    note.Id = Query.lastInsertId().toLongLong();
    Query.prepare("INSERT INTO tags (note_id, tag_name) VALUES (:note_id, :tag_name);");
    for (const auto& tag : note.Tags) {
        Query.bindValue(":note_id", note.Id);
        Query.bindValue(":tag_name", QString::fromStdString(tag));
        if (!Query.exec()) {
            qDebug() << "InsertNoteRaw error (tags):" << Query.lastError().text();
            return false;
        }
    }
    Query.prepare("INSERT INTO attachments (note_id, original_name_encrypted, disk_filename, iv, file_size, file_hash) "
        "VALUES (:note_id, :orig_name, :disk_name, :iv, :file_size, :file_hash);");
    for (const auto& attach : note.Attachments) {
        Query.bindValue(":note_id", note.Id);
        Query.bindValue(":orig_name", QString::fromStdString(attach.OriginalNameEncrypted));
        Query.bindValue(":disk_name", QString::fromStdString(attach.DiskFileName));

        // Преобразуем вектор байт IV в QByteArray для вставки в BLOB поле БД
        QByteArray ivBytes(reinterpret_cast<const char*>(attach.Iv.data()), attach.Iv.size());
        Query.bindValue(":iv", ivBytes);

        Query.bindValue(":file_size", attach.FileSize);
        Query.bindValue(":file_hash", QString::fromStdString(attach.FileHash));
        if (!Query.exec()) {
            qDebug() << "InsertNoteRaw error (attachments):" << Query.lastError().text();
            return false;
        }
    }
    return true;
}

// 2. Высокоуровневый метод для добавления заметок пользователем вручную
bool DatabaseManager::AddNote(Note& note) {
    if (!beginTransaction()) return false;
    // Узнаем хэш последней записи на диске
    std::string LastNoteHash = "";
    QSqlQuery Query;

    if (Query.exec("SELECT name, secured_text, previous_hash FROM notes ORDER BY id DESC LIMIT 1;") && Query.next()) {
        std::string PrevName = Query.value(0).toString().toStdString();
        std::string PrevText = Query.value(1).toString().toStdString();
        std::string PrevRootHash = Query.value(2).toString().toStdString();

        std::string DataToHash = PrevName + PrevText + PrevRootHash;
        LastNoteHash = CalculateSHA256(DataToHash);
    }

    // Записываем хэш в наш объект
    note.PreviousHash = LastNoteHash;

    // Просто вызываем наш низкоуровневый метод для фактической записи!
    if (!InsertNoteRaw(note)) {
        rollbackTransaction(); // Откатываемся в случае ошибки[cite: 5]
        return false;
    }
    if (!IncrementDatabaseVersion()) {
        qDebug() << "Cloud database error";
        rollbackTransaction();
        return false;
    }

    return commitTransaction(); // Фиксируем изменения[cite: 5]
}
bool DatabaseManager::HasCryptoKeys() {
    QSqlQuery Query;
    if (Query.exec("SELECT COUNT(*) FROM SystemInfo;") && Query.next()) {
        return Query.value(0).toInt() > 0;
    }
    return false;
}
bool DatabaseManager::LoadCryptoKeys(QString& MasterSalt, QString& EncryptedDek, QString& DekHash) {
    QSqlQuery Query;
    if (Query.exec("SELECT MasterSalt, EncryptedDek, DekHash FROM SystemInfo LIMIT 1;") && Query.next()) {
        MasterSalt = Query.value(0).toString();
        EncryptedDek = Query.value(1).toString();
        DekHash = Query.value(2).toString();
        return true;
    }
    return false;
}
bool DatabaseManager::SaveCryptoKeys(const QString& MasterSalt, const QString& EncryptedDek, const QString& DekHash) {
    QSqlQuery Query;

    // Сначала проверяем, есть ли уже запись в таблице
    if (HasCryptoKeys()) {
        // Если запись есть, обновляем СТРОГО ключи шифрования, не трогая настройки 2FA!
        Query.prepare("UPDATE SystemInfo SET MasterSalt = :MasterSalt, EncryptedDek = :EncryptedDek, DekHash = :DekHash;");
    }
    else {
        // Если база чистая (первый запуск), создаем запись с дефолтными 2FA (0 и '')
        Query.prepare("INSERT INTO SystemInfo (MasterSalt, EncryptedDek, DekHash) "
            "VALUES (:MasterSalt, :EncryptedDek, :DekHash);");
    }

    Query.bindValue(":MasterSalt", MasterSalt);
    Query.bindValue(":EncryptedDek", EncryptedDek);
    Query.bindValue(":DekHash", DekHash);

    if (!Query.exec()) {
        qDebug() << "Ошибка сохранения крипто-ключей:" << Query.lastError().text();
        return false;
    }
    return true;
}
// Функция теперь принимает четкий ID, который мы берем из note.Id
bool DatabaseManager::RemoveNote(long long noteId) {
    // 1. Открываем транзакцию, чтобы все изменения были атомарными
    if (!m_db.transaction()) return false;

    QSqlQuery query;

    // Шаг A: Удаляем саму заметку (теги удалятся автоматически благодаря ON DELETE CASCADE)
    query.prepare("DELETE FROM notes WHERE id = :id;");
    query.bindValue(":id", noteId);
    if (!query.exec()) {
        qDebug() << "Ошибка удаления заметки по ID:" << query.lastError().text();
        m_db.rollback();
        return false;
    }

    // Шаг B: Вытаскиваем все ОСТАВШИЕСЯ заметки строго по порядку их ID
    if (!query.exec("SELECT id, name, secured_text FROM notes ORDER BY id ASC;")) {
        qDebug() << "Ошибка выборки для пересчета хэшей:" << query.lastError().text();
        m_db.rollback();
        return false;
    }

    // Собираем их ID и данные во временный вектор, чтобы освободить query для обновлений
    struct TempNote { long long id; std::string name; std::string text; };
    std::vector<TempNote> remainingNotes;
    while (query.next()) {
        remainingNotes.push_back({
            query.value(0).toLongLong(),
            query.value(1).toString().toStdString(),
            query.value(2).toString().toStdString()
            });
    }

    // Шаг C: Бежим по цепочке с самого начала и обновляем previous_hash на диске
    std::string expectedHash = ""; // Первая заметка в базе всегда имеет предыдущий хэш ""

    for (const auto& note : remainingNotes) {
        // Записываем текущий ожидаемый хэш этой заметке
        query.prepare("UPDATE notes SET previous_hash = :prevHash WHERE id = :id;");
        query.bindValue(":prevHash", QString::fromStdString(expectedHash));
        query.bindValue(":id", note.id);

        if (!query.exec()) {
            qDebug() << "Ошибка обновления хэша для ID" << note.id << ":" << query.lastError().text();
            m_db.rollback();
            return false;
        }

        // Вычисляем, какой хэш должен быть у СЛЕДУЮЩЕЙ за ней заметки
        std::string dataToHash = note.name + note.text + expectedHash;
        expectedHash = CalculateSHA256(dataToHash);
    }

    // Если всё прошло гладко — фиксируем изменения на диске
    if (!IncrementDatabaseVersion()) {
        rollbackTransaction();
        return false;
    }
    return m_db.commit();
}

std::vector<Note> DatabaseManager::getAllNotes() {
    std::vector<Note> notesList;
    std::unordered_map<qlonglong, size_t> idToVectorIndex;
    QSqlQuery query;

    // Шаг 1: Читаем все заметки (ОБЯЗАТЕЛЬНО запрашиваем столбец id!)
    if (!query.exec("SELECT id, name, secured_text, previous_hash FROM notes ORDER BY id;")) {
        qDebug() << "Ошибка чтения заметок:" << query.lastError().text();
        return notesList;
    }

    while (query.next()) {
        // Достаем автоинкрементный ID из первой колонки (индекс 0)
        qlonglong dbId = query.value(0).toLongLong();
        std::string name = query.value(1).toString().toStdString();
        std::string securedText = query.value(2).toString().toStdString();
        std::string PrevHash = query.value(3).toString().toStdString();

        Note note;
        note.Id = dbId;
        note.Name = name;
        note.SecuredText = securedText;
        note.PreviousHash = PrevHash;
        notesList.push_back(note);
        idToVectorIndex[dbId] = notesList.size() - 1;
    }

    // Шаг 2: Чтение тегов (остается без изменений, он использует idToVectorIndex)
    if (!query.exec("SELECT note_id, tag_name FROM tags;")) {
        qDebug() << "Ошибка чтения тегов:" << query.lastError().text();
        return notesList;
    }

    while (query.next()) {
        qlonglong noteId = query.value(0).toLongLong();
        std::string tagName = query.value(1).toString().toStdString();

        if (idToVectorIndex.find(noteId) != idToVectorIndex.end()) {
            size_t vectorIdx = idToVectorIndex[noteId];
            notesList[vectorIdx].Tags.push_back(tagName);
        }
    }
    // === ШАГ 3: Чтение вложений из БД ===
    if (!query.exec("SELECT note_id, original_name_encrypted, disk_filename, iv, file_size, file_hash FROM attachments;")) {
        qDebug() << "Ошибка чтения вложений:" << query.lastError().text();
        return notesList;
    }

    while (query.next()) {
        qlonglong noteId = query.value(0).toLongLong();

        AttachmentInfo attach;
        attach.OriginalNameEncrypted = query.value(1).toString().toStdString();
        attach.DiskFileName = query.value(2).toString().toStdString();

        QByteArray ivData = query.value(3).toByteArray();
        attach.Iv.assign(ivData.begin(), ivData.end());

        attach.FileSize = query.value(4).toLongLong();
        attach.FileHash = query.value(5).toString().toStdString();
        // Если заметка с таким ID существует в нашем векторе, добавляем вложение к ней
        if (idToVectorIndex.find(noteId) != idToVectorIndex.end()) {
            size_t vectorIdx = idToVectorIndex[noteId];
            notesList[vectorIdx].Attachments.push_back(attach);
        }
    }

    return notesList;
}

bool DatabaseManager::Is2FAEnabled() {
    QSqlQuery query;
    if (query.exec("SELECT Is2FAEnabled FROM SystemInfo LIMIT 1;") && query.next()) {
        return query.value(0).toInt() == 1;
    }
    return false;
}

bool DatabaseManager::Enable2FA(const std::string& encryptedSecret) {
    QSqlQuery query;
    // Так как в SystemInfo всегда только одна строка, мы обновляем её
    query.prepare("UPDATE SystemInfo SET Is2FAEnabled = 1, Secret2FA = :Secret;");
    query.bindValue(":Secret", QString::fromStdString(encryptedSecret));

    if (!query.exec()) {
        qDebug() << "Ошибка включения 2FA в БД:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::Disable2FA() {
    QSqlQuery query;
    // Сбрасываем флаг и затираем строку с зашифрованным секретом в базе
    if (!query.exec("UPDATE SystemInfo SET Is2FAEnabled = 0, Secret2FA = '';")) {
        qDebug() << "Ошибка отключения 2FA в БД:" << query.lastError().text();
        return false;
    }
    return true;
}

std::string DatabaseManager::GetEncrypted2FASecret() {
    QSqlQuery query;
    if (query.exec("SELECT Secret2FA FROM SystemInfo LIMIT 1;") && query.next()) {
        return query.value(0).toString().toStdString();
    }
    return "";
}

bool DatabaseManager::beginTransaction() {
    if (m_db.isOpen()) {
        return m_db.transaction();
    }
    return false;
}

bool DatabaseManager::commitTransaction() {
    if (m_db.isOpen()) {
        return m_db.commit();
    }
    return false;
}

bool DatabaseManager::rollbackTransaction() {
    if (m_db.isOpen()) {
        return m_db.rollback();
    }
    return false;
}

bool DatabaseManager::IncrementDatabaseVersion() {
    QSqlQuery query;
    return query.exec("UPDATE SystemInfo SET Version = Version + 1;");
}

int DatabaseManager::GetDatabaseVersion() {
    QSqlQuery query;
    if (query.exec("SELECT Version FROM SystemInfo LIMIT 1;") && query.next()) {
        return query.value(0).toInt();
    }
    return 1; // Дефолтное значение, если что-то пошло не так
}