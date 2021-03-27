#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    RawMemory(const RawMemory& other) = delete;

    RawMemory& operator= (const RawMemory& other) = delete;

    RawMemory(RawMemory&& other) noexcept {
        Exchange(std::move(other));
    }

    RawMemory& operator= (RawMemory&& other) noexcept {
        Exchange(std::move(other));
        return *this;
    }

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
    void Exchange(RawMemory&& other) {
        Deallocate(buffer_);

        buffer_ = other.buffer_;
        capacity_ = other.capacity_;

        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

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
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector& operator= (const Vector& other) {
        if (this != &other) {
            if (other.size_ > data_.Capacity()) {
                Vector tmp(other);
                Swap(tmp);
            }
            else {
                size_t min_size = std::min(size_, other.size_);
                std::copy_n(other.data_.GetAddress(), min_size, data_.GetAddress());
                if (size_ >= other.size_) {
                    size_t delta = size_ - other.size_;
                    std::destroy_n(data_.GetAddress() + min_size, delta);
                }
                else {
                    size_t delta = other.size_ - size_;
                    std::uninitialized_copy_n(other.data_.GetAddress() + min_size, delta, data_.GetAddress() + min_size);
                }
                size_ = other.size_;
            }
        }
        return *this;
    }

    Vector(Vector&& other) noexcept {
        *this = std::move(other);
    }

    Vector& operator= (Vector&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);

            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    // �������������� ������ ��� �������� �������, ����� �������� �� ��������� ����������
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
        ReserveProcess(new_data);

        data_.Swap(new_data);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
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

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    void PushBack(const T& value) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data((size_ == 0) ? 1 : size_ * 2);
            new (new_data + size_) T(value);
            ReserveProcess(new_data);
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(value);
        }
        ++size_;
    }

    void PushBack(T&& value) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data((size_ == 0) ? 1 : size_ * 2);
            new (new_data + size_) T(std::move(value));
            ReserveProcess(new_data);
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::move(value));
        }
        ++size_;
    }

    void PopBack() noexcept {
        std::destroy_at(data_ + size_ - 1);
        --size_;
    }

private:
    void ReserveProcess(RawMemory<T>& new_data) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};
