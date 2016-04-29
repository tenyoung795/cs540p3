#ifndef CS540_INTERPOLATE_HPP
#define CS540_INTERPOLATE_HPP

#include <cstddef>
#include <cstring>

#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cs540 {
template <typename...>
class Interpolation;
}

template <typename... Ts>
std::ostream &operator<<(std::ostream &, cs540::Interpolation<Ts...> &&);

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

namespace {
template <typename>
struct IsIomanip : std::false_type {};

template <>
struct IsIomanip<std::ios_base &(&)(std::ios_base &)> : std::true_type {};

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

template <typename MoneyT>
struct IsIomanip<decltype(std::get_money(std::declval<MoneyT &>()))> : std::true_type {};

template <typename MoneyT>
struct IsIomanip<decltype(std::put_money(std::declval<const MoneyT &>()))> : std::true_type {};

template <>
struct IsIomanip<decltype(std::get_time(nullptr, ""))> : std::true_type {};

template <>
struct IsIomanip<decltype(std::put_time(nullptr, ""))> : std::true_type {};

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

static void print_escaping_percents(const char *str, std::ostream &out) {
    constexpr char escaped_percent[] = R"(\%)";
    while (true) {
        const char *needle = std::strstr(str, escaped_percent);
        if (!needle) break;
        out.write(str, needle - str);
        out << '%';
        str = needle + (sizeof(escaped_percent) - 1u);
    }
    out << str;
}

static const char *print_till_specifier(const char *fmt, std::ostream &out) {
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

// I have to use size_t_constant instead of std::size_t because C++ templates forbid
// a partially-specialized value parameter from depending on other parameters.

template <typename, typename...>
struct Print;

template <std::size_t I, typename... Ts>
struct Print<size_t_constant<I>, Ts...> {
    static void run(Interpolation<Ts...> &&interpolation,
                    const char *fmt, std::ostream &out) {
        Interpolation<Ts...>::template _PrintElement<I>
                            ::run(std::move(interpolation),
                                  print_till_specifier(fmt, out), out);
    }
    Print() = delete;
};

template <typename... Ts>
struct Print<size_t_constant<sizeof...(Ts)>, Ts...> {
    static void run(Interpolation<Ts...> &&, const char *fmt, std::ostream &out) {
        print_escaping_percents(fmt, out);
    }
    Print() = delete;
};
} // anonymous namespace

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
                Print<size_t_constant<I + 1>, Ts...>::run(std::move(interpolation), fmt, out);
            }
        }
        _PrintElement() = delete;
    };

    template <std::size_t I>
    struct _PrintElement<I, void> : Print<size_t_constant<I>, Ts...> {
        static_assert(I == sizeof...(Ts), "I must represent the end of the tuple");
    };

    const char *const _fmt;
    std::tuple<Ts &&...> _elements;

    static const char *_check_format(const char *fmt) {
        constexpr auto expected = CountSpecifiers<Ts...>::value;
        std::size_t actual = 0;
        const char *iter = fmt;
        while (true) {
            switch (*iter) {
                case '\0':
                    goto done;
                case '%':
                    ++actual;
                    if (actual > expected) {
                        throw WrongNumberOfArgs {expected, actual};
                    }
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
        if (actual < expected) {
            throw WrongNumberOfArgs {expected, actual};
        }
        return fmt;
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
static auto Interpolate(const char *fmt, Ts &&...elements) {
    return Interpolation<Ts...> {fmt, std::forward<Ts>(elements)...};
}

static auto ffr(std::ostream &(&f)(std::ostream &)) {
    return f;
}
} // namespace cs540

template <typename... Ts>
static std::ostream &operator<<(std::ostream &out,
                                cs540::Interpolation<Ts...> &&interpolation) {
    auto fmt = interpolation._fmt;
    cs540::Print<cs540::size_t_constant<0>, Ts...>::run(std::move(interpolation),
                                                        fmt, out);
    return out;
}

#endif // CS540_INTERPOLATE_HPP
