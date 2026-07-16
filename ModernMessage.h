#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include "WinThemeUtils.h"
#include "QVBoxLayout"

class ModernMessageBox : public QDialog {
    Q_OBJECT
public:
    enum IconType { Info, Warning, Critical, Question };

    ModernMessageBox(QWidget* parent, const QString& title, const QString& text, IconType iconType = Info, bool showCancel = false)
        : QDialog(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground, true);
        setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);

        // Внешние зависимости проекта для применения Mica
        WinThemeUtils::ApplyModernBlur(this);

        setWindowTitle(title);
        setFixedSize(380, 180);

        QVBoxLayout* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        QWidget* mainCanvas = new QWidget(this);
        mainCanvas->setObjectName("MainCanvas");
        rootLayout->addWidget(mainCanvas);

        QVBoxLayout* layout = new QVBoxLayout(mainCanvas);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        // Иконка-текст для простоты и стиля
        QString iconPrefix;
        if (iconType == Info) iconPrefix = "ℹ️  ";
        else if (iconType == Warning) iconPrefix = "⚠️  ";
        else if (iconType == Critical) iconPrefix = "❌  ";
        else if (iconType == Question) iconPrefix = "❓  ";

        QLabel* label = new QLabel(iconPrefix + text, mainCanvas);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(10);

        if (showCancel || iconType == Question) {
            QPushButton* cancelBtn = new QPushButton(iconType == Question ? "Нет" : "Отмена", mainCanvas);
            btnLayout->addWidget(cancelBtn);
            connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        }

        QPushButton* okBtn = new QPushButton(iconType == Question ? "Да" : "ОК", mainCanvas);
        okBtn->setDefault(true);
        btnLayout->addWidget(okBtn);
        connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);

        layout->addLayout(btnLayout);
    }

    static bool showQuestion(QWidget* parent, const QString& title, const QString& text) {
        ModernMessageBox box(parent, title, text, Question);
        return box.exec() == QDialog::Accepted;
    }

    static void showInfo(QWidget* parent, const QString& title, const QString& text) {
        ModernMessageBox box(parent, title, text, Info);
        box.exec();
    }

    static void showWarning(QWidget* parent, const QString& title, const QString& text) {
        ModernMessageBox box(parent, title, text, Warning);
        box.exec();
    }

    static void showCritical(QWidget* parent, const QString& title, const QString& text) {
        ModernMessageBox box(parent, title, text, Critical);
        box.exec();
    }
};