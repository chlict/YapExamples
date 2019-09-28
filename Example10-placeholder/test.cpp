#include <boost/yap/expression.hpp>
#include <boost/hana/maximum.hpp>
#include <boost/hana.hpp>
#include <boost/yap/print.hpp>

#include <iostream>

namespace yap = boost::yap;
namespace hana = boost::hana;

// A placeholder used by AssignTemps
template <long long I>
struct at_placeholder : boost::hana::llong<I> {};

auto const _0 = yap::make_terminal(at_placeholder<0>{});
auto const _1 = yap::make_terminal(at_placeholder<1>{});
auto const _2 = yap::make_terminal(at_placeholder<2>{});
auto const _3 = yap::make_terminal(at_placeholder<3>{});
auto const _4 = yap::make_terminal(at_placeholder<4>{});

template <typename ExprMap>
struct placeholder_transform {
    template<long long I>
    auto operator()(
        boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
        at_placeholder<I> i)
    {   
        printf("placeholder<%d> matched\n", I);
    }   

    ExprMap map_;
};

template <typename ExprMap>
struct xform {
    ExprMap map_;

    explicit constexpr xform(const ExprMap &map) : map_(map) {}

    template <typename Expr1, typename Expr2>
    decltype(auto) constexpr operator()(yap::expr_tag<yap::expr_kind::assign>, Expr1 &&lhs, Expr2 &&rhs) const {
        std::cout << "xform: assign matched\n";
        yap::print(std::cout, yap::as_expr(std::forward<Expr1>(lhs)));
    }

    template <long long I, typename Expr2>
    decltype(auto) constexpr operator()(yap::expr_tag<yap::expr_kind::assign>, at_placeholder<I> const &lhs, Expr2 &&rhs) const {
        std::cout << "xform: assign placeholder matched\n";
        yap::print(std::cout, yap::as_expr(std::forward<Expr2>(rhs)));
    }
};

int foo() { return 0; }

int main() {
    auto call_foo = yap::make_terminal(foo);
    {
        auto expr = (_0 = call_foo());
        yap::print(std::cout, expr);
        yap::transform(expr, xform{hana::make_map()});
    }
}
