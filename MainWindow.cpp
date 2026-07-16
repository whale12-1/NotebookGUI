#include "MainWindow.h"
#include "AuthWindow.h"
#include "WinThemeUtils.h"
#include "SettingsDialog.h"
#include "ModernMessage.h"
// Разметки (Layouts) для красивого расположения кнопок
#include <QVBoxLayout>   
#include <QHBoxLayout>   
#include <QFile>
#include <QTextStream>
#include <QTextEdit>
// Добавь эти три инклуда:
#include <QPushButton>
#include <QListWidget>
#include <QTimer>
#include <QClipboard>
#include <QApplication>
#include <QUuid>      // Для генерации случайных уникальных имен файлов
#include <QDir>       // Для создания папки под вложения
#include <QFileDialog>// Для выбора папки сохранения расшифрованного файла
#include <openssl/rand.h>
// Виджеты для отображения текста и всплывающих окон
#include <QLabel>        
#include <QMessageBox>   

// Твои кастомные окна и менеджеры
#include "CreateNoteDialog.h" 
#include "DecryptDialog.h"
#include "ChangePass.h"

// Стандартные библиотеки для работы с потоками и хэшами (если они нужны внутри методов класса)
#include <fstream>
#include <cstdio>        // Для std::remove и std::rename при сохранении файла

bool MainWindow::event(QEvent* event) {
    // Если происходит движение мыши, клик или нажатие клавиши — сбрасываем таймер
    if (event->type() == QEvent::MouseMove ||
        event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::KeyPress)
    {
        if (m_InactivityTimer && m_InactivityTimer->isActive()) {
            m_InactivityTimer->start(); // Перезапуск таймера заново на 5 минут
        }
    }
    return QMainWindow::event(event); // Передаем событие дальше штатным виджетам
}


MainWindow::MainWindow(QWidget* parent) :QMainWindow(parent), m_FileManager("data.txt") {
    setWindowTitle("Твой Сейф Заметок (Завершено)");
    this->setObjectName("MainWindowInstance");
    resize(800, 600);

    // Включаем прозрачность для главного окна, чтобы работал Mica
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    WinThemeUtils::ApplyModernBlur(this);

    // Главный контейнер
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Чтобы центральный виджет не имел своего фона
    centralWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    // Слой для центрального виджета, который растянет нашу "карточку" на все окно с красивыми отступами
    QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0); // Отступы от краев экрана до нашей панели

    // --- НАША КРАСИВАЯ ПОДЛОЖКА (КАНВАС) ---
    QWidget* mainCanvas = new QWidget(centralWidget);
    mainCanvas->setObjectName("MainCanvas"); // Зададим имя для QSS
    rootLayout->addWidget(mainCanvas);

    // Теперь всю внутреннюю разметку вешаем на mainCanvas!
    QVBoxLayout* MainLayout = new QVBoxLayout(mainCanvas);
    MainLayout->setContentsMargins(20, 20, 20, 20); // Внутренние отступы внутри сейфа
    MainLayout->setSpacing(12);

    // Переносим добавление виджетов на mainCanvas (вместо centralWidget)
    QLabel* WelcomeLabel = new QLabel("Выберите заметку из списка и нажмите «Расшифровать»:", mainCanvas);
    MainLayout->addWidget(WelcomeLabel);

    m_NotesView = new QListView(mainCanvas);
    MainLayout->addWidget(m_NotesView);

    m_SearchField = new QLineEdit(mainCanvas);
    m_SearchField->setPlaceholderText("🔍 Быстрый поиск заметок...");
    MainLayout->addWidget(m_SearchField);
    // Создаем горизонтальный контейнер для тегов
    QWidget* tagsContainer = new QWidget(mainCanvas);
    m_TagsLayout = new QHBoxLayout(tagsContainer);
    m_TagsLayout->setContentsMargins(0, 0, 0, 0);
    m_TagsLayout->setSpacing(6); // Расстояние между кнопками-тегами

    // Добавляем контейнер тегов в главный вертикальный Layout
    MainLayout->addWidget(tagsContainer);

    // 2. Создаем Модель и передаем ей указатель на вектор заметок m_UserNotes
    m_NotesModel = new NotesModel(&m_UserNotes, this);
    m_ProxyModel = new CustomFilterProxyModel(this);

    // 4. Настраиваем Прокси-модель: говорим, откуда брать исходные данные
    m_ProxyModel->setSourceModel(m_NotesModel);

    // [Опционально] Говорим прокси-модели игнорировать регистр букв (чтобы "План" и "план" давали один результат)
    m_ProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // 3. Соединяем их вместе! Теперь View смотрит на мир через призму нашей модели
    m_NotesView->setModel(m_ProxyModel);

    m_SettingsBtn = new QPushButton(this);
    m_SettingsBtn->setObjectName("Settings2FABtn");
    m_SettingsBtn->setToolTip("Настройки двухфакторной аутентификации (2FA)");
    m_SettingsBtn->setFixedSize(24, 24);
    m_SettingsBtn->setText("🛡️");

    QHBoxLayout* topBarLayout = new QHBoxLayout();
    topBarLayout->setContentsMargins(5, 5, 5, 0); // Небольшие отступы сверху и сбоку
    topBarLayout->addWidget(m_SettingsBtn, 0, Qt::AlignLeft); // Прижимаем строго влево
    topBarLayout->addStretch();

    MainLayout->insertLayout(0, topBarLayout);
    // Кнопка "Расшифровать"
    m_DecryptButton = new QPushButton("🔑 Расшифровать выбранную заметку", mainCanvas); // Вместо this
    MainLayout->addWidget(m_DecryptButton);
    // Кнопка "Добавить заметку"
    m_CreateButton = new QPushButton("➕ Добавить новую заметку",mainCanvas); // Вместо this
    MainLayout->addWidget(m_CreateButton);
    // Кнопка "Удалить"
    m_DeleteButton = new QPushButton("Удалить заметку", mainCanvas);
    m_DeleteButton->setObjectName("m_DeleteButton");
    MainLayout->addWidget(m_DeleteButton);
    // Кнопка "Сменить Мастер-пароль"
    m_ChangePassButton = new QPushButton("🔒 Сменить Мастер-пароль", mainCanvas); // Вместо this
    MainLayout->addWidget(m_ChangePassButton);

    m_ClipboardTimer = new QTimer(this);
    m_ClipboardTimer->setInterval(30000); // 30 секунд в мс
    m_ClipboardTimer->setSingleShot(true);

    m_InactivityTimer = new QTimer(this);
    m_InactivityTimer->setInterval(300000); // 5 минут в мс
    m_InactivityTimer->setSingleShot(true);

    connect(m_ClipboardTimer, &QTimer::timeout, this, &MainWindow::ClearSecureClipboard);
    connect(m_CreateButton, &QPushButton::clicked, this, &MainWindow::HandleAddNote);
    connect(m_DecryptButton, &QPushButton::clicked, this, &MainWindow::HandleDecryptNote);
    connect(m_DeleteButton, &QPushButton::clicked, this, &MainWindow::HandleDeleteNote);
    connect(m_ChangePassButton, &QPushButton::clicked, this, &MainWindow::HandleChangeMasterPassword);
    connect(m_InactivityTimer, &QTimer::timeout, this, &MainWindow::HandleInactivityLockOut);
    // Когда текст в строке поиска меняется, прокси-модель автоматически применяет его как фильтр
    connect(m_SearchField, &QLineEdit::textChanged, m_ProxyModel, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_SettingsBtn, &QPushButton::clicked, this, &HandleBtnSettingsClicked);

    this->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    WinThemeUtils::ApplyModernBlur(this);

}

void MainWindow::OnCopySecretClicked(const QString& secretText) {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(secretText);

    m_LastCopiedSecret = secretText; // Запоминаем, какой именно секрет ушел в буфер
    m_ClipboardTimer->start();       // Запускаем счетчик на 30 секунд
}

//void MainWindow::RefreshNoteList() {
//    m_NotesList->clear();
//    if (m_UserNotes.empty()) {
//        m_NotesList->addItem("У вас пока нет сохраненных заметок.");
//    }
//    else {
//        for (size_t i = 0; i < m_UserNotes.size(); ++i) {
//            QString itemText = QString("[%1] %2")
//                .arg(i + 1)
//                .arg(QString::fromStdString(m_UserNotes[i].Name));
//            m_NotesList->addItem(itemText);
//        }
//    }
//};

MainWindow::~MainWindow() {

}

void MainWindow::UpdateTagsPanel() {
    // 1. Очищаем старые кнопки из m_TagsLayout
    QLayoutItem* item;
    while ((item = m_TagsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    // 2. Собираем только уникальные теги со всех заметок
    std::set<QString> uniqueTags;
    for (const auto& note : m_UserNotes) {
        for (const auto& tag : note.Tags) {
            uniqueTags.insert(QString::fromStdString(tag));
        }
    }

    // 3. Всегда первой добавляем кнопку "Все", чтобы сбросить фильтр
    uniqueTags.insert("Все");

    // 4. Создаем кнопки для каждого тега
    for (const QString& tag : uniqueTags) {
        QPushButton* tagBtn = new QPushButton(tag, this);
        tagBtn->setObjectName("TagButton"); // Для стилизации в QSS

        // Связываем клик по кнопке с фильтрацией
        connect(tagBtn, &QPushButton::clicked, this, [this, tag]() {
            if (tag == "Все") {
                m_ProxyModel->setSelectedTag(""); // Сброс фильтра
            }
            else {
                m_ProxyModel->setSelectedTag(tag); // Фильтр по тегу
            }
            });

        m_TagsLayout->addWidget(tagBtn);
    }

    // Добавляем пружину в конец, чтобы кнопки прижимались к левому краю
    m_TagsLayout->addStretch();
}

// В MainWindow.cpp
void MainWindow::InitSession(const SecureString& DecryptedDek,
    const std::string& EncryptedDek,
    const std::string& Salt,
    const SecureString& MasterPassword,
    const std::string& DekHash)
{
    m_DecryptedDEK = std::make_unique<ProtectedKey>(DecryptedDek);
    m_ActualEncryptedDEK = EncryptedDek;
    m_DEKHash = DekHash;
    m_MasterPassword = std::make_unique<ProtectedKey>(MasterPassword);
    m_RandomSalt = Salt;
    m_NotesModel->PopulateFromDatabase();

    // Твой старый код отрисовки интерфейса и запуска таймера активности...
    UpdateTagsPanel();
}
void MainWindow::HandleInactivityLockOut() {
    // 1. БЕЗОПАСНОСТЬ: Перед закрытием затираем мастер-пароль и DEK в памяти!
    // Допустим, они у тебя хранятся в переменных m_DecryptedDEK и m_MasterPassword
    m_DecryptedDEK.reset();
    m_MasterPassword.reset();
    for (auto& note : m_UserNotes) {
        OPENSSL_cleanse(&note.Name[0], note.Name.length());
        OPENSSL_cleanse(&note.SecuredText[0], note.SecuredText.length());
    }
    m_UserNotes.clear();
    AuthWindow* authWin = new AuthWindow();
    authWin->show();

    this->close();
}
void MainWindow::HandleAddNote() {
    CreateNoteDialog Dialog(this);
    if (Dialog.exec() == QDialog::Accepted) {
        std::string NoteName = Dialog.GetName();
        SecureString RawText = Dialog.GetText();
        std::vector<std::string> NoteTags = Dialog.GetTags();

        // ИСПРАВЛЕНО: Вызываем правильный метод GetAllAttachments()
        std::map<std::string, std::string> Attachments = Dialog.GetAllAttachments();

        SecureString Key = Dialog.IsAutoEncryptChecked() ? m_DecryptedDEK->data() : Dialog.GetCustomKey();
        std::vector<unsigned char> RandomSaltVector(m_RandomSalt.begin(), m_RandomSalt.end());

        std::string EncryptedText;
        try {
            EncryptedText = EncryptAES(RawText, Key, RandomSaltVector);
        }
        catch (...) {
            ModernMessageBox::showCritical(this, "Ошибка", "Не удалось зашифровать заметку.");
            return;
        }

        Note NewNote;
        NewNote.Name = NoteName;
        NewNote.SecuredText = EncryptedText;
        NewNote.Tags = NoteTags;

        for (const auto& it : Attachments) {
            try {
                SecureString Name(it.first.begin(), it.first.end());

                // ИСПРАВЛЕНО: Используем std::vector вместо сырого массива для OpenSSL и структуры
                std::vector<unsigned char> ivVec(16);
                if (!(RAND_bytes(ivVec.data(), 16) == 1)) {
                    throw std::runtime_error("Не удалось сгенерировать вспомогательный вектор iv");
                }

                std::string EncryptedName = EncryptAES(Name, Key, RandomSaltVector);
                QString DiskName = QUuid::createUuid().toString(QUuid::WithoutBraces) + ".dat";

                QDir().mkpath("./attachments");
                QString destPath = "./attachments/" + DiskName;

                // ИСПРАВЛЕНО: Исправлено имя функции, передан Key и корректный тип ivVec
                unsigned int FileSize = EncryptFileStream(it.second, destPath.toStdString(), Key, RandomSaltVector, ivVec);
                if (FileSize == 0) {
                    QMessageBox::warning(this, "Ошибка", "Ошибка при записи файла вложения.");
                    return;
                }

                // ИСПРАВЛЕНО: В DiskFileName кладем только DiskName.toStdString() (без папки)
                // Передаем ivVec, соответствующий типу структуры
                NewNote.Attachments.push_back({ EncryptedName, DiskName.toStdString(), ivVec, FileSize });
            }
            catch (...) {
                ModernMessageBox::showCritical(this, "Ошибка", "Критический сбой при обработке вложения.");
                return;
            }
        }

        if (m_NotesModel->AddNote(NewNote)) {
            ModernMessageBox::showInfo(this, "Успех", "Заметка успешно создана и сохранена в базу!");
            UpdateTagsPanel();
        }
        else {
            ModernMessageBox::showCritical(this, "Ошибка", "Не удалось сохранить заметку в базу данных.");
        }
    }
}

void MainWindow::HandleDeleteNote() {
    // 1. Получаем индекс строки, на которую кликнули в интерфейсе
    QModelIndex SelectedIndex = m_NotesView->currentIndex();

    if (!SelectedIndex.isValid()) {
        ModernMessageBox::showWarning(this, "Ошибка", "Выберите заметку для удаления!");
        return;
    }

    // 2. ЗАЩИТА ОТ ФИЛЬТРАЦИИ: Переводим прокси-индекс в реальный индекс исходной модели
    QModelIndex SourceIndex = m_ProxyModel->mapToSource(SelectedIndex);
    int TargetRow = SourceIndex.row();

    // 3. Запрашиваем подтверждение у пользователя
    bool Reply = ModernMessageBox::showQuestion(this, "Подтверждение", "Вы уверены, что хотите удалить эту заметку?");

    if (Reply) {
        // 4. Передаем в модель ПРАВИЛЬНУЮ строку
        if (m_NotesModel->RemoveNote(TargetRow)) {
            UpdateTagsPanel();
            ModernMessageBox::showInfo(this, "Успех", "Заметка успешно удалена!");
        }
        else {
            ModernMessageBox::showWarning(this, "Ошибка", "Не удалось удалить запись из базы данных.");
        }
    }
}
void MainWindow::HandleDecryptNote() {
    QModelIndex currentIndex = m_NotesView->currentIndex();
    if (!currentIndex.isValid()) {
        ModernMessageBox::showWarning(this, "Внимание", "Пожалуйста, выберите заметку из списка!");
        return;
    }
    QModelIndex sourceIndex = m_ProxyModel->mapToSource(currentIndex);
    int currentRow = sourceIndex.row();

    DecryptNoteDialog Dialog(this);
    if (Dialog.exec() == QDialog::Accepted) {
        SecureString DecryptingKey = Dialog.IsExtraKeyNeeded() ? Dialog.GetExtraKey() : m_DecryptedDEK->data();
        std::vector<unsigned char> RandomSaltVector(m_RandomSalt.begin(), m_RandomSalt.end());
        try {
            SecureString decryptedText = DecryptAES(m_UserNotes[currentRow].SecuredText, DecryptingKey, RandomSaltVector);

            QDialog* viewDialog = new QDialog(this);
            viewDialog->setObjectName("DecryptViewDialog");
            viewDialog->setWindowTitle(QString::fromStdString(m_UserNotes[currentRow].Name));
            viewDialog->setMinimumSize(480, 500); // Оптимальный размер

            // Настройка прозрачности и размытия Mica
            viewDialog->setAttribute(Qt::WA_TranslucentBackground, true);
            viewDialog->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
            WinThemeUtils::ApplyModernBlur(viewDialog);

            // Базовый слой без отступов
            QVBoxLayout* rootLayout = new QVBoxLayout(viewDialog);
            rootLayout->setContentsMargins(0, 0, 0, 0);

            // Подложка-канвас
            QWidget* mainCanvas = new QWidget(viewDialog);
            mainCanvas->setObjectName("MainCanvas");
            rootLayout->addWidget(mainCanvas);

            // Внутренняя разметка на канвасе
            QVBoxLayout* layout = new QVBoxLayout(mainCanvas);
            layout->setContentsMargins(24, 24, 24, 24);
            layout->setSpacing(12);

            QTextEdit* viewer = new QTextEdit(mainCanvas);
            viewer->setObjectName("DecryptViewer");
            viewer->setReadOnly(true);
            viewer->setMarkdown(QString::fromLocal8Bit(decryptedText.c_str()));

            // Сбрасываем стандартную палитру QTextEdit на прозрачную
            QPalette viewPal = viewer->palette();
            viewPal.setColor(QPalette::Base, Qt::transparent);
            viewer->setPalette(viewPal);
            viewer->viewport()->setAutoFillBackground(false);
            viewer->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);

            layout->addWidget(viewer);

            // === Отображение и дешифрование вложений ===
            const Note& currentNote = m_UserNotes[currentRow];
            if (!currentNote.Attachments.empty()) {
                QLabel* attachLabel = new QLabel("📎 Доступные вложения (нажмите для сохранения):", mainCanvas);
                attachLabel->setStyleSheet("font-weight: bold; margin-top: 5px; color: #94a3b8;");
                layout->addWidget(attachLabel);

                QHBoxLayout* attachLayout = new QHBoxLayout();
                attachLayout->setSpacing(8);

                for (const auto& attach : currentNote.Attachments) {
                    std::string decryptedFileNameStr;
                    try {
                        decryptedFileNameStr = DecryptAES(attach.OriginalNameEncrypted, DecryptingKey, RandomSaltVector);
                    }
                    catch (...) {
                        decryptedFileNameStr = "unknown_file";
                    }
                    QString decryptedFileName = QString::fromStdString(decryptedFileNameStr);

                    QPushButton* fileBtn = new QPushButton(QString("💾 %1").arg(decryptedFileName), mainCanvas);
                    fileBtn->setObjectName("TagButton");

                    connect(fileBtn, &QPushButton::clicked, this, [this, attach, decryptedFileName, DecryptingKey, RandomSaltVector]() {
                        QString savePath = QFileDialog::getSaveFileName(this, "Сохранить файл как...", decryptedFileName);
                        if (!savePath.isEmpty()) {
                            QString encryptedFilePath = QString("./attachments/%1").arg(QString::fromStdString(attach.DiskFileName));
                            try {
                                if (DecryptFileStream(encryptedFilePath.toStdString(), savePath.toStdString(), DecryptingKey, RandomSaltVector, attach.Iv)) {
                                    ModernMessageBox::showInfo(this, "Успех", "Файл успешно сохранен!");
                                }
                                else {
                                    ModernMessageBox::showCritical(this, "Ошибка", "Не удалось расшифровать файл.");
                                }
                            }
                            catch (const std::exception& e) {
                                ModernMessageBox::showCritical(this, "Ошибка", QString("Ошибка дешифрования: %1").arg(e.what()));
                            }
                        }
                        });

                    attachLayout->addWidget(fileBtn);
                }
                attachLayout->addStretch();
                layout->addLayout(attachLayout);
            }

            QPushButton* closeBtn = new QPushButton("Закрыть", mainCanvas);
            closeBtn->setObjectName("DecryptCloseBtn");
            connect(closeBtn, &QPushButton::clicked, viewDialog, &QDialog::accept);
            layout->addWidget(closeBtn);

            viewDialog->setStyleSheet(this->styleSheet());
            viewDialog->exec();
            delete viewDialog;
        }
        catch (const std::exception& e) {
            ModernMessageBox::showCritical(this, "Ошибка", "Неверный ключ шифрования или данные повреждены!");
        }
    }
}

void MainWindow::HandleChangeMasterPassword() {
    ChangePassDialog Dialog(this);
    if (Dialog.exec() == QDialog::Accepted) {
        SecureString oldPassword = Dialog.GetOldPass();
        SecureString newPassword = Dialog.GetNewPass();
        std::vector<unsigned char> RandomSaltVector(m_RandomSalt.begin(), m_RandomSalt.end());
        try {
            SecureString testDecryptedDEK;
            try {
                testDecryptedDEK = DecryptAES(m_ActualEncryptedDEK, oldPassword, RandomSaltVector);
                std::string dekForHash(testDecryptedDEK.data(), testDecryptedDEK.length());
                if (CalculateSHA256(dekForHash) != m_DEKHash) {
                    OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
                    ModernMessageBox::showCritical(this, "Ошибка", "Старый мастер-пароль введен неверно!");
                    return;
                }
                OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
            }
            catch (...) {
                ModernMessageBox::showCritical(this, "Ошибка", "Старый мастер-пароль введен неверно!");
                return;
            }
            // 2. Зашифровываем ТОТ ЖЕ САМЫЙ DEK с использованием НОВОГО пароля,
            // но сохраняя текущую соль файла, чтобы не сломать ключи существующих заметок!
            std::string newEncryptedDEK = EncryptAES(testDecryptedDEK, newPassword, RandomSaltVector);
            // 3. Запись на диск
            if (!DatabaseManager::instance().SaveCryptoKeys(
                QString::fromStdString(m_RandomSalt),
                QString::fromStdString(newEncryptedDEK),
                QString::fromStdString(m_DEKHash)))
            {
                ModernMessageBox::showWarning(this, "Ошибка", "Не удалось обновить крипто-ключи в базе данных.");
                return;
            }
            // 4. Обновляем переменные в памяти
            m_ActualEncryptedDEK = newEncryptedDEK;
            m_DecryptedDEK.reset(new ProtectedKey(testDecryptedDEK)); // Теперь глобальный DEK гарантированно живой
            std::string dekForHash(testDecryptedDEK.data(), testDecryptedDEK.length());
            m_DEKHash = CalculateSHA256(dekForHash);
            OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
            m_MasterPassword.reset(new ProtectedKey(newPassword));

            ModernMessageBox::showInfo(this, "Успех", "Мастер-пароль успешно изменен!");
        }
        catch (const std::exception& e) {
            ModernMessageBox::showCritical(this, "Критическая ошибка",
                QString("Сбой при перешифровании: %1").arg(e.what()));
        }
    }
}

void MainWindow::ClearSecureClipboard() {
    QClipboard* clipboard = QApplication::clipboard();

    // Проверяем, что в буфере лежит именно то, что скопировали мы
    if (clipboard->text() == m_LastCopiedSecret) {
        clipboard->clear(); // Очищаем буфер системы
    }

    // Стираем сам секрет из переменной отслеживания ради безопасности
    if (!m_LastCopiedSecret.isEmpty()) {
        // Переводим в std::string и затираем байты через OpenSSL
        QByteArray bytes = m_LastCopiedSecret.toUtf8();
        OPENSSL_cleanse(bytes.data(), bytes.size());
        m_LastCopiedSecret.clear();
    }
}


void MainWindow::HandleBtnSettingsClicked() {
    SettingsDialog Settings(m_UserNotes,m_DecryptedDEK->data(), m_RandomSalt, this);
    Settings.setStyleSheet(this->styleSheet());
    Settings.exec();
}