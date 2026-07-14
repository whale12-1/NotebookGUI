#include "SettingsDialog.h"
#include "DatabaseManager.h"
#include "PasswordInput.h"
#include "FileManager.h"
#include "TwoFactorDialog.h"
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
    setWindowTitle("Настройки Сейфа");
    setFixedSize(400, 300);
    SetupUI();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::SetupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    m_TabWidget = new QTabWidget(this);

    // --- ВКЛАДКА 1: БЭКАПЫ ---
    QWidget* backupTab = new QWidget();
    QVBoxLayout* backupLayout = new QVBoxLayout(backupTab);

    QLabel* backupInfo = new QLabel(
        "Резервная копия содержит ваши зашифрованные заметки.\n"
        "Вы сможете восстановить их на другом устройстве с помощью мастер-пароля.",
        backupTab
    );
    backupInfo->setWordWrap(true);

    m_BtnCreateBackup = new QPushButton("Создать резервную копию", backupTab);
    m_BtnCreateBackup->setObjectName("btnCreateBackup"); // Для QSS

    backupLayout->addWidget(backupInfo);
    backupLayout->addStretch();
    backupLayout->addWidget(m_BtnCreateBackup);
    backupLayout->addSpacing(10);

    // --- ВКЛАДКА 2: БЕЗОПАСНОСТЬ (2FA) ---
    QWidget* securityTab = new QWidget();
    QVBoxLayout* securityLayout = new QVBoxLayout(securityTab);

    m_ChbEnable2FA = new QCheckBox("Включить двухфакторную аутентификацию (2FA)", securityTab);
    m_Lbl2FAStatus = new QLabel(securityTab);

    // Проверяем текущий статус 2FA в базе
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
    QPushButton* btnClose = new QPushButton("Закрыть", this);
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
        QMessageBox::information(this, "Бэкап", "Резервная копия успешно создана!");
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Критическая ошибка", QString("Ошибка при шифровании бэкапа: %1").arg(e.what()));
    }
}

void SettingsDialog::HandleToggle2FA(bool enabled) {
    if (enabled) {
        SecureString rawSecret;
        std::string displaySecret = Generate2FASecret(rawSecret);

        QDialog codeShowDialog(this);
        codeShowDialog.setWindowTitle("Настройка Google Authenticator");
        codeShowDialog.setFixedSize(380, 260);
        codeShowDialog.setStyleSheet(this->styleSheet());

        QVBoxLayout* cLayout = new QVBoxLayout(&codeShowDialog);
        QLabel* info = new QLabel("1. Добавьте этот секретный код в ваше мобильное приложение:\n", &codeShowDialog);
        info->setWordWrap(true);

        QLineEdit* keyField = new QLineEdit(QString::fromStdString(displaySecret), &codeShowDialog);
        keyField->setReadOnly(true);
        keyField->setAlignment(Qt::AlignCenter);
        keyField->setStyleSheet("font-size: 18px; font-weight: bold; letter-spacing: 2px; color: #409eff; background: #f4f4f5; border: 1px dashed #409eff; padding: 5px;");

        QLabel* info2 = new QLabel("\n2. Введите 6-значный код из телефона для подтверждения:", &codeShowDialog);

        QLineEdit* confirmField = new QLineEdit(&codeShowDialog);
        confirmField->setMaxLength(6);
        confirmField->setAlignment(Qt::AlignCenter);
        confirmField->setPlaceholderText("000 000");
        confirmField->setStyleSheet("font-size: 16px; padding: 5px;");

        QPushButton* saveBtn = new QPushButton("Подтвердить и сохранить", &codeShowDialog);
        saveBtn->setStyleSheet("background-color: #67c23a; color: white; font-weight: bold; padding: 8px; border-radius: 6px;");

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
                    QMessageBox::information(&codeShowDialog, "Успех", "2FA успешно активирована!");
                    isConfirmed = true;
                    codeShowDialog.accept();
                }
            }
            else {
                QMessageBox::critical(&codeShowDialog, "Ошибка", "Неверный код! Попробуйте еще раз.");
            }
            });

        codeShowDialog.exec();

        if (isConfirmed) {
            m_Lbl2FAStatus->setText("Статус: Подключено");
        }
        else {
            // Если пользователь закрыл окно или не подтвердил код, откатываем состояние чекбокса назад без вызова триггеров
            m_ChbEnable2FA->blockSignals(true);
            m_ChbEnable2FA->setChecked(false);
            m_ChbEnable2FA->blockSignals(false);
            m_Lbl2FAStatus->setText("Статус: Отключено");
        }
    }
    else {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Отключение 2FA", "Вы уверены, что хотите отключить двухфакторную аутентификацию?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            if (DatabaseManager::instance().Disable2FA()) {
                QMessageBox::information(this, "Успех", "Двухфакторная аутентификация отключена.");
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