#include "Interpolate.hpp"
#include <iostream>
#include <typeinfo>
#include <sstream>
#include <iomanip>
#include <cassert>

class A {
    friend int main();
    friend std::ostream & operator<<(std::ostream &os, const A &a) {
        return os << a.i;
    }
    A(int i_) : i(i_) {}
    A(const A &) = delete;
    A &operator=(const A &) = delete;
    const int i;
};

class B {
    friend int main();
    friend std::ostream & operator<<(std::ostream &os, const B &a) {
        return os << a.str;
    }
    B(std::string s) : str(std::move(s)) {}
    B(const B &) = delete;
    B &operator=(const B &) = delete;
    const std::string str;
};

template <typename... Ts>
void test(const char *func, int line_no, const std::string &cmp, const std::string &fmt, Ts &&...params) {
    std::stringstream s;
    s << cs540::Interpolate(fmt.c_str(), std::forward<Ts>(params)...);
    if (s.str() != cmp) {
        std::cerr << "Comparison failed at " << func << ":" << line_no << ":\n";
        std::cerr << "    Correct result: \"" << cmp << "\"\n";
        std::cerr << "    Actual result: \"" << s.str() << "\"\n";
    }
}
#define CS540_TEST(...) test(__FUNCTION__, __LINE__, __VA_ARGS__)

int
main() {

    using namespace cs540;

    CS540_TEST("", "");

    CS540_TEST(R"(\)", R"(\)");
    CS540_TEST(R"(%)", R"(\%)");
    CS540_TEST(R"(\\)", R"(\\)");
    CS540_TEST(R"(foo)", R"(foo)");
    CS540_TEST("\n", "\n");
    CS540_TEST(R"(\%)", R"(\\%)");
    // Test if it returns ostream.
    {
        std::stringstream s;
        s << Interpolate("i=%", 1234) << ", foo" << std::endl;
        assert(s.str() == "i=1234, foo\n");
    }
    CS540_TEST("1234", "%", 1234);
    CS540_TEST(" 1234", " %", 1234);
    CS540_TEST("1234 ", "% ", 1234);
    CS540_TEST(" 1234 ", " % ", 1234);

    CS540_TEST("i=1, x=3.49887, s=foo, a=768, b=hello, c=x", "i=%, x=%, s=%, a=%, b=%, c=%", 1, 3.4988678671, "foo", A(768), B("hello"), 'x');

    // Test rvalue/lvalue.
    {
        int i1 = 12376;
        const int i2 = 778;
        CS540_TEST("i=8798, j=12376, k=778", "i=%, j=%, k=%", 8798, i1, i2);
    }

    // Many args.
    CS540_TEST(
        "56789 3.14 1234 Z hello 313 goodbye -31 1.99 0xffff7777 "
        "56789 3.14 1234 Z hello 313 goodbye -31 1.99 0xffff7777 "
        "56789 3.14 1234 Z hello 313 goodbye -31 1.99 0xffff7777 "
        "56789 3.14 1234 Z hello 313 goodbye -31 1.99 0xffff7777 "
        "56789 3.14 1234 Z hello 313 goodbye -31 1.99 0xffff7777",
        "% % % % % % % % % % "
        "% % % % % % % % % % "
        "% % % % % % % % % % "
        "% % % % % % % % % % "
        "% % % % % % % % % %",
        56789, 3.14, short(1234), 'Z', "hello", A(313), B("goodbye"), -31, 1.99F, (void *)0xffff7777,
        56789, 3.14, short(1234), 'Z', "hello", A(313), B("goodbye"), -31, 1.99F, (void *)0xffff7777,
        56789, 3.14, short(1234), 'Z', "hello", A(313), B("goodbye"), -31, 1.99F, (void *)0xffff7777,
        56789, 3.14, short(1234), 'Z', "hello", A(313), B("goodbye"), -31, 1.99F, (void *)0xffff7777,
        56789, 3.14, short(1234), 'Z', "hello", A(313), B("goodbye"), -31, 1.99F, (void *)0xffff7777);

    // Test too many args.
    try {
        std::stringstream s;
        s << Interpolate("i=%, j=%", 1, 2, 3);
        assert(false);
    } catch (cs540::WrongNumberOfArgs) {
        // std::cout << "Caught exception due to too many args." << std::endl;
    }

    // Test too few.
    try {
        std::stringstream s;
        s << Interpolate("i=%, j=%, k=%", 1, 2);
        assert(false);
    } catch (cs540::WrongNumberOfArgs) {
        // std::cout << "Caught exception due to few args." << std::endl;
    }

    /*
     * Manipulators.
     */

    CS540_TEST("1, 0, false, true, 0, 1", "%, %, %, %, %, %", true, false, std::boolalpha, false, true, std::noboolalpha, false, true);
    CS540_TEST("0x2134, f78", "%, %", std::showbase, std::hex, 0x2134, std::noshowbase, 0xf78);
    CS540_TEST("1, 1.00000, 1", "%, %, %", 1.0, std::showpoint, 1.0, std::noshowpoint, 1.0);
    CS540_TEST("1, 2, +1, +3", "%, %, %, %", 1.0, 2, std::showpos, 1.0, 3, std::noshowpos);
    CS540_TEST("3.14e-15, 0xfffff115, 3.14E-15, 0XFFFFF115", "%, %, %, %", std::showbase, std::hex, 3.14e-15, -3819, std::uppercase, 3.14e-15, -3819);

    // std::unitbuf, std::nounitbuf, std::internal, std::left, std::right,
    // std::dec, std::hex, std::oct, std::fixed, std::scientific,
    // std::hexfloat, and std::defaultfloat tested, but shouldn't really be
    // missing anything, since they have the same type as a number of other
    // manipulators: std::ios_base &(std::ios_base &).

    // Test std::ends. This consumes a % sign.
    {
        char str[] = {'a', 'b', 'x', '\0', 'y', 'c', 'd'};
        std::string cmp(str, sizeof str);
        CS540_TEST(cmp, "ab%%%cd", 'x', ffr(std::ends), 'y');
    }

    // std::endl also consumes a % sign.
    CS540_TEST("i=1%, x=3.4989, s=foo\n true 1 1234\\",
     R"(i=%\%, x=%, s=%% % % %\)",
     1, std::setprecision(5), 3.4988678671, "foo", ffr(std::endl), std::boolalpha, true, std::noboolalpha, true, A(1234));
}
