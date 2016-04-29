#ifndef CS540_INTERPOLATE_HPP
#define CS540_INTERPOLATE_HPP

#include <cstddef>

#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

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
using IOManip = std::ios_base &(std::ios_base &);

template <typename...>
struct CountRequiredArgs;

template <>
struct CountRequiredArgs<> {
    static constexpr std::size_t result = 0;
    CountRequiredArgs() = delete;
};

template <typename... Ts>
struct CountRequiredArgs<IOManip &, Ts...> {
    static constexpr auto result = CountRequiredArgs<Ts...>::result;
    CountRequiredArgs() = delete;
};

template <typename T, typename... Ts>
struct CountRequiredArgs<T, Ts...> {
    static constexpr auto result = 1u + CountRequiredArgs<Ts...>::result;
    CountRequiredArgs() = delete;
};
}

template <typename... Ts>
class Interpolation {
    const char *_fmt;
    std::tuple<Ts &&...> _elements;

    static const char *_check_format(const char *fmt) {
        constexpr auto expected = CountRequiredArgs<Ts...>::result;
        std::size_t actual = 0;
        while (true) {
            switch (*fmt) {
                case '\0':
                    goto done;
                case '%':
                    ++actual;
                    if (actual > expected) {
                        throw WrongNumberOfArgs {expected, actual};
                    }
                    break;
                case '\\':
                    ++fmt;
                    switch (*fmt) {
                        case '\0':
                            goto done;
                        case '%':
                            break;
                        default:
                            // skip the extra ++fmt
                            continue;
                    }
                    break;
            }
            ++fmt;
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
    Interpolation &operator=(const Interpolation &) = delete;
    Interpolation &operator=(Interpolation &&) = delete;

    template <typename... Us>
    friend std::ostream &::operator<<(std::ostream &out, Interpolation<Us...> &&);
};

template <typename... Ts>
static auto Interpolate(const char *fmt, Ts &&...elements) {
    return Interpolation<Ts...> {fmt, std::forward<Ts>(elements)...};
}

static auto ffr(std::ostream &(&f)(std::ostream &)) {
    return f;
}
} // namespace cs540

template <typename... Ts>
static std::ostream &operator<<(std::ostream &out, cs540::Interpolation<Ts...> &&) {
    return out;
}

#endif // CS540_INTERPOLATE_HPP
