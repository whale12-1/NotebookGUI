#include "CreateNoteDialog.h"
#include "WinThemeUtils.h"
#include "ModernMessage.h"
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
#include <QFileDialog> 
#include <QFileInfo>   

CreateNoteDialog::CreateNoteDialog(QWidget* parent) : QDialog(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    WinThemeUtils::ApplyModernBlur(this);
    setObjectName("CreateNoteDialog");
    setWindowTitle("Создание новой заметки");
    setMinimumWidth(480); // Немного увеличили для лучшей эргономики
    setMinimumHeight(600);

    QVBoxLayout* MainLayout = new QVBoxLayout(this);
    // Добавляем комфортные отступы от краев окна, чтобы интерфейс "дышал"
    MainLayout->setContentsMargins(20, 20, 20, 20);
    MainLayout->setSpacing(10);

    QLabel* NameLabel = new QLabel("Название заметки:", this);
    m_NameInput = new QLineEdit(this);
    m_NameInput->setPlaceholderText("Введите название...");
    MainLayout->addWidget(NameLabel);
    MainLayout->addWidget(m_NameInput);

    // --- НАСТРОЙКА MARKDOWN TOOLBAR ---
    QToolBar* mdToolBar = new QToolBar("Форматирование", this);
    mdToolBar->setObjectName("MarkdownToolBar");
    mdToolBar->setMovable(false);
    mdToolBar->setFloatable(false);

    // Стиль для самого тулбара и его внутренних кнопок-иконок
    mdToolBar->setStyleSheet(
        "QToolBar#MarkdownToolBar { "
        "   background: rgba(255, 255, 255, 0.03); "
        "   border: 1px solid rgba(255, 255, 255, 0.08); "
        "   border-radius: 8px; "
        "   padding: 4px; "
        "}"
        "QToolBar#MarkdownToolBar QToolButton { "
        "   background: transparent; "
        "   color: #e2e8f0; "
        "   border-radius: 4px; "
        "   font-weight: bold; "
        "   font-size: 12px; "
        "   padding: 4px 10px; "
        "   margin-right: 4px; "
        "}"
        "QToolBar#MarkdownToolBar QToolButton:hover { "
        "   background: rgba(255, 255, 255, 0.1); "
        "   color: #ffffff; "
        "}"
        "QToolBar#MarkdownToolBar QToolButton:pressed { "
        "   background: rgba(255, 255, 255, 0.05); "
        "}"
    );

    // Кнопка Жирный текст (Bold)
    QAction* boldAction = mdToolBar->addAction("B");
    boldAction->setToolTip("Жирный текст");
    connect(boldAction, &QAction::triggered, this, [this]() {
        QTextCursor cursor = m_TextInput->textCursor();
        if (!cursor.hasSelection()) return;

        QTextCharFormat format;
        format.setFontWeight(cursor.charFormat().fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        cursor.mergeCharFormat(format);
        });

    // Кнопка Курсив (Italic)
    QAction* italicAction = mdToolBar->addAction("I"); // Поменяли "/" на понятную "I"
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

    MainLayout->addWidget(mdToolBar);

    // 2. Поле ввода текста заметки
    QLabel* TextLabel = new QLabel("Содержимое (текст):", this);
    m_TextInput = new QTextEdit(this);

    QPalette textPal = m_TextInput->palette();
    textPal.setColor(QPalette::Base, Qt::transparent);
    m_TextInput->setPalette(textPal);
    // ---> ДОБАВЛЯЕМ ЭТИ ДВЕ СТРОКИ ДЛЯ БОРЬБЫ С БЕЛЫМ ФОНОМ <---
    m_TextInput->viewport()->setAutoFillBackground(false);
    m_TextInput->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);

    m_TextInput->setPlaceholderText("Напишите что-нибудь секретное...");
    MainLayout->addWidget(TextLabel);
    MainLayout->addWidget(m_TextInput);

    // Разделительная линия
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: rgba(255, 255, 255, 0.06); max-height: 1px; border: none;");
    MainLayout->addWidget(line);

    // 3. Выбор режима шифрования (Чекбокс)
    m_AutoEncryptCheck = new QCheckBox("Шифровать автоматически (Мастер-паролем)", this);
    m_AutoEncryptCheck->setChecked(true);
    MainLayout->addWidget(m_AutoEncryptCheck);

    // 4. Поле для кастомного ключа
    QLabel* customKeyLabel = new QLabel("Уникальный ключ для этой заметки:", this);
    m_CustomKeyInput = new QLineEdit(this);
    m_CustomKeyInput->setPlaceholderText("Ключ заблокирован (включен авто-режим)");
    m_CustomKeyInput->setEchoMode(QLineEdit::Password);
    m_CustomKeyInput->setEnabled(false);
    MainLayout->addWidget(customKeyLabel);
    MainLayout->addWidget(m_CustomKeyInput);

    // Разделитель перед тегами и вложениями
    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background-color: rgba(255, 255, 255, 0.06); max-height: 1px; border: none;");
    MainLayout->addWidget(line2);

    // ================= [ СЕКЦИЯ ТЕГОВ ] =================
    QLabel* TagLabel = new QLabel("Теги заметки:", this);
    MainLayout->addWidget(TagLabel);

    m_TagInput = new QLineEdit(this);
    m_TagInput->setPlaceholderText("Добавить тег (например: работа)...");

    m_AddTagButton = new QPushButton("+", this);
    m_AddTagButton->setFixedWidth(34);
    m_AddTagButton->setObjectName("AddTagButton");

    QHBoxLayout* tagInputLayout = new QHBoxLayout();
    tagInputLayout->setSpacing(8);
    tagInputLayout->addWidget(m_TagInput);
    tagInputLayout->addWidget(m_AddTagButton);
    MainLayout->addLayout(tagInputLayout);

    QWidget* tagsContainer = new QWidget(this);
    m_TagsLayout = new QHBoxLayout(tagsContainer);
    m_TagsLayout->setContentsMargins(0, 4, 0, 4);
    m_TagsLayout->setSpacing(6);
    MainLayout->addWidget(tagsContainer);

    connect(m_AddTagButton, &QPushButton::clicked, this, [this]() {
        QString tagText = m_TagInput->text().trimmed();
        if (!tagText.isEmpty()) {
            std::string stdTag = tagText.toStdString();
            if (std::find(m_CurrentTags.begin(), m_CurrentTags.end(), stdTag) == m_CurrentTags.end()) {
                m_CurrentTags.push_back(stdTag);
                UpdateDialogTags();
            }
            m_TagInput->clear();
        }
        });

    // ================= [ СЕКЦИЯ ВЛОЖЕНИЙ ] =================
    QLabel* AttachmentLabel = new QLabel("Зашифрованные вложения:", this);
    MainLayout->addWidget(AttachmentLabel);

    m_AttachmentPathDisplay = new QLineEdit(this);
    m_AttachmentPathDisplay->setReadOnly(true);
    m_AttachmentPathDisplay->setPlaceholderText("Выберите файл на компьютере...");

    m_BrowseAttachmentButton = new QPushButton("Обзор...", this);
    m_BrowseAttachmentButton->setObjectName("BrowseAttachmentButton");
    m_BrowseAttachmentButton->setFixedHeight(38); // Выравниваем по высоте с QLineEdit

    m_AddAttachmentButton = new QPushButton("+", this);
    m_AddAttachmentButton->setFixedWidth(34);
    m_AddAttachmentButton->setObjectName("AddAttachmentButton");

    QHBoxLayout* attachmentInputLayout = new QHBoxLayout();
    attachmentInputLayout->setSpacing(8);
    attachmentInputLayout->addWidget(m_AttachmentPathDisplay);
    attachmentInputLayout->addWidget(m_BrowseAttachmentButton);
    attachmentInputLayout->addWidget(m_AddAttachmentButton);
    MainLayout->addLayout(attachmentInputLayout);

    QWidget* attachmentsContainer = new QWidget(this);
    m_AttachmentsLayout = new QHBoxLayout(attachmentsContainer);
    m_AttachmentsLayout->setContentsMargins(0, 4, 0, 4);
    m_AttachmentsLayout->setSpacing(6);
    MainLayout->addWidget(attachmentsContainer);

    connect(m_BrowseAttachmentButton, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Выбрать файл для вложения");
        if (!filePath.isEmpty()) {
            m_AttachmentPathDisplay->setText(filePath);
        }
        });

    connect(m_AddAttachmentButton, &QPushButton::clicked, this, [this]() {
        QString filePath = m_AttachmentPathDisplay->text().trimmed();
        if (!filePath.isEmpty()) {
            QFileInfo fileInfo(filePath);
            if (fileInfo.exists()) {
                QString fileName = fileInfo.fileName();
                if (!m_PendingAttachments.contains(fileName)) {
                    m_PendingAttachments.insert(fileName, filePath);
                    UpdateDialogAttachments();
                }
                m_AttachmentPathDisplay->clear();
            }
            else {
                ModernMessageBox::showWarning(this, "Ошибка", "Указанный файл не существует!");
            }
        }
        });

    // ================= [ КНОПКИ УПРАВЛЕНИЯ ] =================
    QFrame* line3 = new QFrame(this);
    line3->setFrameShape(QFrame::HLine);
    line3->setStyleSheet("background-color: rgba(255, 255, 255, 0.06); max-height: 1px; border: none;");
    MainLayout->addWidget(line3);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    m_SaveButton = new QPushButton("Сохранить", this);
    m_CancelButton = new QPushButton("Отмена", this);
    m_SaveButton->setDefault(true);

    buttonLayout->addWidget(m_SaveButton);
    buttonLayout->addWidget(m_CancelButton);
    MainLayout->addLayout(buttonLayout);

    connect(m_AutoEncryptCheck, &QCheckBox::toggled, this, &CreateNoteDialog::HandleCheckBox);
    connect(m_SaveButton, &QPushButton::clicked, this, &CreateNoteDialog::HandleSave);
    connect(m_CancelButton, &QPushButton::clicked, this, &CreateNoteDialog::HandleCancel);
}

void CreateNoteDialog::UpdateDialogTags() {
    QLayoutItem* item;
    while ((item = m_TagsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    for (size_t i = 0; i < m_CurrentTags.size(); ++i) {
        std::string tag = m_CurrentTags[i];
        QString btnText = QString("%1  ×").arg(QString::fromStdString(tag));
        QPushButton* tagBtn = new QPushButton(btnText, this);
        tagBtn->setObjectName("TagButton");

        connect(tagBtn, &QPushButton::clicked, this, [this, tag]() {
            auto it = std::find(m_CurrentTags.begin(), m_CurrentTags.end(), tag);
            if (it != m_CurrentTags.end()) {
                m_CurrentTags.erase(it);
            }
            UpdateDialogTags();
            });

        m_TagsLayout->addWidget(tagBtn);
    }
    m_TagsLayout->addStretch();
}

void CreateNoteDialog::UpdateDialogAttachments() {
    QLayoutItem* item;
    while ((item = m_AttachmentsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    for (auto it = m_PendingAttachments.begin(); it != m_PendingAttachments.end(); ++it) {
        QString fileName = it.key();
        QString filePath = it.value();
        QFileInfo info(filePath);

        double sizeMb = static_cast<double>(info.size()) / (1024.0 * 1024.0);
        QString sizeStr = sizeMb < 0.1
            ? QString("%1 KB").arg(info.size() / 1024)
            : QString("%1 MB").arg(sizeMb, 0, 'f', 1);

        QString badgeText = QString("📎 %1 (%2)  ✕").arg(fileName, sizeStr);

        QPushButton* fileBtn = new QPushButton(badgeText, this);
        fileBtn->setObjectName("TagButton");
        fileBtn->setToolTip(filePath);

        connect(fileBtn, &QPushButton::clicked, this, [this, fileName]() {
            m_PendingAttachments.remove(fileName);
            UpdateDialogAttachments();
            });

        m_AttachmentsLayout->addWidget(fileBtn);
    }
    m_AttachmentsLayout->addStretch();
}

void CreateNoteDialog::HandleSave() {
    if (!IsAutoEncryptChecked() && m_CustomKeyInput->text().trimmed().isEmpty()) {
        ModernMessageBox::showWarning(this, "Предупреждение", "Ключ шифрования не может быть пустым!");
        return;
    }
    if (m_NameInput->text().trimmed().isEmpty()) {
        ModernMessageBox::showWarning(this, "Предупреждение", "Название заметки не может быть пустым!");
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

CreateNoteDialog::~CreateNoteDialog() {}

std::string CreateNoteDialog::GetName() const {
    return m_NameInput->text().trimmed().toStdString();
}

SecureString CreateNoteDialog::GetText() const {
    return QStringToSecureString(m_TextInput->toMarkdown().trimmed());
}

bool CreateNoteDialog::IsAutoEncryptChecked() const {
    return m_AutoEncryptCheck->isChecked();
}

SecureString CreateNoteDialog::GetCustomKey() const {
    return QStringToSecureString(m_CustomKeyInput->text().trimmed());
}

std::map<std::string, std::string> CreateNoteDialog::GetAllAttachments() const {
    std::map<std::string, std::string> res;
    for (auto it = m_PendingAttachments.constBegin(); it != m_PendingAttachments.constEnd();++it) {
        std::string key = it.key().toStdString();
        std::string value = it.value().toStdString();
        res[key] = value;
    }
    return res;
}