#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>

template <typename T>
class Vector {
public:
    Vector() = default;

    explicit Vector(size_t size)
        : data_(Allocate(size))
        , capacity_(size)
        , size_(size)
    {
        size_t i = 0;
        try {
            for (; i != size; ++i) {
                new (data_ + i) T();
            }
        }
        catch (...) {
            Destructor(data_, i);
            throw;
        }
    }

    Vector(const Vector& other)
        : data_(Allocate(other.size_))
        , capacity_(other.size_)
        , size_(other.size_)
    {
        size_t i = 0;
        try {
            for (; i != other.size_; ++i) {
                CopyConstruct(data_ + i, other.data_[i]);
            }
        }
        catch (...) {
            Destructor(data_, i);
            throw;
        }
    }

    // �������������� ������ ��� �������� �������, ����� �������� �� ��������� ����������
    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        size_t i = 0;
        T* new_data = nullptr;
        try {
            new_data = Allocate(new_capacity);
            for (; i != size_; ++i) {
                CopyConstruct(new_data + i, data_[i]);
            }
            Destructor(data_, size_);
        }
        catch (...) {
            if (new_data) {
                Destructor(new_data, i);
            }
            throw;
        }

        data_ = new_data;
        capacity_ = new_capacity;
    }

    static void Swap(Vector& lhs, Vector& rhs) {
        std::swap(lhs.data_, rhs.data_);
        std::swap(lhs.size_, rhs.size_);
        std::swap(lhs.capacity_, rhs.capacity_);
    }

    ~Vector() {
        Destructor(data_, size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return capacity_;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

private:
    // �������� ����� ������ ��� n ��������� � ���������� ��������� �� ��
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // �������� ����������� � ����������� ����� ������
    static void Destructor(T* buf, size_t n) noexcept {
        DestroyN(buf, n);
        Deallocate(buf);
    }

    // ����������� ����� ������, ���������� ����� �� ������ buf ��� ������ Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    // �������� ����������� n �������� ������� �� ������ buf
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    // ������ ����� ������� elem � ����� ������ �� ������ buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    // �������� ���������� ������� �� ������ buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }

private:
    T* data_ = nullptr;
    size_t capacity_ = 0;
    size_t size_ = 0;
};
