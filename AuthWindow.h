#pragma once

#include <QWidget>
#include <string>
#include <vector>
#include "CryptoManager.h" // Для структуры Note, если при авторизации мы сразу загружаем сейф
#include <openssl/crypto.h>

// Опережающие объявления виджетов Qt
class QLineEdit;
class QPushButton;
class QLabel;

class AuthWindow : public QWidget {
    Q_OBJECT

public:
    explicit AuthWindow(QWidget* parent = nullptr);
    ~AuthWindow();

private slots:
    void HandleBtnLogin();    
    void HandleBtnRegister();
    void HandleBtnImportBackup();
protected:
    void showEvent(QShowEvent* event) override;
private:
    bool IsStorageExists() const;
    // Элементы интерфейса
    QLabel* m_StatusLabel;
    QLineEdit* m_PasswordInput;
    QPushButton* m_BtnSettings;
    QPushButton* m_BtnLogin;
    QPushButton* m_BtnRegister;
};