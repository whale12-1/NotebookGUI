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

private:
    bool IsStorageExists() const;
    // Элементы интерфейса
    QLabel* m_StatusLabel;
    QLineEdit* m_PasswordInput;
    QPushButton* m_BtnLogin;
    QPushButton* m_BtnRegister;
};