#pragma once

#include <QDialog>
#include "SecureString.h"
#include "CryptoManager.h"

class QTabWidget;
class QPushButton;
class QCheckBox;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const std::vector<Note>& UserNotes, const SecureString& decryptedDEK, const std::string& randomSalt, QWidget* parent = nullptr);
    ~SettingsDialog();

private slots:
    void HandleCreateBackup();
    void HandleToggle2FA(bool enabled);

private:
    void SetupUI();

    const std::vector<Note>& m_UserNotes;
    SecureString m_DecryptedDEK;
    std::string m_RandomSalt;
    QTabWidget* m_TabWidget;
    // Виджеты вкладки Бэкапа
    QPushButton* m_BtnCreateBackup;
    // Виджеты вкладки 2FA
    QCheckBox* m_ChbEnable2FA;
    QLabel* m_Lbl2FAStatus;
};