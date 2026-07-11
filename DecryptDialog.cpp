#include "DecryptDialog.h"

// Тяжелые инклуды элементов интерфейса Qt
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

// Менеджеры разметки и диалоги
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

DecryptNoteDialog::DecryptNoteDialog(QWidget* parent) : QDialog(parent),m_IsExtraKeyNeeded(false){
    setWindowTitle("Выбор метода расшифровки");
    setFixedSize(500, 250);

    // Обрати внимание: теперь везде передаем адрес &cryptoDialog вместо указателя
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    QLabel* infoLabel = new QLabel("Введите индивидуальный ключ или используйте Мастер-пароль:", this);
    dialogLayout->addWidget(infoLabel);

    m_KeyInput = new QLineEdit(this);
    m_KeyInput->setEchoMode(QLineEdit::Password);
    dialogLayout->addWidget(m_KeyInput);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_BtnCustomKey = new QPushButton("🔑 Индивидуальным ключом", this);
    m_BtnMasterKey = new QPushButton("🛡️ Мастер-паролем", this);
    m_BtnCancel = new QPushButton("Отмена", this);

    buttonLayout->addWidget(m_BtnCustomKey);
    buttonLayout->addWidget(m_BtnMasterKey);
    buttonLayout->addWidget(m_BtnCancel);
    dialogLayout->addLayout(buttonLayout);


    connect(m_BtnCustomKey, &QPushButton::clicked,this, &DecryptNoteDialog::HandleBtnCustomKey);
    connect(m_BtnMasterKey, &QPushButton::clicked, this,&DecryptNoteDialog::HandleBtnMasterKey);
    connect(m_BtnCancel, &QPushButton::clicked, this, &DecryptNoteDialog::HandleBtnCancel);
}
DecryptNoteDialog::~DecryptNoteDialog(){};

void DecryptNoteDialog::HandleBtnCustomKey() {
    m_IsExtraKeyNeeded = true;
    if (m_KeyInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите индивидуальный ключ!");
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

// 2. Метод-геттер: сообщает MainWindow, был ли выбран индивидуальный ключ
bool DecryptNoteDialog::IsExtraKeyNeeded() {
    return m_IsExtraKeyNeeded;
}