#ifndef CS540_FUNCTION_HPP
#define CS540_FUNCTION_HPP

#include <cstddef>

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace cs540 {
class BadFunctionCall : public std::logic_error {
public:
    BadFunctionCall() : logic_error{""} {}
};

namespace internal {
template <typename R, typename... Args>
class FunctionBase {
protected:
    FunctionBase() = default;

public:
    FunctionBase(const FunctionBase &) = delete;
    FunctionBase(FunctionBase &&) = delete;
    FunctionBase &operator=(const FunctionBase &) = delete;
    FunctionBase &operator=(FunctionBase &&) = delete;

    virtual ~FunctionBase() = default;
    virtual R operator()(Args...) = 0;
    virtual std::unique_ptr<FunctionBase> clone() = 0;
};

template <typename F, typename... Args>
class Function final : public FunctionBase<std::result_of_t<F(Args...)>, Args...> {
    using _R = std::result_of_t<F(Args...)>;
    F _f;

public:
    template <typename G>
    Function(G &&f) : _f{std::forward<G>(f)} {}

    _R operator()(Args... args) override {
        return _f(args...);
    }

    std::unique_ptr<FunctionBase<_R, Args...>> clone() override {
        return std::make_unique<Function>(_f);
    }
};

template <typename... Args, typename F>
auto function(F &&f) {
    return std::make_unique<internal::Function<F, Args...>>(
        std::forward<F>(f)
    );
}

template <typename... Args, typename F>
auto function(F *f) {
    return f
        ? std::make_unique<internal::Function<F &, Args...>>(*f)
        : nullptr;
}
}

template <typename>
class Function;

template <typename R, typename... Args>
class Function<R(Args...)> {
    std::unique_ptr<internal::FunctionBase<R, Args...>> _f;

public:
    constexpr Function() noexcept = default;
    constexpr Function(std::nullptr_t) noexcept : Function{} {}

    template <typename F>
    Function(F &&f) :
        _f{internal::function<Args...>(f)} {}

    Function(const Function &that) :
        _f{that._f ? that._f->clone() : nullptr} {}

    Function(Function &&) = default;

    Function &operator=(const Function &that) {
        if (this != &that) {
            _f = that._f ? that._f->clone() : nullptr;
        }
        return *this;
    }

    Function &operator=(Function &&) = default;

    R operator()(Args... args) {
        return _f ? (*_f)(args...) : throw BadFunctionCall {};
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(_f);
    }
};

template <typename R, typename... Args>
bool operator==(const Function<R(Args...)> &f, std::nullptr_t) noexcept {
    return !f;
}

template <typename R, typename... Args>
bool operator==(std::nullptr_t, const Function<R(Args...)> &f) noexcept {
    return !f;
}

template <typename R, typename... Args>
bool operator!=(const Function<R(Args...)> &f, std::nullptr_t) noexcept {
    return static_cast<bool>(f);
}

template <typename R, typename... Args>
bool operator!=(std::nullptr_t, const Function<R(Args...)> &f) noexcept {
    return static_cast<bool>(f);
}
}

#endif // CS540_FUNCTION_HPP
