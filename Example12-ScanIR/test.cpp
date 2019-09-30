#include <boost/yap/expression.hpp>
#include <boost/hana/maximum.hpp>
#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <cassert>

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

struct TempVar {
    int id;
    constexpr TempVar(int i) : id(i) {}

    constexpr TempVar(const TempVar &t) : id(t.id) {
        // printf("Copy TempVar: %p -> %p, this->id = %d\n", &t, this, id);
    }

    constexpr TempVar(TempVar &&t) : id(t.id) {
        // printf("Move TempVar: %p -> %p, this->id = %d\n", &t, this, id);
    }

    friend std::ostream& operator<< (std::ostream &, const TempVar &t);
};

std::ostream& operator<< (std::ostream &os, const TempVar &t) {
    os << "_" << t.id;
    // os << " @" << &t;
    return os;
}

auto GenTemp() {
    static int id = 0;
    return TempVar{id++};
}

template <typename IRList>
auto PrintIRList(IRList &&irList) {
    printf("IR list:\n");
    hana::for_each(irList,
        [](auto const &ir) {
            static_assert(hana::is_a<hana::pair_tag, decltype(ir)>);
            yap::print(std::cout, hana::first(ir));
        }
    );
}

struct Tensor {
    int id;

    friend std::ostream& operator<< (std::ostream &os, const Tensor &t) {
        os << t.id;
        return os;
    }
};

constexpr auto MakeTensor(int id) {
    return Tensor{id};
}

template <typename Sequence>
struct GenIR {
    Sequence mIRList;

    constexpr GenIR(const Sequence &seq) : mIRList(seq) {}

    template <typename Callable, typename ...Args>
    auto operator() (yap::expr_tag<yap::expr_kind::call>, Callable &&callable, Args &&...args) {
        printf("GenIR: call matched\n");
        auto assign = yap::make_expression<yap::expr_kind::assign>(
            yap::make_terminal(GenTemp()),
            yap::make_expression<yap::expr_kind::call>(yap::as_expr(callable), yap::as_expr(args)...)
        );
        printf("assign:\n"); yap::print(std::cout, assign);
        return hana::append(mIRList, hana::make_pair(assign, yap::left(assign)));
    }

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::plus>, Expr1 &&lhs, Expr2 &&rhs) {
        printf("GenIR: plus matched\n");
        auto lhsList = yap::transform(yap::as_expr(lhs), GenIR<decltype(mIRList)>(mIRList));
        auto rhsList = yap::transform(yap::as_expr(rhs), GenIR<decltype(mIRList)>(mIRList));
        // printf("lhsList:\n"); PrintIRList(lhsList);
        // printf("rhsList:\n"); PrintIRList(rhsList);
        // yap::print(std::cout, hana::second(hana::back(lhsList)));
        // yap::print(std::cout, hana::second(hana::back(rhsList)));
        auto assign = yap::make_expression<yap::expr_kind::assign>(
            yap::make_terminal(GenTemp()),
            yap::make_expression<yap::expr_kind::plus>(
                // Use std::move here otherwise only an expr_ref is built and will be out of date outside of this function 
                std::move(hana::second(hana::back(lhsList))),  // result operand of transformed lhs
                std::move(hana::second(hana::back(rhsList)))   // result operand of transformed rhs
            )
        );
        // The resulting IRList: tuple of {mIRList, lhsList, rhsList, assign}
        return hana::append(hana::concat(hana::concat(mIRList, lhsList), rhsList), hana::make_pair(assign, yap::left(assign)));
    }
};

struct MatchIR {
    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, Expr1 &&lhs, Expr2 &&rhs) {
        printf("MatchIR: common assign matched\n");
        assert(false && "Should not reach here");
    }

    template <typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, TempVar const &temp, Expr2 &&rhs) {
        printf("MatchIR: assign to temp matched\n");
        PrintTypeName(rhs);
        std::cout << temp << std::endl;
    }

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, TempVar const &temp,
                     yap::expression<yap::expr_kind::plus, hana::tuple<Expr1, Expr2>> const &plusExpr) {
        printf("MatchIR: assign _temp = plus matched\n");
        auto tensor = MakeTensor(temp.id);
        auto term = yap::make_terminal(std::move(tensor));
        return yap::make_expression<yap::expr_kind::assign>(std::move(term), plusExpr);
        // std::cout << temp << std::endl;
    }

    template <typename Fn, typename ...Args>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, TempVar const &temp,
                     yap::expression<yap::expr_kind::call, hana::tuple<Fn, Args...>> const &callExpr) {
        printf("MatchIR: assign _temp = call matched\n");
        // std::cout << temp << std::endl;
        return yap::make_expression<yap::expr_kind::assign>(
            std::move(yap::as_expr(temp)),
            std::move(yap::as_expr(callExpr))
        );
    }
};

template <typename Sequence>
auto ScanIR(const Sequence &irList) {
    printf("ScanIR\n");
    auto newIRList = hana::transform(irList, [](auto &irPair) {
        // yap::print(std::cout, hana::first(irPair));
        auto const &ir = hana::first(irPair);
        return yap::transform(ir, MatchIR{});
    });

    printf("New IR list:\n");
    hana::for_each(newIRList, [](auto &ir) {
        yap::print(std::cout, ir);
    });
}

int foo() { return 0; }

int bar(int arg) { return 1; }

template <typename Fn>
using result_t = std::result_of_t<Fn()>;

template <typename Fn>
auto print_func_result_type(Fn &&fn) {
    using t = std::result_of_t<Fn()>;
    PrintTypeName<t>();
}

int main() {
    print_func_result_type(foo);
    auto call_foo = yap::make_terminal(foo);
    {
        auto expr = (call_foo() + yap::make_terminal(bar)(1));
        // auto expr = call_foo();
        // yap::print(std::cout, expr);
        // yap::transform(expr, xform{hana::make_map()});
        // auto irList = yap::transform(expr, GenIR{hana::make_tuple(), hana::make_tuple()});
        // PrintIRList(hana::first(irList));
        auto irList = yap::transform(expr, GenIR{hana::make_tuple()});
        printf("After transform:\n");
        // PrintIRList(irList);

        ScanIR(irList);
    }
}
