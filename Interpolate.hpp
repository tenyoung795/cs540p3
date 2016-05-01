#ifndef CS540_INTERPOLATE_HPP
#define CS540_INTERPOLATE_HPP

#include <cstddef>

#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cs540 {
namespace internal {
template <typename...>
class Interpolation;
}
}

template <typename... Ts>
std::ostream &operator<<(std::ostream &, cs540::internal::Interpolation<Ts...> &&);

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
template <typename>
struct IsIomanip : std::false_type {};

template <>
struct IsIomanip<std::ios_base &(&)(std::ios_base &)> : std::true_type {};

template <>
struct IsIomanip<std::ios_base &(*)(std::ios_base &)> : std::true_type {};

template <>
struct IsIomanip<decltype(
    std::resetiosflags(std::declval<std::ios_base::fmtflags>())
)> : std::true_type {};

template <>
struct IsIomanip<decltype(
    std::setiosflags(std::declval<std::ios_base::fmtflags>())
)> : std::true_type {};

template <>
struct IsIomanip<decltype(std::setbase(0))> : std::true_type {};

template <>
struct IsIomanip<decltype(std::setfill('\0'))> : std::true_type {};

template <>
struct IsIomanip<decltype(std::setprecision(0))> : std::true_type {};

template <>
struct IsIomanip<decltype(std::setw(0))> : std::true_type {};

template <std::size_t N>
using size_t_constant = std::integral_constant<std::size_t, N>;

template <typename...>
struct CountSpecifiers;

template <>
struct CountSpecifiers<> : size_t_constant<0> {};

template <typename T, typename... Ts>
struct CountSpecifiers<T, Ts...> : size_t_constant<
    !IsIomanip<T>::value + CountSpecifiers<Ts...>::value
> {};

inline const char *print_till_specifier(const char *fmt, std::ostream &out) {
    const char *next = fmt;
    while (true) {
        switch (*next) {
            case '\0':
                out << fmt;
                return next;
            case '%':
                out.write(fmt, next - fmt);
                ++next;
                return next;
            case '\\':
                ++next;
                switch (*next) {
                    case '\0':
                        out << fmt;
                        return next;
                    case '%':
                        out.write(fmt, (next - 1) - fmt);
                        out << '%';
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
    template <typename, typename...>
    friend struct Print;

    template <std::size_t I,
              typename T = std::tuple_element_t<I, std::tuple<Ts..., void>>>
    struct _PrintElement {
        static void run(Interpolation &&interpolation, const char *fmt,
                        std::ostream &out) {
            out << std::forward<T>(std::get<I>(interpolation._elements));
            if (IsIomanip<T>::value) {
                _PrintElement<I + 1>::run(std::move(interpolation), fmt, out);
            } else {
                std::move(interpolation)._print<I + 1>(fmt, out);
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

    static const char *_check_format(const char *fmt);

    template <std::size_t I>
    void _print(const char *fmt, std::ostream &out) && {
        _PrintElement<I>::run(std::move(*this),
                              print_till_specifier(fmt, out), out);
    }

public:
    explicit Interpolation(const char *fmt, Ts &&...elements) :
        _fmt{_check_format(fmt)},
        _elements{std::forward<Ts>(elements)...} {}

    Interpolation(const Interpolation &) = delete;
    Interpolation(Interpolation &&) = default;

    template <typename... Us>
    friend std::ostream &::operator<<(std::ostream &out, Interpolation<Us...> &&);
}; // template <typename...> class Interpolation

template <typename... Ts>
const char *Interpolation<Ts...>::_check_format(const char *fmt) {
    constexpr auto expected = CountSpecifiers<Ts...>::value;
    std::size_t actual = 0;
    const char *iter = fmt;
    while (true) {
        switch (*iter) {
            case '\0':
                goto done;
            case '%':
                ++actual;
                break;
            case '\\':
                ++iter;
                switch (*iter) {
                    case '\0':
                        goto done;
                    case '%':
                        break;
                    default:
                        // skip the extra ++iter
                        continue;
                }
                break;
        }
        ++iter;
    }
done:
    if (actual != expected) {
        throw WrongNumberOfArgs {expected, actual};
    }
    return fmt;
}
} // namespace internal

template <typename... Ts>
auto Interpolate(const char *fmt, Ts &&...elements) {
    return internal::Interpolation<Ts...> {fmt, std::forward<Ts>(elements)...};
}

inline auto ffr(std::ostream &(&f)(std::ostream &)) {
    return f;
}
} // namespace cs540

template <typename... Ts>
std::ostream &operator<<(std::ostream &out,
                         cs540::internal::Interpolation<Ts...> &&interpolation) {
    auto fmt = interpolation._fmt;
    std::move(interpolation).template _print<0>(fmt, out);
    return out;
}

#endif // CS540_INTERPOLATE_HPP
