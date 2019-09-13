#include <boost/yap/expression.hpp>
#include <boost/yap/print.hpp>

#include <iostream>
#include <cmath>

namespace yap = boost::yap;
namespace hana = boost::hana;

class Number {
public:
    double x;
    Number() : x(0.0) {}
    Number(double x) : x(x) {}
    Number(const Number &other) : x(other.x) {}

    friend std::ostream& operator<< (std::ostream &os, const Number &number);
};

std::ostream& operator<< (std::ostream &os, const Number &number) {
    os << "x: " << number.x;
    return os;
}

struct Config {
    int f1;
    int f2;

    friend std::ostream& operator<< (std::ostream &os, const Config &config) {
        os << "f1: " << config.f1 << " f2: " << config.f2;
        return os;
    }
};

Number Add(const Number &left, const Number &right) {
    std::cout << "- Add() called" << std::endl;
    return Number(left.x + right.x);
}

Number Mul(const Number &left, const Number &right) {
    std::cout << "- Mul() called" << std::endl;
    return Number(left.x * right.x);
}

Number Copy(const Number &src) {
    std::cout << "- Copy() called" << std::endl;
    return Number(src);
}

std::ostream& operator<< (std::ostream &os, Number (*f)(const Number &, const Number &)) {
    os << (f == Add ? "Add" :
           f == Mul ? "Mul" :
           "unknown function");
    return os;
}

using namespace boost::yap::literals;

struct Transformer {
    // template <typename T>
    // auto operator() (yap::expr_tag<yap::expr_kind::terminal>, T &&t) {
    //     printf("Terminal matched\n");
    //     std::cout << t << std::endl;
    //     return yap::make_terminal(t);
    // }

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::plus>,
                     Expr1 &&lhs, Expr2 &&rhs) {
        printf("plus_expr matched\n");
        return yap::make_expression<yap::expr_kind::call>(
            yap::make_terminal(Add),
            yap::transform(yap::as_expr(lhs), *this),
            yap::transform(yap::as_expr(rhs), *this)
        );
    }

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::multiplies>,
                     Expr1 &&lhs, Expr2 &&rhs) {
        printf("mult_expr matched\n");
        return yap::make_expression<yap::expr_kind::call>(
            yap::make_terminal(Mul),
            yap::transform(yap::as_expr(lhs), *this),
            yap::transform(yap::as_expr(rhs), *this)
        );
    }
};

void TestNumberExpr() {
    auto n1 = Number(0.0);
    auto n2 = Number(1.0);
    auto n3 = Number(2.0);
    auto n4 = Number(3.0);
    Config config;

    auto term1 = yap::make_terminal(n1);
    auto term2 = yap::make_terminal(n2);
    auto term3 = yap::make_terminal(n3);
    auto term4 = yap::make_terminal(n4);
    auto copy = yap::make_terminal(Copy);

    // auto expr1 = term1 * term2 + term3 * term4;
    auto expr1 = term1 + copy(term2 * term3);
    std::cout << "- Before transformation:\n";
    yap::print(std::cout, expr1);

    auto expr2 = yap::transform(expr1, Transformer{});

    std::cout << "- After transformation:\n";
    yap::print(std::cout, expr2);

    std::cout << "- Evaluation:\n";
    auto r1 = yap::evaluate(expr2);
    std::cout << r1 << std::endl;
}

int main() {
    TestNumberExpr();
}