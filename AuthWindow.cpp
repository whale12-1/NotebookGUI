#include "AuthWindow.h"

// 1. Инклуды твоих собственных модулей проекта
#include "FileManager.h"    // Для проверки файла и загрузки сессии
#include "CryptoManager.h"   // Для функций шифрования, хеширования и PBKDF2
#include "MainWindow.h"      // Чтобы создать и показать главное окно при успешном входе
#include "TwoFactorDialog.h"
// 2. Инклуды элементов интерфейса Qt
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCoreApplication>
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
    setFixedSize(350, 180);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_StatusLabel = new QLabel(this);

    m_PasswordInput = new QLineEdit(this);
    m_PasswordInput->setEchoMode(QLineEdit::Password); // Скрываем ввод мастер-пароля
    m_PasswordInput->setPlaceholderText("Введите мастер-пароль");

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_BtnLogin = new QPushButton("Войти", this);
    m_BtnRegister = new QPushButton("Создать сейф", this);

    m_BtnLogin->setDefault(true); // Кнопка "Войти" сработает по нажатию Enter

    btnLayout->addWidget(m_BtnLogin);
    btnLayout->addWidget(m_BtnRegister);

    mainLayout->addWidget(m_StatusLabel);
    mainLayout->addWidget(m_PasswordInput);
    mainLayout->addLayout(btnLayout);

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
            Dialog.setStyleSheet(this->styleSheet());
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
        MainWindow* mainWindow = new MainWindow();

        // Инициализируем сессию в MainWindow, передавая туда живые расшифрованные ключи
        mainWindow->InitSession(decryptedDEK, encryptedDEK, salt,inputPassSecureString,DEKHash);
        mainWindow->show();

        this->close(); // Закрываем окно авторизации
    }
    catch (...) {
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