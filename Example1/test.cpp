#include <boost/yap/expression.hpp>
#include <boost/yap/print.hpp>

#include <iostream>
#include <cmath>

namespace yap = boost::yap;

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

void TestBasic() {
    printf("\ntest placeholder\n");
    PrintTypeName(1_p);

    auto n1 = Number();
    auto n2 = Number(1.0);
    printf("\ntest expr\n");
    // auto expr = Add<host>(term<Number>{n1}, term<Number>{n2}, std::move(config));
    auto expr = 1_p + n1;
    PrintTypeName(expr);
    yap::print(std::cout, expr);

    printf("\ntest evaluate\n");
    auto x = yap::evaluate(expr, n2);
    // x.Println();

    printf("\ntest expr2\n");
    auto expr2 = yap::make_terminal<yap::minimal_expr>(n1);
    PrintTypeName(expr2);
    yap::print(std::cout, expr2);
}

void TestNumberExpr1() {
    auto n1 = Number();
    auto n2 = Number(1.0);
    auto n3 = Number(2.0);
    Config config;

    auto n4 = Add<host>(n1, n2);
    // n4.Println();
    auto n5 = Add<device>(n1, n2, std::move(config));
    // n5.Println();

    printf("test n6\n");
    auto n6 = Add<host>(
        n1,
        Mul<host>(
            n2,
            n3)
        );
    n6.Println();

}

template <yap::expr_kind Kind, typename Tuple>
struct NumberExprT {
    const static yap::expr_kind kind = Kind;
    Tuple elements;
};

template <typename T>
using term = boost::yap::terminal<boost::yap::expression, T>;

struct Transformer {
    term<Number> operator() (term<Number> const &n);

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expression<
                        yap::expr_kind::plus,
                        boost::hana::tuple<Expr1, Expr2> > const &plus_expr) {
        printf("plus_tag matched\n");
        yap::print(std::cout, yap::left(plus_expr));
        yap::print(std::cout, yap::right(plus_expr));
        return yap::transform(yap::left(plus_expr), *this) + 
               yap::transform(yap::right(plus_expr), *(this));
    }
};

term<Number> Transformer::operator() (term<Number> const &n) {
    printf("Terminal matched\n");
    double value = yap::value(n).x;
    yap::value(n).Println();
    return term<Number>{std::move(Number(value + 10.0))};
}

void TestNumberExpr2() {
    auto n1 = Number();
    auto n2 = Number(1.0);
    auto n3 = Number(2.0);
    Config config;

    auto expr1 = 1_p + 2_p;
    auto expr1_func = [expr1](auto && ... args) {
        return yap::evaluate(expr1, args...);
    };

    auto r1 = expr1_func(n3, n2);
    r1.Println();
}

void TestNumberExpr3() {
    auto n1 = Number();
    auto n2 = Number(1.0);
    auto n3 = Number(2.0);
    Config config;

    auto expr1 = term<Number>(n1) + term<Number>(n2);
    yap::print(std::cout, expr1);
    auto expr2 = yap::transform(expr1, Transformer{});
    auto r1 = yap::evaluate(expr2);
    r1.Println();
}

int main() {

    // TestBasic();
    // TestNumberExpr1();
    // TestNumberExpr2();
    TestNumberExpr3();
}