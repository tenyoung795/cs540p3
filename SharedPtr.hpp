#ifndef CS540_SHARED_PTR_HPP
#define CS540_SHARED_PTR_HPP

#include <cstddef>

#include <stdexcept>

namespace cs540 {
class _DynamicTag {
    template <typename>
    friend class SharedPtr;

    _DynamicTag() = default;
};

template <typename T>
class SharedPtr {
    template <typename>
    friend class SharedPtr;

    T *_ptr;

    template <typename U>
    explicit SharedPtr(_DynamicTag, const SharedPtr<U> &sp) :
        _ptr{dynamic_cast<T *>(sp._ptr)} {}

public:
    constexpr SharedPtr() : _ptr{} {}

    template <typename U>
    explicit SharedPtr(U *ptr) : _ptr{ptr} {}

    template <typename U>
    SharedPtr(const SharedPtr<U> &sp) : _ptr{static_cast<T *>(sp._ptr)} {}

    void reset() {
        _ptr = nullptr;
    }

    template <typename U>
    void reset(U *ptr) {
        _ptr = ptr;
    }

    constexpr T *get() const {
        return _ptr;
    }

    T &operator*() const {
        return *get();
    }

    constexpr T *operator->() const {
        return get();
    }

    constexpr explicit operator bool() const {
        return get();
    }

    template <typename U, typename V>
    friend SharedPtr<U> dynamic_pointer_cast(const SharedPtr<V> &);
};

template <typename T1, typename T2>
static constexpr bool operator==(const SharedPtr<T1> &sp1, const SharedPtr<T2> &sp2) {
    return sp1.get() == sp2.get();
}

template <typename T>
static constexpr bool operator==(const SharedPtr<T> &sp, std::nullptr_t) {
    return sp;
}

template <typename T>
static constexpr bool operator==(std::nullptr_t, const SharedPtr<T> &sp) {
    return sp;
}

template <typename T1, typename T2>
static constexpr bool operator!=(const SharedPtr<T1> &sp1, const SharedPtr<T2> &sp2) {
    return !(sp1 == sp2);
}

template <typename T>
static constexpr bool operator!=(const SharedPtr<T> &sp, std::nullptr_t) {
    return !sp;
}

template <typename T>
static constexpr bool operator!=(std::nullptr_t, const SharedPtr<T> &sp) {
    return !sp;
}

template <typename T, typename U>
static SharedPtr<T> static_pointer_cast(const SharedPtr<U> &sp) {
    return SharedPtr<T> {sp};
}

template <typename T, typename U>
static SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp) {
    return SharedPtr<T> {_DynamicTag{}, sp};
}
}

#endif // CS540_SHARED_PTR_HPP
