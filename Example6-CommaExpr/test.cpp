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

struct IfExpr {};
struct ForExpr {};

struct Transformer {
    template <typename T>
    auto operator() (yap::expr_tag<yap::expr_kind::terminal>, T &&t) {
        printf("Terminal matched\n");
        PrintTypeName(t);
        std::cout << t << std::endl;
        // return yap::make_terminal(t);
        return t;
    }

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::plus>,
                     Expr1 &&lhs, Expr2 &&rhs) {
        printf("plus_expr matched\n");
        return Add(yap::transform(yap::as_expr(lhs), *this),
                   yap::transform(yap::as_expr(rhs), *this));
    }

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::multiplies>,
                     Expr1 &&lhs, Expr2 &&rhs) {
        printf("mult_expr matched\n");
        return Mul(yap::transform(yap::as_expr(lhs), *this),
                   yap::transform(yap::as_expr(rhs), *this));
    }

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::comma>,
                     Expr1 &&lhs, Expr2 &&rhs) {
        printf("comma_expr matched\n");
        yap::transform(yap::as_expr(lhs), *this);
        return yap::transform(yap::as_expr(rhs), *this);
    }

    template <typename Cond, typename Then, typename Else>
    auto operator() (boost::yap::expr_tag<boost::yap::expr_kind::call>,
                     IfExpr const & if_expr, Cond && cond_expr, Then && then_expr, Else && else_expr) {
        std::cout << "If matched" << std::endl;
        bool cond = yap::transform(yap::as_expr(cond_expr), *this);
        if (cond) {
            std::cout << "then_expr transformed" << std::endl;
            return yap::transform(yap::as_expr(then_expr), *this);
        } else {
            std::cout << "else_expr transformed" << std::endl;
            return yap::transform(yap::as_expr(else_expr), *this);
        }
    }

    template <typename Callable, typename ... Args>
    auto operator() (boost::yap::expr_tag<boost::yap::expr_kind::call>,
                     Callable callable, Args ... args) {
        std::cout << "Call matched" << std::endl;
        return  callable(yap::transform(yap::as_expr(args), *this)...);
    }
};

int Inc(int i) { return i + 1; }

void TestNumberExpr() {
    auto n1 = Number(0.0);
    auto n2 = Number(1.0);
    auto n3 = Number(2.0);
    auto n4 = Number(3.0);
    Config config;

    auto e1 = yap::make_terminal(n1) + yap::make_terminal(n2);
    auto e2 = yap::make_terminal(n3) + yap::make_terminal(n4);
    auto fn = yap::make_terminal(Inc);
 
    // auto expr1 = yap::make_expression<yap::expr_kind::comma>(e1, e2);
    auto expr1 = (e1, e2);
    // for_(e1 : range1, e2 : range2, e3 : range3) -> (e1.CopyTo<UBUF>() + e2.CopyTo<UBUF>()).CopyTo<GM>(e3)

    std::cout << "- Before transformation:\n";
    yap::print(std::cout, expr1);

    auto expr2 = yap::transform(expr1, Transformer{});

    std::cout << "- After transformation:\n";
    std::cout << expr2 << std::endl;
    // yap::print(std::cout, expr2);

    // std::cout << "- Evaluation:\n";
    // auto r1 = yap::evaluate(expr2);
    // std::cout << r1 << std::endl;
}

int main() {
    TestNumberExpr();
}