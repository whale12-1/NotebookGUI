#include "SettingsDialog.h"
#include "DatabaseManager.h"
#include "PasswordInput.h"
#include "FileManager.h"
#include "TwoFactorDialog.h"
#include "WinThemeUtils.h" // Добавлено для Mica
#include "ModernMessage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <openssl/crypto.h>

SettingsDialog::SettingsDialog(const std::vector<Note>& UserNotes, const SecureString& decryptedDEK, const std::string& randomSalt, QWidget* parent)
    : QDialog(parent), m_DecryptedDEK(decryptedDEK), m_RandomSalt(randomSalt), m_UserNotes(UserNotes)
{
    setObjectName("SettingsDialog");

    // Настройка прозрачности и Mica-эффекта
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
    WinThemeUtils::ApplyModernBlur(this);

    setWindowTitle("Настройки Сейфа");
    setFixedSize(450, 350); // Чуть увеличили для комфортного размещения
    SetupUI();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::SetupUI() {
    // 1. Базовый ROOT слой без отступов
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // 2. Единая подложка-канвас
    QWidget* mainCanvas = new QWidget(this);
    mainCanvas->setObjectName("MainCanvas");
    rootLayout->addWidget(mainCanvas);

    // 3. Вертикальный контейнер на канвасе
    QVBoxLayout* mainLayout = new QVBoxLayout(mainCanvas);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    m_TabWidget = new QTabWidget(mainCanvas);
    m_TabWidget->setObjectName("SettingsTabWidget");

    // --- ВКЛАДКА 1: БЭКАПЫ ---
    QWidget* backupTab = new QWidget();
    backupTab->setAttribute(Qt::WA_TranslucentBackground, true); // Прозрачность вкладки
    QVBoxLayout* backupLayout = new QVBoxLayout(backupTab);
    backupLayout->setContentsMargins(15, 15, 15, 15);

    QLabel* backupInfo = new QLabel(
        "Резервная копия содержит ваши зашифрованные заметки.\n"
        "Вы сможете восстановить их на другом устройстве с помощью мастер-пароля.",
        backupTab
    );
    backupInfo->setWordWrap(true);

    m_BtnCreateBackup = new QPushButton("Создать резервную копию", backupTab);
    m_BtnCreateBackup->setObjectName("btnCreateBackup");

    backupLayout->addWidget(backupInfo);
    backupLayout->addStretch();
    backupLayout->addWidget(m_BtnCreateBackup);

    // --- ВКЛАДКА 2: БЕЗОПАСНОСТЬ (2FA) ---
    QWidget* securityTab = new QWidget();
    securityTab->setAttribute(Qt::WA_TranslucentBackground, true); // Прозрачность вкладки
    QVBoxLayout* securityLayout = new QVBoxLayout(securityTab);
    securityLayout->setContentsMargins(15, 15, 15, 15);

    m_ChbEnable2FA = new QCheckBox("Включить двухфакторную аутентификацию (2FA)", securityTab);
    m_Lbl2FAStatus = new QLabel(securityTab);

    bool is2FAActive = DatabaseManager::instance().Is2FAEnabled();
    m_ChbEnable2FA->setChecked(is2FAActive);
    m_Lbl2FAStatus->setText(is2FAActive ? "Статус: Подключено" : "Статус: Отключено");

    securityLayout->addWidget(m_ChbEnable2FA);
    securityLayout->addWidget(m_Lbl2FAStatus);
    securityLayout->addStretch();

    // Добавляем вкладки в виджет
    m_TabWidget->addTab(backupTab, "Резервное копирование");
    m_TabWidget->addTab(securityTab, "Безопасность");

    mainLayout->addWidget(m_TabWidget);

    // Кнопка закрытия окна настроек
    QPushButton* btnClose = new QPushButton("Закрыть", mainCanvas);
    btnClose->setObjectName("SettingsCloseBtn");
    mainLayout->addWidget(btnClose);

    // Коннекты сигналов
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_BtnCreateBackup, &QPushButton::clicked, this, &SettingsDialog::HandleCreateBackup);
    connect(m_ChbEnable2FA, &QCheckBox::toggled, this, &SettingsDialog::HandleToggle2FA);
}

void SettingsDialog::HandleCreateBackup() {
    // Тут вызывается логика создания бэкапа (экспорт текущей базы)
    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Сохранить бэкап",
        "",
        "Зашифрованные бэкапы (*.enc)"
    );

    if (savePath.isEmpty()) return;

    SecureString EnteredKey;
    PasswordInput PasswordDialog(this);

    // Вызываем exec() у созданного объекта диалога
    if (PasswordDialog.exec() == QDialog::Accepted) {
        EnteredKey = PasswordDialog.GetKey();
    }
    else {
        return; // Пользователь нажал "Отмена" при вводе пароля
    }
    try {
        FileManager BackupSaver(savePath.toStdString());
        std::vector<unsigned char>RandomSaltVector(m_RandomSalt.begin(), m_RandomSalt.end());
        std::string EncryptedDEK = EncryptAES(m_DecryptedDEK, EnteredKey, RandomSaltVector);
        std::string dekForHash(m_DecryptedDEK.data(), m_DecryptedDEK.length());
        std::string DEKHash = CalculateSHA256(dekForHash);
        OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
        if (!BackupSaver.SaveBackup(m_UserNotes, EncryptedDEK, m_RandomSalt, DEKHash)) {
            QMessageBox::critical(this, "Ошибка записи", "Не удалось сохранить бэкап или формат файла поврежден.");
            return;
        }
        ModernMessageBox::showInfo(this, "Бэкап", "Резервная копия успешно создана!");
    }
    catch (const std::exception& e) {
        ModernMessageBox::showCritical(this, "Критическая ошибка", QString("Ошибка при шифровании бэкапа: %1").arg(e.what()));
    }
}

void SettingsDialog::HandleToggle2FA(bool enabled) {
    if (enabled) {
        SecureString rawSecret;
        std::string displaySecret = Generate2FASecret(rawSecret);

        // --- СОЗДАНИЕ И НАСТРОЙКА ДИАЛОГА В НОВОМ СТИЛЕ ---
        QDialog codeShowDialog(this);
        codeShowDialog.setObjectName("TwoFactorSetupDialog");
        codeShowDialog.setAttribute(Qt::WA_TranslucentBackground, true); // Прозрачность[cite: 11]
        codeShowDialog.setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint); 
        WinThemeUtils::ApplyModernBlur(&codeShowDialog); // Размытие Mica

        codeShowDialog.setWindowTitle("Настройка Google Authenticator");
        codeShowDialog.setFixedSize(400, 310); // Немного увеличили высоту под canvas-контейнер
        codeShowDialog.setStyleSheet(this->styleSheet());

        // 1. Корневой Layout без отступов[cite: 12, 15, 16]
        QVBoxLayout* rootLayout = new QVBoxLayout(&codeShowDialog);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        // 2. Единая подложка-канвас для стилизации[cite: 12, 15, 16]
        QWidget* mainCanvas = new QWidget(&codeShowDialog);
        mainCanvas->setObjectName("MainCanvas"); 
        rootLayout->addWidget(mainCanvas); 

        // 3. Внутренний контейнер на канвасе[cite: 12, 15, 16]
        QVBoxLayout* cLayout = new QVBoxLayout(mainCanvas);
        cLayout->setContentsMargins(20, 20, 20, 20); 
        cLayout->setSpacing(10);

        QLabel* info = new QLabel("1. Добавьте этот секретный код в ваше мобильное приложение:\n", mainCanvas);
        info->setWordWrap(true);

        QLineEdit* keyField = new QLineEdit(QString::fromStdString(displaySecret), mainCanvas);
        keyField->setReadOnly(true);
        keyField->setAlignment(Qt::AlignCenter);

        // Убираем непрозрачный фон для QLineEdit на прозрачной форме
        QPalette keyPal = keyField->palette();
        keyPal.setColor(QPalette::Base, Qt::transparent);
        keyField->setPalette(keyPal);
        keyField->setStyleSheet("font-size: 18px; font-weight: bold; letter-spacing: 2px; color: #7289da; border: 1px dashed rgba(114, 137, 218, 0.4); padding: 5px;");

        QLabel* info2 = new QLabel("\n2. Введите 6-значный код из телефона для подтверждения:", mainCanvas);

        QLineEdit* confirmField = new QLineEdit(mainCanvas);
        confirmField->setMaxLength(6);
        confirmField->setAlignment(Qt::AlignCenter);
        confirmField->setPlaceholderText("000 000");
        confirmField->setObjectName("2FACodeField");

        QPalette confirmPal = confirmField->palette();
        confirmPal.setColor(QPalette::Base, Qt::transparent);
        confirmField->setPalette(confirmPal);

        QPushButton* saveBtn = new QPushButton("Подтвердить и сохранить", mainCanvas);
        saveBtn->setStyleSheet("background-color: rgba(103, 194, 58, 0.15); border: 1px solid rgba(103, 194, 58, 0.25); color: #67c23a; font-weight: bold; padding: 8px;");

        cLayout->addWidget(info);
        cLayout->addWidget(keyField);
        cLayout->addWidget(info2);
        cLayout->addWidget(confirmField);
        cLayout->addWidget(saveBtn);

        bool isConfirmed = false;

        connect(saveBtn, &QPushButton::clicked, [&]() {
            int code = confirmField->text().toInt();
            if (Verify2FACode(rawSecret, code)) {
                std::vector<unsigned char> saltVector(m_RandomSalt.begin(), m_RandomSalt.end());
                std::string encryptedSecret = EncryptAES(rawSecret, m_DecryptedDEK, saltVector);

                if (DatabaseManager::instance().Enable2FA(encryptedSecret)) {
                    // Замена QMessageBox на кастомный ModernMessage
                    ModernMessageBox::showInfo(&codeShowDialog, "Успех", "2FA успешно активирована!");
                    isConfirmed = true;
                    codeShowDialog.accept();
                }
            }
            else {
                ModernMessageBox::showCritical(&codeShowDialog, "Ошибка", "Неверный код! Попробуйте еще раз.");
            }
            });

        codeShowDialog.exec();

        if (isConfirmed) {
            m_Lbl2FAStatus->setText("Статус: Подключено");
        }
        else {
            // Если пользователь закрыл окно или не подтвердил код, откатываем состояние чекбокса
            m_ChbEnable2FA->blockSignals(true);
            m_ChbEnable2FA->setChecked(false);
            m_ChbEnable2FA->blockSignals(false);
            m_Lbl2FAStatus->setText("Статус: Отключено");
        }
    }
    else {
        // Использование кастомного окна диалога "Да/Нет"
        bool reply = ModernMessageBox::showQuestion(this, "Отключение 2FA", "Вы уверены, что хотите отключить двухфакторную аутентификацию?");

        if (reply) {
            if (DatabaseManager::instance().Disable2FA()) {
                ModernMessageBox::showInfo(this, "Успех", "Двухфакторная аутентификация отключена.");
                m_Lbl2FAStatus->setText("Статус: Отключено");
            }
        }
        else {
            // Если передумал — возвращаем галочку обратно
            m_ChbEnable2FA->blockSignals(true);
            m_ChbEnable2FA->setChecked(true);
            m_ChbEnable2FA->blockSignals(false);
        }
    }
}