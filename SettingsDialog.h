#pragma once

#include <QDialog>
#include "SecureString.h"
#include "CryptoManager.h"
#include "ProtectedKey.h"

class QTabWidget;
class QPushButton;
class QCheckBox;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(
        std::string& DEKHash,
        std::string& encryptedDEK,
        const std::vector<Note>& UserNotes,
        std::unique_ptr<ProtectedKey>& decryptedDEK,
        std::unique_ptr<ProtectedKey>&masterPass ,
        const std::string& randomSalt, 
        QWidget* parent = nullptr);
    ~SettingsDialog();

private slots:
    void HandleCreateBackup();
    void HandleToggle2FA(bool enabled);
    void HandleChangePassword();

private:
    void SetupUI();
    bool PassChanged = false;
    const std::vector<Note>& m_UserNotes;
    std::unique_ptr<ProtectedKey>& m_DecryptedDEK;
    std::unique_ptr<ProtectedKey>& m_MasterPassword;
    std::string& m_EncryptedDEK;
    std::string& m_DEKHash;
    std::string m_RandomSalt;
    QTabWidget* m_TabWidget;
    // Виджеты вкладки Бэкапа
    QPushButton* m_BtnCreateBackup;
    QPushButton* m_BtnChangePassword;
    // Виджеты вкладки 2FA
    QCheckBox* m_ChbEnable2FA;
    QLabel* m_Lbl2FAStatus;
};