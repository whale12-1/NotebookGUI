#pragma once

#include <QDialog>  // Базовый класс для диалоговых окон
#include <string>    // Для геттеров, которые будут возвращать std::string наружу
#include "SecureString.h"
// Опережающие объявления виджетов Qt, чтобы хедер оставался легким
class QLineEdit;
class QTextEdit;
class QPushButton;
class QCheckBox;
class QHBoxLayout;

class PasswordInput : public QDialog {
    Q_OBJECT
public:
    PasswordInput(QWidget* parent = nullptr);
    ~PasswordInput();

    SecureString GetKey() const;
private slots:
    void HandleLogin();
    void HandleCancel();
private:
    QLineEdit* m_KeyInput;
    QPushButton* m_LoginButton;
    QPushButton* m_CancelButton;
};