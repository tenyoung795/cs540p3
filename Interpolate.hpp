#ifndef CS540_INTERPOLATE_HPP
#define CS540_INTERPOLATE_HPP

#include <ostream>
#include <stdexcept>

namespace cs540 {
class WrongNumberOfArgs : public std::logic_error {
};

template <typename... Ts>
static auto Interpolate(const char *, Ts &&...) {
    return "";
}

static auto ffr(std::ostream &(&f)(std::ostream &)) {
    return f;
}
}

#endif // CS540_INTERPOLATE_HPP
