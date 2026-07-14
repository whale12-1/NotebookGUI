#include "PasswordInput.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include "CryptoManager.h"

PasswordInput::PasswordInput(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Введите пароль от выбранного бэкапа");
    setFixedSize(360, 220);
    setStyleSheet(this->styleSheet());

    QVBoxLayout* MainLayout = new QVBoxLayout(this);
    QLabel* InfoLabel = new QLabel("Введите индивидуальный ключ для данного бэкапа:", this);
    MainLayout->addWidget(InfoLabel);
    m_KeyInput = new QLineEdit(this);
    m_KeyInput->setEchoMode(QLineEdit::Password);
    MainLayout->addWidget(m_KeyInput);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_LoginButton = new QPushButton("Войти", this);
    m_CancelButton = new QPushButton("Отмена", this);

    buttonLayout->addWidget(m_LoginButton);
    buttonLayout->addWidget(m_CancelButton);
    MainLayout->addLayout(buttonLayout);

    connect(m_LoginButton, &QPushButton::clicked, this, &PasswordInput::HandleLogin);
    connect(m_CancelButton, &QPushButton::clicked, this, &PasswordInput::HandleCancel);
}

PasswordInput::~PasswordInput() {

}

SecureString PasswordInput::GetKey() const {
    return QStringToSecureString(m_KeyInput->text().trimmed());
}

void PasswordInput::HandleLogin() {
    if (m_KeyInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Название заметки не может быть пустым!");
        return;
    }
    accept();
}

void PasswordInput::HandleCancel() {
    reject();
}