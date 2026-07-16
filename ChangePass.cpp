#include "ChangePass.h"
#include "WinThemeUtils.h" // Добавлено для Mica[cite: 14, 15, 16, 17]
#include "ModernMessage.h"
// Инструменты разметки и базовые виджеты
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

ChangePassDialog::ChangePassDialog(QWidget* parent) : QDialog(parent) {
    setObjectName("ChangePassDialog"); 

    // --- НОВЫЙ РЕЖИМ: Прозрачность и Mica-эффект ---
    setAttribute(Qt::WA_TranslucentBackground, true); 
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint); 
    WinThemeUtils::ApplyModernBlur(this); 

    setWindowTitle("Смена Мастер-пароля");
    setFixedSize(450, 350); 

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

        
    QWidget* mainCanvas = new QWidget(this);
    mainCanvas->setObjectName("MainCanvas");
    rootLayout->addWidget(mainCanvas);

    QVBoxLayout* layout = new QVBoxLayout(mainCanvas);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    // Поле 1: Старый пароль[cite: 13]
    layout->addWidget(new QLabel("Введите старый мастер-пароль:", mainCanvas)); 
    m_OldPassInput = new QLineEdit(mainCanvas); 
    m_OldPassInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_OldPassInput);

    // Поле 2: Новый пароль[cite: 13]
    layout->addWidget(new QLabel("Введите новый мастер-пароль:", mainCanvas)); 
    m_NewPassInput = new QLineEdit(mainCanvas);
    m_NewPassInput->setEchoMode(QLineEdit::Password); 
    layout->addWidget(m_NewPassInput);

    // Поле 3: Подтверждение нового пароля[cite: 13]
    layout->addWidget(new QLabel("Подтвердите новый мастер-пароль:", mainCanvas));
    m_ConfirmPassInput = new QLineEdit(mainCanvas);
    m_ConfirmPassInput->setEchoMode(QLineEdit::Password); 
    layout->addWidget(m_ConfirmPassInput);

    // Кнопки управления[cite: 13]
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_BtnSave = new QPushButton("Сохранить", mainCanvas); 
    m_BtnCancel = new QPushButton("Отмена", mainCanvas);
    btnLayout->addWidget(m_BtnSave); 
    btnLayout->addWidget(m_BtnCancel);
    layout->addLayout(btnLayout); 

    // Привязываем стандартные действия кнопок[cite: 13]
    connect(m_BtnSave, &QPushButton::clicked, this, &ChangePassDialog::HandleBtnSave);
    connect(m_BtnCancel, &QPushButton::clicked, this, &ChangePassDialog::HandleBtnCancel);
}

SecureString ChangePassDialog::GetOldPass() const {
    // 1. Получаем байты напрямую из Qt контейнера
    QByteArray bytes = m_OldPassInput->text().trimmed().toLocal8Bit();
    // 2. Конструируем SecureString из сырого указателя
    SecureString securePass(bytes.constData(), bytes.size());
    // 3. КРИТИЧЕСКИ ВАЖНО: Затираем память QByteArray нулями!
    // Заполняем весь буфер нулями, чтобы пароль не остался в куче.
    std::fill(bytes.begin(), bytes.end(), 0);
    return securePass;
}

SecureString ChangePassDialog::GetNewPass() const {
    QByteArray bytes = m_NewPassInput->text().trimmed().toLocal8Bit();
    // 2. Конструируем SecureString из сырого указателя
    SecureString securePass(bytes.constData(), bytes.size());
    // 3. КРИТИЧЕСКИ ВАЖНО: Затираем память QByteArray нулями!
    // Заполняем весь буфер нулями, чтобы пароль не остался в куче.
    std::fill(bytes.begin(), bytes.end(), 0);
    return securePass;
}


void ChangePassDialog::HandleBtnSave() {
    // 1. Вытаскиваем строки из полей и сразу убираем пробелы по краям
    QString oldPass = m_OldPassInput->text().trimmed();
    QString newPass = m_NewPassInput->text().trimmed();
    QString confirmPass = m_ConfirmPassInput->text().trimmed();

    // 2. Валидация №1: Проверяем, что ни одно поле не пустое
    if (oldPass.isEmpty() || newPass.isEmpty() || confirmPass.isEmpty()) {
        ModernMessageBox::showWarning(this, "Ошибка валидации", "Пожалуйста, заполните все поля!");
        return; // Прерываем операцию, окно остаётся открытым
    }

    // 3. Валидация №2: Проверяем, что новый пароль совпадает с подтверждением
    if (newPass != confirmPass) {
        ModernMessageBox::showWarning(this, "Ошибка валидации", "Новый пароль и подтверждение не совпадают!");
        return;
    }

    // 4. Валидация №3: Защита от дурака — новый пароль не должен быть равен старому
    if (oldPass == newPass) {
        ModernMessageBox::showWarning(this, "Ошибка валидации", "Новый пароль не должен совпадать со старым!");
        return;
    }

    // Если все проверки пройдены, закрываем диалог и возвращаем статус "Принято"
    accept();
}

void ChangePassDialog::HandleBtnCancel() {
    // Пользователь передумал — закрываем окно без изменений
    reject();
}


ChangePassDialog::~ChangePassDialog() {};
