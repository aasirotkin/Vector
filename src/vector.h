#pragma once

#include "raw_memory.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
class Vector {
public:
// ---------- Iterator --------------------------------------------------------
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    const_iterator cend() const noexcept {
        return end();
    }

public:
// ---------- Vector ----------------------------------------------------------
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

    // ?????????????? ?????? ??? ???????? ???????, ????? ???????? ?? ????????? ??????????
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
        SelectUninitializedMoveOrCopyWhole(new_data);

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

    template <typename... Args>
    void PushBack(Args&&... args) {
        EmplaceBack(std::forward<Args>(args)...);
    }

    void PopBack() noexcept {
        std::destroy_at(data_ + size_ - 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data((size_ == 0) ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Args>(args)...);
            // ----------------------------------------------------------------
            try {
                SelectUninitializedMoveOrCopyWhole(new_data);
            }
            catch (...) {
                std::destroy_at(new_data + size_);
                throw;
            }
            // ----------------------------------------------------------------
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *(data_ + size_ - 1);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        if (pos < begin() || pos > end()) {
            throw std::range_error("Pos value is outside the Vector");
        }
        size_t shift = pos - begin();

        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data((size_ == 0) ? 1 : size_ * 2);
            new (new_data + shift) T(std::forward<Args>(args)...);
            // ----------------------------------------------------------------
            try {
                SelectUninitializedMoveOrCopy(data_.GetAddress(), shift, new_data.GetAddress());
            }
            catch (...) {
                std::destroy_at(new_data + shift);
                throw;
            }
            // ----------------------------------------------------------------
            try {
                SelectUninitializedMoveOrCopy(data_.GetAddress() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
            }
            catch (...) {
                std::destroy_n(new_data.GetAddress(), shift + 1);
                throw;
            }
            // ----------------------------------------------------------------
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            if (size_ != 0) {
                size_t min_size = size_ - 1;
                T cp_value = T(std::forward<Args>(args)...);
                new (data_ + size_) T(std::move(data_[min_size]));
                std::move_backward(data_ + shift, data_ + min_size, data_ + size_);
                data_[shift] = std::move(cp_value);
            }
            else {
                new (data_ + size_) T(std::forward<Args>(args)...);
            }
        }
        ++size_;
        return data_ + shift;
    }

iterator Erase(const_iterator pos) {
    if (pos < begin() || pos > end()) {
        throw std::range_error("Pos value is outside the Vector");
    }
    size_t shift = pos - begin();
    if (size_ > (shift + 1)) {
        std::move(data_ + shift + 1, data_ + size_, data_ + shift);
    }
    std::destroy_at(data_ + size_ - 1);
    --size_;
    return data_ + shift;
}

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

private:
    void SelectUninitializedMoveOrCopy(T* buff_from, size_t dist, T* buff_to) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(buff_from, dist, buff_to);
        }
        else {
            std::uninitialized_copy_n(buff_from, dist, buff_to);
        }
    }

    void SelectUninitializedMoveOrCopyWhole(RawMemory<T>& new_data) {
        SelectUninitializedMoveOrCopy(data_.GetAddress(), size_, new_data.GetAddress());
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};
