#include <boost/yap/expression.hpp>
#include <boost/yap/print.hpp>

#include <iostream>
#include <cmath>

namespace yap = boost::yap;
namespace hana = boost::hana;

enum Targ {host, device};

class Number {
public:
    double x;
    Number() : x(0.0) {}
    Number(double x) : x(x) {}
    Number(const Number &other) : x(other.x) {}

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
Number Add(const Number &left, const Number &right, Config &&config = Config()) {
    if constexpr (targ == host) {
        printf("- Add<host> called\n");
        double r = left.x + right.x;
        // printf("- Set config\n");
        config.f1 = 10;
        config.f2 = 20;
        return Number(r);
    }
    else if constexpr (targ == device) {
        printf("- Add<device> called\n");
        double r = left.x + right.x;
        // printf("- Use config\n");
        // config.Println();
        return Number(r);
    }
}

template <Targ targ>
Number Mul(const Number &left, const Number &right, Config &&config = Config()) {
    if constexpr (targ == host) {
        printf("- Mul<host> called\n");
        double r = left.x * right.x;
        // printf("- Set config\n");
        config.f1 = 10;
        config.f2 = 20;
        return Number(r);
    }
    else if constexpr (targ == device) {
        printf("- Mul<device> called\n");
        double r = left.x * right.x;
        // printf("- Use config\n");
        // config.Println();
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

template <yap::expr_kind Kind, typename Tuple>
struct NumberExprT {
    const static yap::expr_kind kind = Kind;
    Tuple elements;

    NumberExprT(const Tuple &tuple) : elements(tuple) {}
};

BOOST_YAP_USER_BINARY_OPERATOR(plus, NumberExprT, NumberExprT)
BOOST_YAP_USER_BINARY_OPERATOR(minus, NumberExprT, NumberExprT)
BOOST_YAP_USER_BINARY_OPERATOR(multiplies, NumberExprT, NumberExprT)

struct NumberExpr :
    NumberExprT<yap::expr_kind::terminal,
                hana::tuple<Number> > {
    NumberExpr(const Number &n): NumberExprT<yap::expr_kind::terminal, hana::tuple<Number>>(hana::make_tuple(n)) {
    }
};

struct Transformer {
    auto operator() (const Number &n) {
        printf("Terminal1 matched\n");
        double value = yap::value(n).x;
        yap::value(n).Println();
        return NumberExpr{Number(value + 10.0)};
    }

    template <typename Expr1, typename Expr2>
    auto operator() (NumberExprT<
                        yap::expr_kind::plus,
                        boost::hana::tuple<Expr1, Expr2> > const &plus_expr) {
        printf("plus_expr matched\n");
        // yap::print(std::cout, yap::left(plus_expr));
        // yap::print(std::cout, yap::right(plus_expr));
        return yap::transform(yap::left(plus_expr), *this) +
               yap::transform(yap::right(plus_expr), *this);
    }

    template <typename Expr1, typename Expr2>
    auto operator() (NumberExprT<
                        yap::expr_kind::multiplies,
                        boost::hana::tuple<Expr1, Expr2> > const &mult_expr) {
        printf("multiplies_expr matched\n");
        printf("left: ");
        PrintTypeName(yap::left(mult_expr));
        yap::print(std::cout, yap::left(mult_expr));
        auto x = yap::deref(yap::left(mult_expr));
        printf("left deref: ");
        PrintTypeName(x);
        printf("right: ");
        PrintTypeName(yap::right(mult_expr));
        yap::print(std::cout, yap::right(mult_expr));
        return yap::transform(yap::deref(yap::left(mult_expr)), *this) *
               yap::transform(yap::deref(yap::right(mult_expr)), *this);
    }

    template <typename Expr>
    auto operator() (Expr const &expr) {
        printf("Should not reach here\n");
        PrintTypeName(expr);
        yap::print(std::cout, expr);
        return expr;
    }
};

void TestNumberExpr3() {
    auto n1 = Number(0.0);
    auto n2 = Number(1.0);
    auto n3 = Number(2.0);
    Config config;

    auto term1 = NumberExpr(n1);
    auto term2 = NumberExpr(n2);
    auto term3 = NumberExpr(n3);

    auto expr1 = term1 * term2;
    std::cout << "- Before transformation:\n";
    yap::print(std::cout, expr1);

    auto expr2 = yap::transform(expr1, Transformer{});

    std::cout << "- After transformation:\n";
    yap::print(std::cout, expr2);

    // std::cout << "Evaluation:\n";
    // auto r1 = yap::evaluate(expr2);
    // r1.Println();
}

int main() {

    // TestBasic();
    // TestNumberExpr1();
    // TestNumberExpr2();
    TestNumberExpr3();
}