#pragma once
#include <string>
#include <algorithm>
#include <cstddef>
#include <new>

template <class T>
struct SecureAllocator {
    // 1. Обязательные типы данных, которые требует стандарт C++
    using value_type = T;

    SecureAllocator() noexcept {}

    // Конструктор копирования для приведения аллокаторов разных типов
    template <class U>
    SecureAllocator(const SecureAllocator<U>&) noexcept {}

    // 2. Метод выделения "сырой" памяти
    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;

        // Выделяем память через глобальный оператор new
        void* p = ::operator new(n * sizeof(T));
        if (!p) throw std::bad_alloc();

        return static_cast<T*>(p);
    }

    // 3. Метод освобождения памяти с гарантированным затиранием данных
    void deallocate(T* p, std::size_t n) noexcept {
        if (!p) return;

        // volatile и void* нужны, чтобы компилятор при оптимизации (-O2/-O3) 
        // не вздумал "выбросить" этот цикл, посчитав его бесполезным.
        volatile char* v_ptr = static_cast<volatile char*>(static_cast<void*>(p));
        std::fill_n(v_ptr, n * sizeof(T), 0);

        // Освобождаем память на уровне ОС
        ::operator delete(p);
    }

    // Операторы сравнения (обязательны для стандарта)
    // Возвращают true, так как у нашего аллокатора нет внутреннего состояния (state),
    // и память, выделенная одним экземпляром, может быть освобождена другим.
    template <class U>
    bool operator==(const SecureAllocator<U>&) const noexcept { return true; }

    template <class U>
    bool operator!=(const SecureAllocator<U>&) const noexcept { return false; }
};

// 4. Объявляем удобный alias (псевдоним) для безопасной строки
using SecureString = std::basic_string<char, std::char_traits<char>, SecureAllocator<char>>;

