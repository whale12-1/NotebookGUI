#include "ChangePass.h"

// Инструменты разметки и базовые виджеты
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

// Диалоговое окно для вывода ошибок (например, если пароли не совпали)
#include <QMessageBox>

ChangePassDialog::ChangePassDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Смена Мастер-пароля");
    setFixedSize(360, 240);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Поле 1: Старый пароль
    layout->addWidget(new QLabel("Введите старый мастер-пароль:", this));
    m_OldPassInput = new QLineEdit(this);
    m_OldPassInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_OldPassInput);

    // Поле 2: Новый пароль
    layout->addWidget(new QLabel("Введите новый мастер-пароль:", this));
    m_NewPassInput = new QLineEdit(this);
    m_NewPassInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_NewPassInput);

    // Поле 3: Подтверждение нового пароля
    layout->addWidget(new QLabel("Подтвердите новый мастер-пароль:", this));
    m_ConfirmPassInput = new QLineEdit(this);
    m_ConfirmPassInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_ConfirmPassInput);

    // Кнопки управления
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_BtnSave = new QPushButton("Сохранить", this);
    m_BtnCancel = new QPushButton("Отмена", this);
    btnLayout->addWidget(m_BtnSave);
    btnLayout->addWidget(m_BtnCancel);
    layout->addLayout(btnLayout);

    // Привязываем стандартные действия кнопок
    connect(m_BtnSave, &QPushButton::clicked, this, &ChangePassDialog::HandleBtnSave );
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
        QMessageBox::warning(this, "Ошибка валидации", "Пожалуйста, заполните все поля!");
        return; // Прерываем операцию, окно остаётся открытым
    }

    // 3. Валидация №2: Проверяем, что новый пароль совпадает с подтверждением
    if (newPass != confirmPass) {
        QMessageBox::warning(this, "Ошибка валидации", "Новый пароль и подтверждение не совпадают!");
        return;
    }

    // 4. Валидация №3: Защита от дурака — новый пароль не должен быть равен старому
    if (oldPass == newPass) {
        QMessageBox::warning(this, "Ошибка валидации", "Новый пароль не должен совпадать со старым!");
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
