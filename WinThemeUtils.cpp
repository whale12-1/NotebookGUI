#include "WinThemeUtils.h"

#ifdef _WIN32
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#endif

namespace WinThemeUtils {
    void ApplyModernBlur(QWidget* window) {
#ifdef _WIN32
        if (!window) return;

        HWND hwnd = (HWND)window->winId();

        // 1. Включаем темную тему для системной рамки
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

        // 2. Активируем Mica-бэкдроп
        DWORD backdropType = 3; // 3 = Mica, 4 = Acrylic
        DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));

        // 3. Расширяем системную рамку на все окно
        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);
#else
        Q_UNUSED(window);
#endif
    }
}