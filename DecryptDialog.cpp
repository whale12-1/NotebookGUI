#include "DecryptDialog.h"
#include "WinThemeUtils.h" // Обязательно подключаем утилиты для размытия Mica
#include "ModernMessage.h"
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

DecryptNoteDialog::DecryptNoteDialog(QWidget* parent) : QDialog(parent), m_IsExtraKeyNeeded(false) {
    setObjectName("DecryptNoteDialog");

    // 1. Настройка системных флагов прозрачности и Mica
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
    WinThemeUtils::ApplyModernBlur(this);

    setWindowTitle("Выбор метода расшифровки");
    setFixedSize(500, 250);

    // 2. Главный базовый слой окна (ROOT)
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // 3. Создаем красивую подложку-канвас (все элементы будут лежать на ней)
    QWidget* mainCanvas = new QWidget(this);
    mainCanvas->setObjectName("MainCanvas");
    rootLayout->addWidget(mainCanvas);

    // 4. Внутренний слой для элементов на канвасе
    QVBoxLayout* dialogLayout = new QVBoxLayout(mainCanvas);
    dialogLayout->setContentsMargins(24, 24, 24, 24);
    dialogLayout->setSpacing(16);

    QLabel* infoLabel = new QLabel("Введите индивидуальный ключ или используйте Мастер-пароль:", mainCanvas);
    infoLabel->setWordWrap(true);
    dialogLayout->addWidget(infoLabel);

    // Поле ввода ключа
    m_KeyInput = new QLineEdit(mainCanvas);
    m_KeyInput->setEchoMode(QLineEdit::Password);
    m_KeyInput->setPlaceholderText("Ключ шифрования заметки...");

    // Сбрасываем стандартный белый фон на уровне палитры QLineEdit
    QPalette keyPal = m_KeyInput->palette();
    keyPal.setColor(QPalette::Base, Qt::transparent);
    m_KeyInput->setPalette(keyPal);

    dialogLayout->addWidget(m_KeyInput);

    // Слой с кнопками управления
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    m_BtnCustomKey = new QPushButton("🔑 Индивидуальным ключом", mainCanvas);
    m_BtnMasterKey = new QPushButton("🛡️ Мастер-паролем", mainCanvas);
    m_BtnCancel = new QPushButton("Отмена", mainCanvas);

    buttonLayout->addWidget(m_BtnCustomKey);
    buttonLayout->addWidget(m_BtnMasterKey);
    buttonLayout->addWidget(m_BtnCancel);

    dialogLayout->addLayout(buttonLayout);

    // Сигналы
    connect(m_BtnCustomKey, &QPushButton::clicked, this, &DecryptNoteDialog::HandleBtnCustomKey);
    connect(m_BtnMasterKey, &QPushButton::clicked, this, &DecryptNoteDialog::HandleBtnMasterKey);
    connect(m_BtnCancel, &QPushButton::clicked, this, &DecryptNoteDialog::HandleBtnCancel);
}

DecryptNoteDialog::~DecryptNoteDialog() {}

void DecryptNoteDialog::HandleBtnCustomKey() {
    m_IsExtraKeyNeeded = true;
    if (m_KeyInput->text().trimmed().isEmpty()) {
        ModernMessageBox::showWarning(this, "Внимание", "Пожалуйста, введите индивидуальный ключ!");
        return;
    }
    accept();
}

void DecryptNoteDialog::HandleBtnCancel() {
    reject();
}

void DecryptNoteDialog::HandleBtnMasterKey() {
    m_IsExtraKeyNeeded = false;
    accept();
}

SecureString DecryptNoteDialog::GetExtraKey() const {
    return QStringToSecureString(m_KeyInput->text().trimmed());
}

bool DecryptNoteDialog::IsExtraKeyNeeded() {
    return m_IsExtraKeyNeeded;
}