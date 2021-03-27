#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>

// ----------------------------------------------------------------------------

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // ����������� �������� ����� ������ ������, ��������� �� ��������� ��������� �������
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // �������� ����� ������ ��� n ��������� � ���������� ��������� �� ��
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // ����������� ����� ������, ���������� ����� �� ������ buf ��� ������ Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

// ----------------------------------------------------------------------------

template <typename T>
class Vector {
public:
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        size_t i = 0;
        try {
            for (; i != size; ++i) {
                new (data_ + i) T();
            }
        }
        catch (...) {
            DestroyN(data_, i);
            throw;
        }
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        size_t i = 0;
        try {
            for (; i != other.size_; ++i) {
                CopyConstruct(data_ + i, other.data_[i]);
            }
        }
        catch (...) {
            DestroyN(data_, i);
            throw;
        }
    }

    // �������������� ������ ��� �������� �������, ����� �������� �� ��������� ����������
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        size_t i = 0;
        RawMemory<T> new_data(new_capacity);
        try {
            for (; i != size_; ++i) {
                CopyConstruct(new_data + i, data_[i]);
            }
            DestroyN(data_, size_);
        }
        catch (...) {
            DestroyN(data_, i);
            throw;
        }

        data_.Swap(new_data);
    }

    ~Vector() {
        DestroyN(data_, size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

private:
    // �������� ����������� n �������� ������� �� ������ buf
    static void DestroyN(RawMemory<T>& data, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(data + i);
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
    RawMemory<T> data_;
    size_t size_ = 0;
};
