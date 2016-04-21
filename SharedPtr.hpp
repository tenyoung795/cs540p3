#ifndef CS540_SHARED_PTR_HPP
#define CS540_SHARED_PTR_HPP

#include <cstddef>

#include <stdexcept>

namespace cs540 {

template <typename T>
class SharedPtr {
public:
    constexpr SharedPtr() = default;

    template <typename U>
    explicit SharedPtr(U *) {}

    template <typename U>
    SharedPtr(const SharedPtr<U> &) {}

    void reset() {
    }

    template <typename U>
    void reset(U *) {
    }

    constexpr T *get() const {
        throw std::runtime_error {__PRETTY_FUNCTION__};
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
    return SharedPtr<T> {sp};
}

}

#endif // CS540_SHARED_PTR_HPP
