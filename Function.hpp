#ifndef CS540_FUNCTION_HPP
#define CS540_FUNCTION_HPP

#include <cstddef>

#include <stdexcept>

namespace cs540 {
class BadFunctionCall : public std::logic_error {
};

template <typename>
class Function;

template <typename... Args, typename R>
class Function<R(Args...)> {
public:
    Function() = default;

    template <typename F>
    Function(F &&) {}

    R operator()(Args...) {
        throw std::runtime_error{__PRETTY_FUNCTION__};
    }

    explicit operator bool() const {
        return false;
    }
};

template <typename... Args, typename R>
bool operator==(const Function<R(Args...)> &f, std::nullptr_t) {
    return !f;
}

template <typename... Args, typename R>
bool operator==(std::nullptr_t, const Function<R(Args...)> &f) {
    return !f;
}

template <typename... Args, typename R>
bool operator!=(const Function<R(Args...)> &f, std::nullptr_t) {
    return static_cast<bool>(f);
}

template <typename... Args, typename R>
bool operator!=(std::nullptr_t, const Function<R(Args...)> &f) {
    return static_cast<bool>(f);
}
}

#endif // CS540_FUNCTION_HPP
