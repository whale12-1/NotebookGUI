#pragma once

#include <QWidget>

namespace WinThemeUtils {
    // Функция безопасно скомпилируется на любой ОС, но сработает только на Windows
    void ApplyModernBlur(QWidget* window);
}