#include "MainWindow.h"
#include "AuthWindow.h"
// Разметки (Layouts) для красивого расположения кнопок
#include <QVBoxLayout>   
#include <QHBoxLayout>   
#include <QFile>
#include <QTextStream>
// Добавь эти три инклуда:
#include <QPushButton>
#include <QListWidget>
#include <QTimer>
#include <QClipboard>
#include <QApplication>

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
    resize(800, 600);
    QFile styleFile(":/Style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&styleFile);
        QString styleSheet = ts.readAll();

        // Применяем стили глобально ко всему окну и его дочерним элементам
        this->setStyleSheet(styleSheet);

        styleFile.close();
    }
    // 1. Создаем специальный центральный виджет-контейнер
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget); // Устанавливаем его главным в QMainWindow

    // 2. ИСПРАВЛЕНО: Привязываем QVBoxLayout к centralWidget, а не к this!
    QVBoxLayout* MainLayout = new QVBoxLayout(centralWidget);

    // 3. Добавляем элементы в слой как обычно
    QLabel* WelcomeLabel = new QLabel("Выберите заметку из списка и нажмите «Расшифровать»:", centralWidget);
    MainLayout->addWidget(WelcomeLabel);
    m_NotesView = new QListView(centralWidget);
    MainLayout->addWidget(m_NotesView);

    m_SearchField = new QLineEdit(centralWidget);
    m_SearchField->setPlaceholderText("🔍 Быстрый поиск заметок...");
    MainLayout->addWidget(m_SearchField);
    // Создаем горизонтальный контейнер для тегов
    QWidget* tagsContainer = new QWidget(centralWidget);
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
    // Кнопка "Расшифровать"
    m_DecryptButton = new QPushButton("🔑 Расшифровать выбранную заметку", centralWidget); // Вместо this
    MainLayout->addWidget(m_DecryptButton);
    // Кнопка "Добавить заметку"
    m_CreateButton = new QPushButton("➕ Добавить новую заметку", centralWidget); // Вместо this
    MainLayout->addWidget(m_CreateButton);
    // Кнопка "Удалить"
    m_DeleteButton = new QPushButton("Удалить заметку", centralWidget); // Вместо this
    m_DeleteButton->setStyleSheet("QPushButton { background-color: #d9534f; color: white; } QPushButton:hover { background-color: #c9302c; }");
    MainLayout->addWidget(m_DeleteButton);
    // Кнопка "Сменить Мастер-пароль"
    m_ChangePassButton = new QPushButton("🔒 Сменить Мастер-пароль", centralWidget); // Вместо this
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
    m_DecryptedDEK = DecryptedDek;
    m_ActualEncryptedDEK = EncryptedDek;
    m_DEKHash = DekHash;
    m_MasterPassword = MasterPassword;
    m_RandomSalt = Salt;
    // СВЕЖАЯ ЛОГИКА: Проверяем цепочку хэшей перед выводом данных на экран!
    if (!DatabaseManager::instance().ValidateIntegrity()) {
        QMessageBox::critical(this, "Критическая ошибка",
            "Обнаружено несанкционированное изменение данных Сейфа (цепочка хэшей разорвана). Доступ заблокирована!");
        QTimer::singleShot(0, this, &QWidget::close); // Безопасно закрываем приложение
        return;
    }
    m_NotesModel->PopulateFromDatabase();

    // Твой старый код отрисовки интерфейса и запуска таймера активности...
    UpdateTagsPanel();
}
void MainWindow::HandleInactivityLockOut() {
    // 1. БЕЗОПАСНОСТЬ: Перед закрытием затираем мастер-пароль и DEK в памяти!
    // Допустим, они у тебя хранятся в переменных m_DecryptedDEK и m_MasterPassword
    if (!m_DecryptedDEK.empty()) {
        OPENSSL_cleanse(&m_DecryptedDEK[0], m_DecryptedDEK.length());
    }
    if (!m_MasterPassword.empty()) {
        OPENSSL_cleanse(&m_MasterPassword[0], m_MasterPassword.length());
    }
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
    // 1. Открываем твой диалог создания заметки
    CreateNoteDialog Dialog(this);
    if (Dialog.exec() == QDialog::Accepted) {
        // 2. Собираем данные из полей диалога
        std::string NoteName = Dialog.GetName();
        SecureString RawText = Dialog.GetText();
        std::vector<std::string> NoteTags = Dialog.GetTags();
        SecureString Key = Dialog.IsAutoEncryptChecked() ? m_DecryptedDEK : Dialog.GetCustomKey();
        std::vector<unsigned char> RandomSaltVector(m_RandomSalt.begin(), m_RandomSalt.end());
        std::string EncryptedText;
        try {
            EncryptedText = EncryptAES(RawText,Key,RandomSaltVector);
        }
        catch(...){
            QMessageBox::critical(this, "Ошибка", "Не удалось зашифровать заметку.");
            return;
        }
        // 4. Создаем объект структуры в новом стиле
        Note NewNote;
        NewNote.Name = NoteName;
        NewNote.SecuredText = EncryptedText;
        NewNote.Tags = NoteTags;

        // 5. Просто отдаем заметку в модель! 
        // Модель сама вызовет DatabaseManager, сохранит всё в SQLite и нарисует на экране.
        if (m_NotesModel->AddNote(NewNote)) {
            QMessageBox::information(this, "Успех", "Заметка сохранена в базу!");
            UpdateTagsPanel(); // Обновляем пастельные кнопочки тегов на панели
        }
        else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить заметку в базу данных.");
        }
    }
}

void MainWindow::HandleDeleteNote() {
    // 1. Получаем индекс строки, на которую кликнули в интерфейсе
    QModelIndex SelectedIndex = m_NotesView->currentIndex();

    if (!SelectedIndex.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите заметку для удаления!");
        return;
    }

    // 2. ЗАЩИТА ОТ ФИЛЬТРАЦИИ: Переводим прокси-индекс в реальный индекс исходной модели
    QModelIndex SourceIndex = m_ProxyModel->mapToSource(SelectedIndex);
    int TargetRow = SourceIndex.row();

    // 3. Запрашиваем подтверждение у пользователя
    QMessageBox::StandardButton Reply;
    Reply = QMessageBox::question(this, "Подтверждение", "Вы уверены, что хотите удалить эту заметку?",
        QMessageBox::Yes | QMessageBox::No);

    if (Reply == QMessageBox::Yes) {
        // 4. Передаем в модель ПРАВИЛЬНУЮ строку
        if (m_NotesModel->RemoveNote(TargetRow)) {
            UpdateTagsPanel();
            QMessageBox::information(this, "Успех", "Заметка успешно удалена!");
        }
        else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить запись из базы данных.");
        }
    }
}
void MainWindow::HandleDecryptNote() {
    QModelIndex currentIndex = m_NotesView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, выберите заметку из списка!");
        return;
    }
    QModelIndex sourceIndex = m_ProxyModel->mapToSource(currentIndex);

    int currentRow = sourceIndex.row();

    DecryptNoteDialog Dialog(this);
    if (Dialog.exec() == QDialog::Accepted) {
        SecureString DecryptingKey = Dialog.IsExtraKeyNeeded() ? Dialog.GetExtraKey() : m_DecryptedDEK;
        std::vector<unsigned char> RandomSaltVector(m_RandomSalt.begin(), m_RandomSalt.end());
        try {
            SecureString decryptedText = DecryptAES(m_UserNotes[currentRow].SecuredText, DecryptingKey, RandomSaltVector);
            QString msg = QString("Название: %1\n\nСодержимое заметки:\n%2")
                .arg(QString::fromStdString(m_UserNotes[currentRow].Name))
                .arg(QString::fromLocal8Bit(decryptedText.c_str()));

            QMessageBox::information(this, "Просмотр заметки", msg);
        }
        catch (const std::exception& e) {
            QMessageBox::critical(this, "Ошибка", "Неверный ключ шифрования или данные повреждены!");
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
                    QMessageBox::critical(this, "Ошибка", "Старый мастер-пароль введен неверно!");
                    return;
                }
                OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
            }
            catch (...) {
                QMessageBox::critical(this, "Ошибка", "Старый мастер-пароль введен неверно!");
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
                QMessageBox::warning(this, "Ошибка", "Не удалось обновить крипто-ключи в базе данных.");
                return;
            }
            // 4. Обновляем переменные в памяти
            m_ActualEncryptedDEK = newEncryptedDEK;
            m_DecryptedDEK = testDecryptedDEK; // Теперь глобальный DEK гарантированно живой
            std::string dekForHash(testDecryptedDEK.data(), testDecryptedDEK.length());
            m_DEKHash = CalculateSHA256(dekForHash);
            OPENSSL_cleanse(&dekForHash[0], dekForHash.length());
            m_MasterPassword = newPassword;

            QMessageBox::information(this, "Успех", "Мастер-пароль успешно изменен!");
        }
        catch (const std::exception& e) {
            QMessageBox::critical(this, "Критическая ошибка",
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