#ifndef CS540_INTERPOLATE_HPP
#define CS540_INTERPOLATE_HPP

#include <cstddef>

#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cs540 {
class WrongNumberOfArgs : public std::logic_error {
public:
    explicit WrongNumberOfArgs(std::size_t expected, std::size_t actual) :
        logic_error{[=] {
            std::ostringstream sstream;
            sstream << "expected " << expected << ", got " << actual;
            return sstream.str();
        }()} {}
};

namespace internal {
template <typename T>
constexpr bool is_iomanip(const T &) noexcept {
    return false;
}

constexpr bool is_iomanip(std::ios_base &(*)(std::ios_base &)) noexcept {
    return true;
}

constexpr bool is_iomanip(std::ios &(*)(std::ios &)) noexcept {
    return true;
}

inline bool is_iomanip(std::ostream &(*f)(std::ostream &)) {
    class NullStreambuf final : public std::streambuf {
        using _Super = std::streambuf;

    public:
        using int_type = _Super::int_type;

    private:
        bool _overflowed = false;

    protected:
        int_type overflow(int_type c) override {
            _overflowed = true;
            return _Super::overflow(c);
        }

    public:
        bool overflowed() const noexcept {
            return _overflowed;
        }
    };
    NullStreambuf buf;
    std::ostream stream{&buf};
    stream << f;
    return !buf.overflowed();
}

constexpr bool is_iomanip(
    const decltype(std::resetiosflags(std::declval<std::ios_base::fmtflags>())) &) noexcept {
    return true;
}

constexpr bool is_iomanip(
    const decltype(std::setiosflags(std::declval<std::ios_base::fmtflags>())) &) noexcept {
    return true;
}

constexpr bool is_iomanip(const decltype(std::setbase(0)) &) noexcept {
    return true;
}

constexpr bool is_iomanip(const decltype(std::setfill('\0')) &) noexcept {
    return true;
}

constexpr bool is_iomanip(const decltype(std::setprecision(0)) &) noexcept {
    return true;
}

constexpr bool is_iomanip(const decltype(std::setw(0)) &) noexcept {
    return true;
}

template <std::size_t N>
using size_t_constant = std::integral_constant<std::size_t, N>;

template <typename, typename...>
struct CountSpecifiers;

template <std::size_t I, typename... Ts>
struct CountSpecifiers<size_t_constant<I>, Ts...> {
    static auto value(const std::tuple<Ts &&...> &elements) {
        return !is_iomanip(std::get<I>(elements))
            + CountSpecifiers<size_t_constant<I + 1>, Ts...>::value(elements);
    }
    CountSpecifiers() = delete;
};

template <typename... Ts>
struct CountSpecifiers<size_t_constant<sizeof...(Ts)>, Ts...> {
    static constexpr std::size_t value(const std::tuple<Ts &&...> &) noexcept {
        return 0;
    }
    CountSpecifiers() = delete;
};

inline const char *print_till_specifier(const char *fmt, std::ostream &out) {
    const char *next = fmt;
    while (true) {
        switch (*next) {
            case '\0':
                out.write(fmt, next - fmt);
                return next;
            case '%':
                out.write(fmt, next - fmt);
                ++next;
                return next;
            case '\\':
                ++next;
                switch (*next) {
                    case '\0':
                        out.write(fmt, next - fmt);
                        return next;
                    case '%':
                        out.write(fmt, (next - 1) - fmt);
                        out.put('%');
                        ++next;
                        fmt = next;
                        break;
                }
                break;
            default:
                ++next;
                break;
        }
    }
}

template <typename... Ts>
class Interpolation { 
    template <std::size_t I,
              typename T = std::tuple_element_t<I, std::tuple<Ts..., void>>>
    struct _PrintElement {
        static void run(Interpolation &&interpolation, const char *fmt,
                        std::ostream &out) {
            auto &&element = std::get<I>(interpolation._elements);
            bool should_consume = !is_iomanip(element);
            out << std::forward<T>(element);
            if (should_consume) {
                std::move(interpolation)._print<I + 1>(fmt, out);
            } else {
                _PrintElement<I + 1>::run(std::move(interpolation), fmt, out);
            }
        }
        _PrintElement() = delete;
    };

    template <std::size_t I>
    struct _PrintElement<I, void> {
        static constexpr void run(Interpolation &&, const char *, std::ostream &) noexcept {
        }
        _PrintElement() = delete;
    };

    const char *const _fmt;
    std::tuple<Ts &&...> _elements;

    void _check_format() const;

    template <std::size_t I>
    void _print(const char *fmt, std::ostream &out) && {
        _PrintElement<I>::run(std::move(*this),
                              print_till_specifier(fmt, out), out);
    }

public:
    explicit Interpolation(const char *fmt, Ts &&...elements) :
        _fmt{fmt}, _elements{std::forward<Ts>(elements)...} {
        _check_format();
    }

    Interpolation(const Interpolation &) = delete;
    Interpolation(Interpolation &&)
        noexcept(std::is_nothrow_move_constructible<std::tuple<Ts &&...>>::value) = default;

    friend std::ostream &operator<<(std::ostream &out, Interpolation &&interpolation) {
        auto fmt = interpolation._fmt;
        std::move(interpolation)._print<0>(fmt, out);
        return out;
    }
}; // template <typename...> class Interpolation

template <typename... Ts>
void Interpolation<Ts...>::_check_format() const {
    std::size_t expected = 0;
    const char *iter = _fmt;
    while (true) {
        switch (*iter) {
            case '\0':
                goto done;
            case '\\':
                ++iter;
                switch (*iter) {
                    case '\0':
                        goto done;
                    case '%':
                        ++iter;
                        break;
                    default:
                        break;
                }
                break;
            case '%':
                ++expected;
            default:
                ++iter;
                break;
        }
    }
done:
    auto actual = CountSpecifiers<size_t_constant<0>, Ts...>::value(_elements);
    if (actual != expected) {
        throw WrongNumberOfArgs {expected, actual};
    }
}
} // namespace internal

template <typename... Ts>
auto Interpolate(const char *fmt, Ts &&...elements) {
    return internal::Interpolation<Ts...> {fmt, std::forward<Ts>(elements)...};
}

constexpr auto ffr(std::ios &(&f)(std::ios &)) noexcept {
    return f;
}

constexpr auto ffr(std::ostream &(&f)(std::ostream &)) noexcept {
    return f;
}
} // namespace cs540

#endif // CS540_INTERPOLATE_HPP
