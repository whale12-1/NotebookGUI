#include "AuthWindow.h"

// 1. Инклуды твоих собственных модулей проекта
#include "FileManager.h"    // Для проверки файла и загрузки сессии
#include "CryptoManager.h"   // Для функций шифрования, хеширования и PBKDF2
#include "MainWindow.h"      // Чтобы создать и показать главное окно при успешном входе
#include "TwoFactorDialog.h"
#include "PasswordInput.h"
// 2. Инклуды элементов интерфейса Qt
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QStyle>
#include <fstream>

// 3. Менеджеры геометрии и окна сообщений
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

// 4. Стандартные системные библиотеки

AuthWindow::AuthWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Авторизация в Сейфе");
    // Увеличили высоту со 180 до 200, чтобы элементам было свободно
    setFixedSize(400, 290);

    // Главный вертикальный менеджер компоновки
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 16); // Делаем аккуратные отступы от краев
    mainLayout->setSpacing(10);

    // --- ВЕРХНЯЯ ПАНЕЛЬ С КНОПКОЙ НАСТРОЕК (TOP BAR) ---
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    topBarLayout->setContentsMargins(0, 0, 0, 0);

    m_BtnSettings = new QPushButton("⚙", this);
    m_BtnSettings->setFixedSize(28, 28); // Задаем фиксированный размер
    m_BtnSettings->setObjectName("Settings2FABtn"); // Привязываем к стилю шестеренки из QSS

    // Создаем выпадающее меню
    QMenu* settingsMenu = new QMenu(this);
    QAction* importAction = settingsMenu->addAction("Импортировать бэкап");
    m_BtnSettings->setMenu(settingsMenu);

    // Добавляем кнопку в горизонтальный лайаут и ставим пружину (Spacer) справа.
    // Это сдвинет кнопку влево и не даст ей растягиваться.
    topBarLayout->addWidget(m_BtnSettings);
    topBarLayout->addStretch();
    // ----------------------------------------------------

    m_StatusLabel = new QLabel(this);
    m_StatusLabel->setWordWrap(true); // Разрешаем перенос длинного текста на новую строку

    m_PasswordInput = new QLineEdit(this);
    m_PasswordInput->setEchoMode(QLineEdit::Password); // Скрываем ввод мастер-пароля
    m_PasswordInput->setPlaceholderText("Введите мастер-пароль");

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_BtnLogin = new QPushButton("Войти", this);
    m_BtnRegister = new QPushButton("Создать сейф", this);

    m_BtnLogin->setDefault(true); // Кнопка "Войти" сработает по нажатию Enter

    btnLayout->addWidget(m_BtnLogin);
    btnLayout->addWidget(m_BtnRegister);

    // Собираем интерфейс строго по порядку сверху вниз:
    mainLayout->addLayout(topBarLayout); // 1. Верхний бар с кнопкой
    mainLayout->addWidget(m_StatusLabel); // 2. Описание статуса
    mainLayout->addWidget(m_PasswordInput); // 3. Поле ввода пароля
    mainLayout->addLayout(btnLayout);     // 4. Кнопки действия

    // Логика переключения режимов при старте
    if (IsStorageExists()) {
        m_StatusLabel->setText("Обнаружен существующий сейф. Пожалуйста, авторизуйтесь:");
        m_BtnRegister->setEnabled(false); // Нельзя создать новый, пока лежит старый
    }
    else {
        m_StatusLabel->setText("Сейф не найден. Придумайте мастер-пароль для инициализации:");
        m_BtnLogin->setEnabled(false);    // Войти нельзя, файла еще нет
    }

    // Коннекты слотов
    connect(m_BtnLogin, &QPushButton::clicked, this, &AuthWindow::HandleBtnLogin);
    connect(m_BtnRegister, &QPushButton::clicked, this, &AuthWindow::HandleBtnRegister);
    connect(importAction, &QAction::triggered, this, &AuthWindow::HandleBtnImportBackup);
}

AuthWindow::~AuthWindow() {}

bool AuthWindow::IsStorageExists() const {
    // 1. Получаем точный абсолютный путь к файлу рядом с бинарником
    if (!DatabaseManager::instance().initDatabase("SafeData.db")) {
        return false;
    }

    if (DatabaseManager::instance().HasCryptoKeys()) {
        return true;
    }
    return false;
}

void AuthWindow::HandleBtnLogin() {
    QString password = m_PasswordInput->text().trimmed();
    if (password.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пароль не может быть пустым!");
        return;
    }

    std::string encryptedDEK;
    std::string salt;
    std::string DEKHash;

    QString MasterSalt, EncryptedDek, DekHash;
    DatabaseManager::instance().LoadCryptoKeys(MasterSalt, EncryptedDek, DekHash);
    if (!DatabaseManager::instance().LoadCryptoKeys(MasterSalt, EncryptedDek, DekHash)) {
        QMessageBox::critical(this, "Ошибка", "Служебные крипто-данные не найдены в базе!");
        return;
    }
    salt = MasterSalt.toStdString();
    encryptedDEK = EncryptedDek.toStdString();
    DEKHash = DekHash.toStdString();
    try {
        SecureString inputPassSecureString = QStringToSecureString(password);
        std::vector<unsigned char> saltVector(salt.begin(), salt.end()); // Преобразуем соль в вектор для криптографии
        SecureString decryptedDEK = DecryptAES(encryptedDEK, inputPassSecureString, saltVector);
        std::string dekForHash(decryptedDEK.data(), decryptedDEK.length());
        if (CalculateSHA256(dekForHash) != DEKHash) {
            OPENSSL_cleanse(&decryptedDEK[0], decryptedDEK.length());
            OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
            QMessageBox::critical(this, "Ошибка доступа", "Неверный мастер-пароль!");
            return;
        }
        OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
        if (DatabaseManager::instance().Is2FAEnabled()) {
            TwoFactorAuthDialog Dialog(this);
            if (Dialog.exec() == QDialog::Accepted) {
                int EnteredCode = Dialog.GetEnteredCode();
                try {
                    SecureString DecSecret2FA = DecryptAES(DatabaseManager::instance().GetEncrypted2FASecret(), decryptedDEK, saltVector);
                    if (!Verify2FACode(DecSecret2FA, EnteredCode)) {
                        QMessageBox::critical(this, "Ошибка доступа", "Неверный код!");
                        return;
                    }
                }
                catch (...) {
                    QMessageBox::critical(this, "Ошибка доступа", "Не пройдена 2FA!");
                    return;
                }
            }
            else {
                return;
            }
        }
        // 3. ЕСЛИ ВСЁ ОТЛИЧНО: Создаем главное окно, передаем туда данные и открываем его!
        m_BtnLogin->setEnabled(false);
        m_PasswordInput->setEnabled(false);
        m_StatusLabel->setText("Проверка целостности базы данных сейфа...");

            // 4. Создаем watcher для слежения за фоновым потоком проверки хэшей
        auto* watcher = new QFutureWatcher<bool>(this);

        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, decryptedDEK, encryptedDEK, salt, inputPassSecureString, DEKHash]() mutable {
            // Получаем результат выполнения ValidateIntegrity из фонового потока
            bool integrityOk = watcher->result();
            watcher->deleteLater();

            if (!integrityOk) {
                QMessageBox::critical(this, "Критическая ошибка", "Обнаружено несанкционированное изменение данных Сейфа (цепочка хэшей разорвана). Доступ заблокирован!");
                // Разблокируем форму, чтобы дать шанс войти в другой сейф или попробовать снова
                m_BtnLogin->setEnabled(true);
                m_PasswordInput->setEnabled(true);
                m_StatusLabel->setText("Обнаружен существующий сейф. Пожалуйста, авторизуйтесь:");
                return;
            }

            // 5. ЕСЛИ ХЭШИ В ПОРЯДКЕ: Создаем MainWindow, передаем сессию и открываем
            MainWindow* mainWindow = new MainWindow();
            mainWindow->InitSession(decryptedDEK, encryptedDEK, salt, inputPassSecureString, DEKHash);
            mainWindow->show();

            this->close(); // Закрываем окно авторизации[cite: 3]
            });
        QString dbPath = "SafeData.db";
        // 6. Отправляем ValidateIntegrity() в фоновый пул потоков QtConcurrent
        QFuture<bool> future = QtConcurrent::run([dbPath]() {
            // Создаем уникальное имя соединения для этого конкретного фонового потока
            QString connectionName = "Thread_Validation_Connection";

            bool result = false;

            // Открываем отдельное временное соединение с тем же файлом БД внутри этого потока
            {
                QSqlDatabase threadDb = QSqlDatabase::addDatabase("QSQLITE", connectionName);
                threadDb.setDatabaseName(dbPath);

                if (!threadDb.open()) {
                    qDebug() << "Фоновый поток не смог открыть БД:" << threadDb.lastError().text();
                    QSqlDatabase::removeDatabase(connectionName);
                    return false;
                }

                // Создаем локальный запрос, явно привязанный к новому потоковому соединению
                QSqlQuery query(threadDb);

                // Включаем внешние ключи, если нужно
                query.exec("PRAGMA foreign_keys = ON;");

                // Выполняем логику ValidateIntegrity прямо здесь, используя query этого потока
                if (!query.exec("SELECT name, secured_text, previous_hash FROM notes ORDER BY id ASC;")) {
                    threadDb.close();
                    QSqlDatabase::removeDatabase(connectionName);
                    return false;
                }

                result = true; // Предположим, что все ок, пока не доказано обратное
                std::string expectedHash = "";

                while (query.next()) {
                    std::string name = query.value(0).toString().toStdString();
                    std::string securedText = query.value(1).toString().toStdString();
                    std::string previousHash = query.value(2).toString().toStdString();

                    if (previousHash != expectedHash) {
                        qDebug() << "КРИТИЧЕСКАЯ ОШИБКА В ПОТОКЕ: Цепочка хэшей разорвана!";
                        result = false;
                        break;
                    }

                    std::string dataToHash = name + securedText + previousHash;
                    expectedHash = CalculateSHA256(dataToHash);
                }

                threadDb.close(); // Закрываем локальное соединение потока
            }

            // Очищаем имя соединения из глобального репозитория Qt
            QSqlDatabase::removeDatabase(connectionName);

            return result;
            });
        watcher->setFuture(future);
    }
    catch (...) {
        m_BtnLogin->setEnabled(true);
        m_PasswordInput->setEnabled(true);
        m_StatusLabel->setText("Обнаружен существующий сейф. Пожалуйста, авторизуйтесь:");
        QMessageBox::critical(this, "Ошибка доступа", "Неверный мастер-пароль!");
    }
}

void AuthWindow::HandleBtnRegister() {
    QString password = m_PasswordInput->text().trimmed();
    if (password.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Введите пароль для инициализации сейфа!");
        return;
    }
    std::string Salt = GenerateRandomSalt();
    std::vector<unsigned char> SaltVector(Salt.begin(),Salt.end());
    SecureString DEK = GetRandonDEK();
    std::string EncryptedDek = EncryptAES(DEK, QStringToSecureString(password), SaltVector);
    std::vector<Note> Notes;
    std::string PreviosHash = "";
    std::string dekForHash(DEK.data(), DEK.length());
    std::string DEKHash = CalculateSHA256(dekForHash);
    OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
    if (DatabaseManager::instance().SaveCryptoKeys(
        QString::fromStdString(Salt),
        QString::fromStdString(EncryptedDek),
        QString::fromStdString(DEKHash)))
    {
        QMessageBox::information(this, "Успех", "Сейф успешно создан!");
        this->close();
    }
    else {
        QMessageBox::warning(this, "Ошибка", "Не удалось инициализировать базу данных.");
    }
}

void AuthWindow::HandleBtnImportBackup() {
    // 1. Выбор файла бэкапа
    QString InitialDir = QCoreApplication::applicationDirPath();
    QString FileFilter = "Зашифрованные бэкапы (*.enc);;Все файлы (*.*)";
    QString SelectedFile = QFileDialog::getOpenFileName(
        this,
        "Выберите файл бэкапа",
        InitialDir,
        FileFilter
    );

    if (SelectedFile.isEmpty()) {
        return; // Пользователь отменил выбор файла
    }

    // 2. Ввод пароля (объявляем переменную ключа заранее, чтобы сохранить область видимости)
    SecureString EnteredKey;
    PasswordInput PasswordDialog(this);

    // Вызываем exec() у созданного объекта диалога
    if (PasswordDialog.exec() == QDialog::Accepted) {
        EnteredKey = PasswordDialog.GetKey();
    }
    else {
        return; // Пользователь нажал "Отмена" при вводе пароля
    }

    // 3. Создаем переменные для данных бэкапа и загружаем файл
    std::vector<Note> Notes;
    std::string Salt;
    std::string EncryptedDEK;
    std::string DEKHash;

    FileManager BackUp(SelectedFile.toStdString()); // Приводим QString к std::string
    if (!BackUp.LoadBackup(Notes, EncryptedDEK, Salt, DEKHash)) {
        QMessageBox::critical(this, "Ошибка чтения", "Не удалось прочитать бэкап или формат файла поврежден.");
        return;
    }

    // Переменные для транзакции и отката
    bool backupInProgress = false;
    QString dbPath = "SafeData.db";
    QString backupOldDbPath = "SafeData.db.old";

    try {
        // 4. Криптографическая проверка пароля бэкапа
        std::vector<unsigned char> SaltVector(Salt.begin(), Salt.end());
        SecureString DecryptedDEK = DecryptAES(EncryptedDEK, EnteredKey, SaltVector);
        std::string DekForHash(DecryptedDEK.data(), DecryptedDEK.length());

        if (CalculateSHA256(DekForHash) != DEKHash) {
            OPENSSL_cleanse(&DecryptedDEK[0], DecryptedDEK.length());
            OPENSSL_cleanse(&DekForHash[0], DekForHash.length());
            QMessageBox::critical(this, "Ошибка доступа", "Неверный пароль от бэкапа!");
            return;
        }
        OPENSSL_cleanse(&DekForHash[0], DekForHash.length());

        // 5. Работа с файловой системой и базой данных
        DatabaseManager::instance().closeDatabase();

        // Сигнализируем, что сейчас начнется опасная фаза перезаписи файлов БД
        backupInProgress = true;

        if (QFile::exists(backupOldDbPath)) {
            QFile::remove(backupOldDbPath);
        }

        if (QFile::exists(dbPath)) {
            if (!QFile::rename(dbPath, backupOldDbPath)) {
                throw std::runtime_error("Не удалось переименовать старую базу.");
            }
        }

        // Создаем новую базу
        if (!DatabaseManager::instance().initDatabase(dbPath)) {
            throw std::runtime_error("Не удалось инициализировать новую базу.");
        }

        // Сохраняем импортированные служебные ключи
        if (!DatabaseManager::instance().SaveCryptoKeys(
            QString::fromStdString(Salt),
            QString::fromStdString(EncryptedDEK),
            QString::fromStdString(DEKHash)))
        {
            throw std::runtime_error("Не удалось сохранить новые ключи шифрования.");
        }

        // 6. Импорт заметок с пересчетом цепочки хэшей
        std::string expectedPreviousHash = "";
        bool importSuccess = true;

        DatabaseManager::instance().beginTransaction();

        for (auto& note : Notes) {
            note.PreviousHash = expectedPreviousHash;

            if (!DatabaseManager::instance().InsertNoteRaw(note)) {
                importSuccess = false;
                break;
            }

            std::string DataToHash = note.Name + note.SecuredText + note.PreviousHash;
            expectedPreviousHash = CalculateSHA256(DataToHash);
        }

        if (importSuccess) {
            DatabaseManager::instance().commitTransaction();
            QFile::remove(backupOldDbPath); // Все ок — удаляем старый бэкап БД

            QMessageBox::information(this, "Успех",
                "База данных успешно восстановлена из бэкапа!\nИспользуйте пароль бэкапа для входа.");

            m_StatusLabel->setText("Обнаружен существующий сейф. Пожалуйста, авторизуйтесь:");
            m_BtnLogin->setEnabled(true);
            m_BtnRegister->setEnabled(false);
        }
        else {
            // Если запись упала — вызываем исключение, чтобы уйти в общий блок отката
            throw std::runtime_error("Ошибка при импорте заметок.");
        }

        OPENSSL_cleanse(&DecryptedDEK[0], DecryptedDEK.length());
    }
    catch (...) {
        QMessageBox::critical(this, "Критическая ошибка", "Произошла непредвиденная ошибка при восстановлении базы!");

        // Надежный откат файлов на диске в исходное состояние
        if (backupInProgress) {
            DatabaseManager::instance().closeDatabase();
            if (QFile::exists(dbPath)) {
                QFile::remove(dbPath);
            }
            if (QFile::exists(backupOldDbPath)) {
                QFile::rename(backupOldDbPath, dbPath);
            }
            DatabaseManager::instance().initDatabase(dbPath);
        }

        // Синхронизируем GUI с итоговым состоянием БД
        if (IsStorageExists()) {
            m_StatusLabel->setText("Обнаружен существующий сейф. Пожалуйста, авторизуйтесь:");
            m_BtnRegister->setEnabled(false);
            m_BtnLogin->setEnabled(true);
        }
        else {
            m_StatusLabel->setText("Сейф не найден. Придумайте мастер-пароль для инициализации:");
            m_BtnLogin->setEnabled(false);
            m_BtnRegister->setEnabled(true);
        }
    }
}