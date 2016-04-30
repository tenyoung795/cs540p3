#ifndef CS540_SHARED_PTR_HPP
#define CS540_SHARED_PTR_HPP

#include <cstddef>

#include <atomic>
#include <utility>

#if ATOMIC_POINTER_LOCK_FREE < 2
#warn "std::atomic_uintptr_t is not always lock-free"
#endif

namespace cs540 {
namespace {
class SharedObjectBase {
    std::atomic_uintptr_t _counter;

protected:
    constexpr SharedObjectBase() noexcept : _counter{1} {}

public:
    SharedObjectBase(const SharedObjectBase &) = delete;
    SharedObjectBase(SharedObjectBase &&) = delete;
    SharedObjectBase &operator=(const SharedObjectBase &) = delete;
    SharedObjectBase &operator=(SharedObjectBase &&) = delete;

    virtual ~SharedObjectBase() = default;

    auto increment() noexcept {
        return _counter.fetch_add(1, std::memory_order_relaxed);
    }

    auto decrement() noexcept {
        return _counter.fetch_sub(1, std::memory_order_acq_rel);
    }
};

template <typename T>
class SharedObject final : public SharedObjectBase {
    const T *const _ptr;

public:
    constexpr explicit SharedObject(const T *ptr) noexcept : _ptr{ptr} {}

    ~SharedObject() override {
        delete _ptr;
    }
};

template <typename T>
inline SharedObjectBase *share(const T *ptr) {
    return ptr ? new SharedObject<T> {ptr} : nullptr;
}
}

template <typename T>
class SharedPtr {
    template <typename>
    friend class SharedPtr;

    SharedObjectBase *_object;
    T *_base;

    template <typename U>
    explicit SharedPtr(const SharedPtr<U> &that, T *base) noexcept :
        _object{that._object}, _base{base} {
        if (_object) _object->increment();
    }

    template <typename U>
    explicit SharedPtr(SharedPtr<U> &&that, T *base) noexcept :
        _object{that._object}, _base{base} {
        that._clear();
    }

    template <typename U>
    void _copy_from(const SharedPtr<U> &that) noexcept {
        if (static_cast<const void *>(this) != static_cast<const void *>(&that)) {
            if (that._object) that._object->increment();
            _release();
            _object = that._object;
            _base = that._base;
        }
    }

    template <typename U>
    void _move_from(SharedPtr<U> &&that) noexcept {
        _object = that._object;
        _base = that._base;
        that._clear();
    }

    void _release() noexcept {
        if (_object && _object->decrement() == 1) {
            delete _object;
        }
    }

    void _clear() noexcept {
        _object = nullptr;
        _base = nullptr;
    }

public:
    constexpr SharedPtr() noexcept :  _object{}, _base{} {}

    constexpr explicit SharedPtr(std::nullptr_t) noexcept : SharedPtr{} {}

    template <typename U>
    explicit SharedPtr(U *ptr) : _object{share(ptr)}, _base{ptr} {}

    SharedPtr(const SharedPtr &that) noexcept : SharedPtr{that, that._base} {}

    template <typename U>
    SharedPtr(const SharedPtr<U> &that) noexcept : SharedPtr{that, that._base} {}

    SharedPtr(SharedPtr &&that) noexcept : SharedPtr{std::move(that), that._base} {}

    template <typename U>
    SharedPtr(SharedPtr<U> &&that) noexcept : SharedPtr{std::move(that), that._base} {}

    SharedPtr &operator=(const SharedPtr &that) noexcept {
        _copy_from(that);
        return *this;
    }

    template <typename U>
    SharedPtr &operator=(const SharedPtr<U> &that) noexcept {
        _copy_from(that);
        return *this;
    }

    SharedPtr &operator=(SharedPtr &&that) noexcept {
        _move_from(std::move(that));
        return *this;
    }

    template <typename U>
    SharedPtr &operator=(SharedPtr<U> &&that) noexcept {
        _move_from(std::move(that));
        return *this;
    }

    ~SharedPtr() {
        _release();
    }

    void reset() noexcept {
        _release();
        _clear();
    }

    void reset(std::nullptr_t) noexcept {
        reset();
    }

    template <typename U>
    void reset(U *ptr) {
        auto new_object = share(ptr);
        _release();
        _object = new_object;
        _base = ptr;
    }

    constexpr T *get() const noexcept {
        return _base;
    }

    T &operator*() const {
        return *get();
    }

    constexpr T *operator->() const noexcept {
        return get();
    }

    constexpr explicit operator bool() const noexcept {
        return get();
    }

    template <typename T1, typename T2>
    friend constexpr bool operator==(const SharedPtr<T1> &,
                                     const SharedPtr<T2> &) noexcept;

    template <typename U, typename V>
    friend SharedPtr<U> static_pointer_cast(const SharedPtr<V> &) noexcept;

    template <typename U, typename V>
    friend SharedPtr<U> static_pointer_cast(SharedPtr<V> &&) noexcept;

    template <typename U, typename V>
    friend SharedPtr<U> dynamic_pointer_cast(const SharedPtr<V> &) noexcept;

    template <typename U, typename V>
    friend SharedPtr<U> dynamic_pointer_cast(SharedPtr<V> &&) noexcept;
};

template <typename T1, typename T2>
inline constexpr bool operator==(const SharedPtr<T1> &sp1,
                                 const SharedPtr<T2> &sp2) noexcept {
    return sp1._object == sp2._object;
}

template <typename T>
inline constexpr bool operator==(const SharedPtr<T> &sp, std::nullptr_t) noexcept {
    return sp;
}

template <typename T>
inline constexpr bool operator==(std::nullptr_t, const SharedPtr<T> &sp) noexcept {
    return sp;
}

template <typename T1, typename T2>
inline constexpr bool operator!=(const SharedPtr<T1> &sp1,
                                 const SharedPtr<T2> &sp2) noexcept {
    return !(sp1 == sp2);
}

template <typename T>
inline constexpr bool operator!=(const SharedPtr<T> &sp, std::nullptr_t) noexcept {
    return !sp;
}

template <typename T>
inline constexpr bool operator!=(std::nullptr_t, const SharedPtr<T> &sp) noexcept {
    return !sp;
}

template <typename T, typename U>
inline SharedPtr<T> static_pointer_cast(const SharedPtr<U> &sp) noexcept {
    return SharedPtr<T> {sp, static_cast<T *>(sp._base)};
}

template <typename T, typename U>
inline SharedPtr<T> static_pointer_cast(SharedPtr<U> &&sp) noexcept {
    return SharedPtr<T> {std::move(sp), static_cast<T *>(sp._base)};
}

template <typename T, typename U>
inline SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp) noexcept {
    auto base = dynamic_cast<T *>(sp._base);
    return base ? SharedPtr<T> {sp, base} : SharedPtr<T> {};
}

template <typename T, typename U>
inline SharedPtr<T> dynamic_pointer_cast(SharedPtr<U> &&sp) noexcept {
    auto base = dynamic_cast<T *>(sp._base);
    return base ? SharedPtr<T> {std::move(sp), base} : SharedPtr<T> {};
}
}

#endif // CS540_SHARED_PTR_HPP
