#include "TwoFactorDialog.h"
#include "WinThemeUtils.h" // Подключаем утилиты[cite: 14, 15, 16, 17]
#include <QIntValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

TwoFactorAuthDialog::TwoFactorAuthDialog(QWidget* parent) : QDialog(parent) {
    // --- НОВЫЙ РЕЖИМ ---
    setAttribute(Qt::WA_TranslucentBackground, true);
    setObjectName("TwoFactorAuthDialog");
    setWindowTitle("Проверка безопасности");
    setFixedSize(320, 200); // Слегка увеличили для корректных отступов канваса
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint); // Убираем знак вопроса
    WinThemeUtils::ApplyModernBlur(this); // Включаем Mica

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0); 

    QWidget* mainCanvas = new QWidget(this);
    mainCanvas->setObjectName("MainCanvas"); 
    rootLayout->addWidget(mainCanvas); 

    QVBoxLayout* mainLayout = new QVBoxLayout(mainCanvas);
    mainLayout->setContentsMargins(20, 20, 20, 20); 
    mainLayout->setSpacing(15);

    QLabel* infoLabel = new QLabel("Введите 6-значный код из приложения\nGoogle Authenticator:", mainCanvas);
    infoLabel->setAlignment(Qt::AlignCenter); 
    mainLayout->addWidget(infoLabel);

        // Поле ввода кода[cite: 11]
    m_CodeField = new QLineEdit(mainCanvas);
    m_CodeField->setObjectName("2FACodeField"); 
    m_CodeField->setAlignment(Qt::AlignCenter); 
    m_CodeField->setPlaceholderText("000 000");
    m_CodeField->setValidator(new QIntValidator(0, 999999, mainCanvas)); 
    m_CodeField->setMaxLength(6); 

        // Убираем непрозрачный фон для QLineEdit
    QPalette keyPal = m_CodeField->palette();
    keyPal.setColor(QPalette::Base, Qt::transparent);
    m_CodeField->setPalette(keyPal);

    m_CodeField->setStyleSheet("QLineEdit#2FACodeField { font-size: 20px; letter-spacing: 3px; font-weight: bold; }"); 
    mainLayout->addWidget(m_CodeField); 

    // Блок кнопок (ОК и Отмена)[cite: 11]
    QHBoxLayout* btnLayout = new QHBoxLayout(); 
    btnLayout->setSpacing(10); 

    m_CancelBtn = new QPushButton("Отмена", mainCanvas); 
    m_CancelBtn->setObjectName("2FACancelBtn"); 

    m_ConfirmBtn = new QPushButton("Подтвердить", mainCanvas); 
    m_ConfirmBtn->setObjectName("2FAConfirmBtn"); 
    m_ConfirmBtn->setDefault(true); 
    btnLayout->addWidget(m_CancelBtn); 
    btnLayout->addWidget(m_ConfirmBtn); 
    mainLayout->addLayout(btnLayout); 

    connect(m_ConfirmBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_CancelBtn, &QPushButton::clicked, this, &QDialog::reject); 
}

int TwoFactorAuthDialog::GetEnteredCode() const {
    return m_CodeField->text().toInt();
}