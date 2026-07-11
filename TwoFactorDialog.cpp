#include "TwoFactorDialog.h"
#include <QIntValidator>
#include <QHBoxLayout>

TwoFactorAuthDialog::TwoFactorAuthDialog(QWidget* parent) : QDialog(parent) {
    // Настройки самого окна
    setObjectName("TwoFactorAuthDialog");
    setWindowTitle("Проверка безопасности");
    setFixedSize(320, 180); // Фиксированный размер, чтобы окошко было аккуратным
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Убираем знак вопроса из заголовка

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Информационный текст
    QLabel* infoLabel = new QLabel("Введите 6-значный код из приложения\nGoogle Authenticator:", this);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("color: #2c3e50; font-size: 13px; font-weight: 500;");
    mainLayout->addWidget(infoLabel);

    // Поле ввода кода
    m_CodeField = new QLineEdit(this);
    m_CodeField->setObjectName("2FACodeField");
    m_CodeField->setAlignment(Qt::AlignCenter);
    m_CodeField->setPlaceholderText("000 000");

    // Ограничиваем ввод: только цифры, максимум 6 символов
    m_CodeField->setValidator(new QIntValidator(0, 999999, this));
    m_CodeField->setMaxLength(6);

    // Делаем шрифт крупным для удобства ввода цифр
    m_CodeField->setStyleSheet(
        "QLineEdit#2FACodeField { "
        "   font-size: 20px; "
        "   letter-spacing: 3px; "
        "   font-weight: bold; "
        "   padding: 6px; "
        "   border: 1px solid #dcdfe6; "
        "   border-radius: 6px; "
        "   background-color: #ffffff; "
        "   color: #2c3e50; "
        "}"
    );
    mainLayout->addWidget(m_CodeField);

    // Блок кнопок (ОК и Отмена)
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_CancelBtn = new QPushButton("Отмена", this);
    m_CancelBtn->setObjectName("2FACancelBtn");

    m_ConfirmBtn = new QPushButton("Подтвердить", this);
    m_ConfirmBtn->setObjectName("2FAConfirmBtn");
    m_ConfirmBtn->setDefault(true); // Кнопка будет срабатывать по нажатию Enter

    btnLayout->addWidget(m_CancelBtn);
    btnLayout->addWidget(m_ConfirmBtn);
    mainLayout->addLayout(btnLayout);

    // Соединяем сигналы со стандартными слотами QDialog
    connect(m_ConfirmBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_CancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

int TwoFactorAuthDialog::GetEnteredCode() const {
    return m_CodeField->text().toInt();
}