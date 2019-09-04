#include <boost/yap/expression.hpp>
#include <boost/yap/print.hpp>

#include <iostream>
#include <cmath>

namespace yap = boost::yap;
template <typename T>
using term = boost::yap::terminal<boost::yap::expression, T>;

enum Targ {host, device};

class Number {
public:
    double x;
    Number() : x(0.0) {}
    Number(double x) : x(x) {}

    Number operator*(const Number &other) {
        printf("operator* called\n");
        double r = x * other.x;
        return Number(r);
    }

    friend std::ostream& operator<< (std::ostream &os, const Number &number);

    void Println() const {
        printf("[%p] x = %f\n", this, x);
    }
};

std::ostream& operator<< (std::ostream &os, const Number &number) {
    os << number.x;
    return os;
}

struct Config {
    int f1;
    int f2;

    void Println() const {
        printf("[Config @%p] f1 = %d, f2 = %d\n", this, f1, f2);
    }
};

template <Targ targ>
Number Add(const Number &left, const Number &right, Config &&config) {
    if constexpr (targ == host) {
        printf("- Add<host> called\n");
        double r = left.x + right.x;
        printf("- Set config\n");
        config.f1 = 10;
        config.f2 = 20;
        return Number(r);
    }
    else if constexpr (targ == device) {
        printf("- Add<device> called\n");
        double r = left.x + right.x;
        printf("- Use config\n");
        config.Println();
        return Number(r);
    }
}

template <Targ targ>
Number Mul(const Number &left, const Number &right, Config &&config) {
    if constexpr (targ == host) {
        printf("- Mul<host> called\n");
        double r = left.x * right.x;
        printf("- Set config\n");
        config.f1 = 10;
        config.f2 = 20;
        return Number(r);
    }
    else if constexpr (targ == device) {
        printf("- Mul<device> called\n");
        double r = left.x * right.x;
        printf("- Use config\n");
        config.Println();
        return Number(r);
    }
}

Number operator+(const Number &left, const Number &right) {
    printf("operator+ called\n");
    return Add<host>(left, right, Config{});
}

Number operator*(const Number &left, const Number &right) {
    printf("operator* called\n");
    left.Println();
    right.Println();
    return Mul<host>(left, right, Config{});
}

using namespace boost::yap::literals;

int main() {
    auto n1 = Number();
    auto n2 = Number(1.0);

    Config config;
    auto n4 = Add<host>(n1, n2, std::move(config));
    n4.Println();
    auto n5 = Add<device>(n1, n2, std::move(config));
    n5.Println();

    printf("test n6\n");
    auto n3 = Number(2.0);
    auto n6 = Add<host>(n1, Mul<host>(n2, n3, std::move(config)), std::move(config));
    n6.Println();

    // auto expr = Add<host>(term<Number>{n1}, term<Number>{n2}, std::move(config));
    auto expr = 1_p + n1;
    yap::print(std::cout, expr);

    auto x = yap::evaluate(expr, n2);
    // yap::print(std::cout, x);
    x.Println();
}