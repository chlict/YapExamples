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
        printf("Copy TempVar: %p -> %p, this->id = %d\n", &t, this, id);
    }

    constexpr TempVar(TempVar &&t) : id(t.id) {
        printf("Move TempVar: %p -> %p, this->id = %d\n", &t, this, id);
    }

    friend std::ostream& operator<< (std::ostream &, const TempVar &t);
};

std::ostream& operator<< (std::ostream &os, const TempVar &t) {
    os << "_" << t.id << " @" << &t;
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
            yap::print(std::cout, ir);
        }
    );
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
        printf("assign:\n");
        yap::print(std::cout, assign);
        return hana::append(mIRList, assign);
    }
};
// // This pass generates a sequence of IR
// template <typename Sequence1, typename Sequence2>
// struct GenIR {
//     // Sequence1 is the generated IR list
//     Sequence1 mIRList;
//     // Sequence2 is the extra information coresponding to each IR
//     Sequence2 mInfoList;

//     constexpr GenIR(const Sequence1 &seq1, const Sequence2 &seq2) : mIRList(seq1), mInfoList(seq2) {}

//     template <typename Callable, typename ...Args>
//     auto operator() (yap::expr_tag<yap::expr_kind::call>, Callable &&callable, Args &&...args) {
//         printf("GenIR: call matched\n");
//         if constexpr (sizeof...(Args) != 0) {
//             auto argTuple = hana::make_tuple(args...);
//         }

//         // Gen IR: _temp = callable()
//         auto temp = GenTemp();
//         auto destExpr = yap::make_terminal(std::move(temp));
//         auto assign = yap::make_expression<yap::expr_kind::assign>(destExpr,
//             yap::make_expression<yap::expr_kind::call>(yap::as_expr(callable), yap::as_expr(args)...)
//         );
//         printf("assign:\n");
//         yap::print(std::cout, assign);
//         PrintIRList(hana::append(mIRList, std::move(assign)));
//         return hana::make_pair(hana::append(mIRList, std::move(assign)), hana::append(mInfoList, destExpr));
//     }

//     template <typename Expr1, typename Expr2>
//     auto operator() (yap::expr_tag<yap::expr_kind::plus>, Expr1 &&lhs, Expr2 &&rhs) {
//         printf("GenIR: plus_expr matched\n");
//         using Seq1 = decltype(mIRList);
//         using Seq2 = decltype(mInfoList);
//         auto lhsPair = yap::transform(yap::as_expr(lhs), GenIR<Seq1, Seq2>{mIRList, mInfoList});
//         // PrintTypeName(hana::first(lhsPair));
//         PrintIRList(hana::first(lhsPair));
//         auto rhsPair = yap::transform(yap::as_expr(rhs), GenIR<Seq1, Seq2>{mIRList, mInfoList});
//         PrintIRList(hana::first(rhsPair));
//         auto expr = yap::make_expression<yap::expr_kind::plus>(yap::as_expr(lhs), yap::as_expr(rhs));
//         return hana::make_pair(
//             hana::concat(hana::concat(mIRList, hana::first(lhsPair)), hana::first(rhsPair)),
//             hana::concat(hana::concat(mIRList, hana::second(lhsPair)), hana::second(rhsPair))
//         );
//     }
// };

int foo() { return 0; }

int main() {
    auto call_foo = yap::make_terminal(foo);
    {
        // auto expr = (call_foo() + call_foo());
        auto expr = call_foo();
        yap::print(std::cout, expr);
        // yap::transform(expr, xform{hana::make_map()});
        // auto irList = yap::transform(expr, GenIR{hana::make_tuple(), hana::make_tuple()});
        // PrintIRList(hana::first(irList));
        auto irList = yap::transform(expr, GenIR{hana::make_tuple()});
        printf("After transform:\n");
        PrintIRList(irList);
    }
}
