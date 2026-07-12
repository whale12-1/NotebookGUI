#include "CreateNoteDialog.h"

// Менеджеры разметки (Layouts)
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QToolBar>
#include <QAction>

CreateNoteDialog::CreateNoteDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Создание новой заметки");
    setMinimumWidth(400);
    setMinimumHeight(450);

    QVBoxLayout* MainLayout = new QVBoxLayout(this);

    QLabel* NameLabel = new QLabel("Название заметки:", this);
    m_NameInput = new QLineEdit(this);
    m_NameInput->setPlaceholderText("Введите название...");
    MainLayout->addWidget(NameLabel);
    MainLayout->addWidget(m_NameInput);

    QToolBar* mdToolBar = new QToolBar("Форматирование", this);
    mdToolBar->setStyleSheet("QToolBar { background-color: #252526; padding: 2px; border-radius: 4px; }");

    // Кнопка Жирный текст (Bold)
    QAction* boldAction = mdToolBar->addAction("B");
    boldAction->setToolTip("Жирный текст");
    connect(boldAction, &QAction::triggered, this, [this]() {
        QTextCursor cursor = m_TextInput->textCursor();
        if (!cursor.hasSelection()) return;

        QTextCharFormat format;
        // Если текст уже жирный — убираем, если нет — делаем жирным (переключатель)
        format.setFontWeight(cursor.charFormat().fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        cursor.mergeCharFormat(format);
        });

    // Кнопка Курсив (Italic)
    QAction* italicAction = mdToolBar->addAction("/");
    italicAction->setToolTip("Курсив");
    connect(italicAction, &QAction::triggered, this, [this]() {
        QTextCursor cursor = m_TextInput->textCursor();
        if (!cursor.hasSelection()) return;

        QTextCharFormat format;
        format.setFontItalic(!cursor.charFormat().fontItalic());
        cursor.mergeCharFormat(format);
        });

    // Кнопка Блок кода (Code Block)
    QAction* codeAction = mdToolBar->addAction("</>");
    codeAction->setToolTip("Блок кода");
    connect(codeAction, &QAction::triggered, this, [this]() {
        QTextCursor cursor = m_TextInput->textCursor();
        cursor.insertText("\n```cpp\n// Ваш код здесь\n```\n");
        });

    // Добавляем панель инструментов в разметку перед полем ввода текста!
    MainLayout->addWidget(mdToolBar);
    // 2. Поле ввода текста заметки
    QLabel* TextLabel = new QLabel("Содержимое (текст):",this);
    m_TextInput = new QTextEdit(this);
    m_TextInput->setPlaceholderText("Напишите что-нибудь секретное...");
    MainLayout->addWidget(TextLabel);
    MainLayout->addWidget(m_TextInput);

    // Разделительная линия для визуального порядка
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    MainLayout->addWidget(line);

    // 3. Выбор режима шифрования (Чекбокс)
    m_AutoEncryptCheck = new QCheckBox("Шифровать автоматически (Мастер-паролем)", this);
    m_AutoEncryptCheck->setChecked(true); // По умолчанию режим авто-шифрования включен
    MainLayout->addWidget(m_AutoEncryptCheck);

    // 4. Поле для кастомного ключа
    QLabel* customKeyLabel = new QLabel("Уникальный ключ для этой заметки:", this);
    m_CustomKeyInput = new QLineEdit(this);
    m_CustomKeyInput->setPlaceholderText("Ключ заблокирован (включен авто-режим)");
    m_CustomKeyInput->setEchoMode(QLineEdit::Password);
    m_CustomKeyInput->setEnabled(false); // Изначально выключено, так как чекбокс активен
    MainLayout->addWidget(customKeyLabel);
    MainLayout->addWidget(m_CustomKeyInput);


    m_TagInput = new QLineEdit(this);
    m_TagInput->setPlaceholderText("Добавить тег (например: работа)...");

    m_AddTagButton = new QPushButton("+", this);
    m_AddTagButton->setFixedWidth(30); // Делаем кнопку компактной и квадратной
    m_AddTagButton->setObjectName("AddTagButton");
    // Собираем их в один горизонтальный ряд
    QHBoxLayout* tagInputLayout = new QHBoxLayout();
    tagInputLayout->addWidget(m_TagInput);
    tagInputLayout->addWidget(m_AddTagButton);

    // 2. Создаем горизонтальный контейнер, где будут жить плашки уже добавленных тегов
    QWidget* tagsContainer = new QWidget(this);
    m_TagsLayout = new QHBoxLayout(tagsContainer);
    m_TagsLayout->setContentsMargins(0, 0, 0, 0);
    m_TagsLayout->setSpacing(4);

     MainLayout->addLayout(tagInputLayout);
     MainLayout->addWidget(tagsContainer);

    // 3. Подключаем обработку нажатия на кнопку "+"
    connect(m_AddTagButton, &QPushButton::clicked, this, [this]() {
        QString tagText = m_TagInput->text().trimmed(); // Убираем лишние пробелы

        if (!tagText.isEmpty()) {
            // Проверяем, нет ли уже такого тега, чтобы избежать дубликатов
            std::string stdTag = tagText.toStdString();
            if (std::find(m_CurrentTags.begin(), m_CurrentTags.end(), stdTag) == m_CurrentTags.end()) {
                m_CurrentTags.push_back(stdTag); // Добавляем тег в вектор
                UpdateDialogTags();              // Перерисовываем плашки на экране
            }
            m_TagInput->clear(); // Очищаем строку ввода для следующего тега
        }
        });
    
    // 5. Кнопки «Сохранить» и «Отмена»
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_SaveButton = new QPushButton("Сохранить", this);
    m_CancelButton = new QPushButton("Отмена", this);

    // Делаем кнопку сохранения активной по умолчанию (при нажатии Enter)
    m_SaveButton->setDefault(true);

    buttonLayout->addWidget(m_SaveButton);
    buttonLayout->addWidget(m_CancelButton);
    MainLayout->addLayout(buttonLayout);

    connect(m_AutoEncryptCheck, &QCheckBox::toggled, this, &CreateNoteDialog::HandleCheckBox);
    connect(m_SaveButton, &QPushButton::clicked, this, &CreateNoteDialog::HandleSave);
    connect(m_CancelButton, &QPushButton::clicked, this, &CreateNoteDialog::HandleCancel);
}

void CreateNoteDialog::UpdateDialogTags() {
    // Очищаем старые виджеты тегов с экрана диалога
    QLayoutItem* item;
    while ((item = m_TagsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    // Рисуем кнопки для текущих тегов
    for (size_t i = 0; i < m_CurrentTags.size(); ++i) {
        std::string tag = m_CurrentTags[i];

        // Добавляем к тексту крестик, чтобы пользователь понял: сюда можно кликнуть для удаления
        QString btnText = QString("%1  ×").arg(QString::fromStdString(tag));
        QPushButton* tagBtn = new QPushButton(btnText, this);
        tagBtn->setObjectName("DialogTagButton"); // ID для стилизации в QSS

        // Если пользователь кликает по тегу — удаляем его!
        connect(tagBtn, &QPushButton::clicked, this, [this, tag]() {
            // Ищем тег в векторе и удаляем его
            auto it = std::find(m_CurrentTags.begin(), m_CurrentTags.end(), tag);
            if (it != m_CurrentTags.end()) {
                m_CurrentTags.erase(it);
            }
            UpdateDialogTags(); // Мгновенно обновляем интерфейс окна
            });

        m_TagsLayout->addWidget(tagBtn);
    }

    // Прижимаем теги к левому краю окна
    m_TagsLayout->addStretch();
}
void CreateNoteDialog::HandleSave() {
    if (m_NameInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Название заметки не может быть пустым!");
        return;
    }
    accept();
}
 
void CreateNoteDialog::HandleCancel() {
    reject();
}


void CreateNoteDialog::HandleCheckBox(bool checked) {
    m_CustomKeyInput->setEnabled(!checked);

    if (checked) {
        m_CustomKeyInput->setPlaceholderText("Ключ заблокирован (включен авто-режим)");
        m_CustomKeyInput->clear();
    }
    else {
        m_CustomKeyInput->setPlaceholderText("Введите уникальный ключ для этой заметки...");
    }
}

CreateNoteDialog::~CreateNoteDialog(){

}

std::string CreateNoteDialog::GetName() const{
    return m_NameInput->text().trimmed().toStdString();
}

SecureString CreateNoteDialog::GetText() const {
    return QStringToSecureString(m_TextInput->toMarkdown().trimmed());
}
bool CreateNoteDialog::IsAutoEncryptChecked() const{
    return m_AutoEncryptCheck->isChecked();
}

SecureString CreateNoteDialog::GetCustomKey() const{
    return QStringToSecureString(m_CustomKeyInput->text().trimmed());
}