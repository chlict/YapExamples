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
struct at_placeholder : boost::hana::llong<I> {
    friend std::ostream& operator<< (std::ostream& os, const at_placeholder<I> &p) {
        os << "temp" << I;
        return os;
    }
};

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

template <typename IRList>
auto PrintIRList(IRList &&irList) {
    printf("IR list:\n");
    hana::for_each(irList,
        [](auto const &ir) {
            yap::print(std::cout, ir);
        }
    );
}

struct Tensor {
    int id;

    friend std::ostream& operator<< (std::ostream &os, const Tensor &t) {
        os << "tensor" << t.id;
        return os;
    }
};

constexpr auto MakeTensor(int id) {
    return Tensor{id};
}

template <typename Sequence, typename Stack, long long I>
struct GenIR {
    Sequence mIRList;
    Stack mStack;
    static const long long placeholder_index = I;

    constexpr GenIR(const Sequence &seq, const Stack &stk) : mIRList(seq), mStack(stk) {}

    template <typename T>
    auto operator() (yap::expr_tag<yap::expr_kind::terminal>, T &&t) {
        // printf("GenIR: terminal matched\n");
        // Push result onto stack
        auto stack = hana::append(mStack, std::move(yap::make_terminal(t)));
        return GenIR<decltype(mIRList), decltype(stack), I>{mIRList, stack};
    }

    template <yap::expr_kind Kind, typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<Kind>, Expr1 &&lhs, Expr2 &&rhs) {
        // printf("GenIR: binary op matched\n");
        auto gen1 = yap::transform(yap::as_expr(lhs), GenIR<decltype(mIRList), decltype(mStack), I>(mIRList, mStack));
        // printf("gen1:\n"); PrintIRList(gen1.mIRList);
        auto gen2 = yap::transform(yap::as_expr(rhs), gen1);
        // printf("gen2:\n"); PrintIRList(gen2.mIRList);
        auto constexpr index = decltype(gen2)::placeholder_index;
        auto temp = yap::make_terminal(at_placeholder<index>{});
        auto assign = yap::make_expression<yap::expr_kind::assign>(std::move(temp),
            yap::make_expression<Kind>(
                std::move(hana::back(hana::drop_back(gen2.mStack))),
                std::move(hana::back(gen2.mStack))
            )
        );
        // printf("append:\n"); yap::print(std::cout, assign);
        auto newIRList = hana::append(gen2.mIRList, std::move(assign));
        // Skip poping operands from mStack
        // Push result onto stack
        auto newStack = hana::append(mStack, std::move(temp));

        return GenIR<decltype(newIRList), decltype(newStack), index + 1>(newIRList, newStack);
    }

    template <typename Callable, typename ...Args>
    auto operator() (yap::expr_tag<yap::expr_kind::call>, Callable &&callable, Args &&...args) {
        // printf("GenIR: call matched\n");
        auto assign = yap::make_expression<yap::expr_kind::assign>(
            yap::make_terminal(at_placeholder<I>{}),
            yap::make_expression<yap::expr_kind::call>(yap::as_expr(callable), yap::as_expr(args)...)
        );
        // printf("assign:\n"); yap::print(std::cout, assign);
        auto newIRList = hana::append(mIRList, std::move(assign));
        auto newStack = hana::append(mStack, std::move(yap::make_terminal(at_placeholder<I>{})));
        return GenIR<decltype(newIRList), decltype(newStack), I + 1>{newIRList, newStack};
    }

};

// template <typename Map>
// struct MatchIR {
//     const Map &mMap; // A map from TempVar to expression

//     constexpr MatchIR(const Map &map) : mMap(map) {}

//     template <typename Expr1, typename Expr2>
//     auto operator() (yap::expr_tag<yap::expr_kind::assign>, Expr1 &&lhs, Expr2 &&rhs) {
//         printf("MatchIR: common assign matched\n");
//         assert(false && "Should not reach here");
//     }

//     template <typename Expr2>
//     auto operator() (yap::expr_tag<yap::expr_kind::assign>, TempVar const &temp, Expr2 &&rhs) {
//         printf("MatchIR: assign to temp matched\n");
//         assert(false && "Should not reach here");
//     }

//     template <typename Expr1, typename Expr2>
//     auto operator() (yap::expr_tag<yap::expr_kind::assign>, TempVar const &temp,
//                      yap::expression<yap::expr_kind::plus, hana::tuple<Expr1, Expr2>> const &plusExpr) {
//         printf("MatchIR: assign _temp = lhs + rhs matched\n");
//         auto lhs = yap::left(plusExpr);
//         auto rhs = yap::right(plusExpr);
//         static_assert(decltype(lhs)::kind == yap::expr_kind::terminal);
//         static_assert(decltype(rhs)::kind == yap::expr_kind::terminal);
//         auto newPlusExpr = yap::transform(plusExpr, ReplaceTemps{mMap});
//         printf("newPlusExpr:\n");
//         yap::print(std::cout, newPlusExpr);
//         auto tensor = MakeTensor(temp.id);
//         auto term = yap::make_terminal(std::move(tensor));
//         return hana::insert(mMap, hana::make_pair(temp.id, term));
//         // return yap::make_expression<yap::expr_kind::assign>(std::move(term), plusExpr);
//         // std::cout << temp << std::endl;
//     }

//     template <typename Fn, typename ...Args>
//     auto operator() (yap::expr_tag<yap::expr_kind::assign>, TempVar const &temp,
//                      yap::expression<yap::expr_kind::call, hana::tuple<Fn, Args...>> const &callExpr) {
//         printf("MatchIR: assign _temp = call matched\n");
//         // std::cout << temp << std::endl;
//         return mMap;
//         // return yap::make_expression<yap::expr_kind::assign>(
//         //     std::move(yap::as_expr(temp)),
//         //     std::move(yap::as_expr(callExpr))
//         // );
//     }
// };

auto f1 = [](auto &&map, auto &&irPair) -> decltype(auto) {
    auto const &ir = hana::first(irPair);
    printf("f1:\n"); yap::print(std::cout, ir);
    // return hana::insert(map, hana::make_pair(hana::llong_c<1>, ir));
    // return yap::transform(ir, MatchIR{map});
};

template <typename Sequence>
auto ScanIR(const Sequence &irList) {
    printf("ScanIR\n");
    auto m = hana::fold(irList, hana::make_map(), f1);
    // auto newIRList = hana::transform(irList, [](auto &irPair) {
    //     // yap::print(std::cout, hana::first(irPair));
    //     auto const &ir = hana::first(irPair);
    //     return yap::transform(ir, MatchIR{hana::make_map()});
    // });

    // printf("New IR list:\n");
    // hana::for_each(newIRList, [](auto &ir) {
    //     yap::print(std::cout, ir);
    // });
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
    long a = 10;
    long b = 20;
    {
        auto call_foo = yap::make_terminal(foo);
        auto expr = yap::make_terminal(a) * 2 + call_foo() + yap::make_terminal(b) * 3;
        yap::print(std::cout, expr);
        using Seq = decltype(hana::make_tuple());
        using Stk = Seq;
        auto gen = yap::transform(expr, GenIR<Seq, Stk, 0>{hana::make_tuple(), hana::make_tuple()});
        printf("After transform:\n");
        PrintIRList(gen.mIRList);

        // ScanIR(irList);
    }
}
