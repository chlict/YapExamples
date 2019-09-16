// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//[ calc3
#include <boost/yap/expression.hpp>

#include <boost/hana/maximum.hpp>

#include <boost/yap/print.hpp>

#include <iostream>


// Look! A transform!  This one transforms the expression tree into the arity
// of the expression, based on its placeholders.
//[ calc3_get_arity_xform
struct get_arity
{
    // Base case 1: Match a placeholder terminal, and return its arity as the
    // result.
    template <long long I>
    boost::hana::llong<I> operator() (boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                                      boost::yap::placeholder<I>)
    { return boost::hana::llong_c<I>; }

    // Base case 2: Match any other terminal.  Return 0; non-placeholders do
    // not contribute to arity.
    template <typename T>
    auto operator() (boost::yap::expr_tag<boost::yap::expr_kind::terminal>, T &&)
    {
        using namespace boost::hana::literals;
        return 0_c;
    }

    // Recursive case: Match any expression not covered above, and return the
    // maximum of its children's arities.
    template <boost::yap::expr_kind Kind, typename... Arg>
    auto operator() (boost::yap::expr_tag<Kind>, Arg &&... arg)
    {
        return boost::hana::maximum(
            boost::hana::make_tuple(
                boost::yap::transform(
                    boost::yap::as_expr(std::forward<Arg>(arg)),
                    get_arity{}
                )...
            )
        );
    }
};
//]

namespace yap = boost::yap;
namespace hana = boost::hana;
using namespace boost::hana::literals;

struct tt
{
    hana::tuple<int> list;

    tt() : list(1) {}
    // Base case 1: Match a placeholder terminal, and return its arity as the
    // result.
    template <long long I>
    auto operator() (boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                     boost::yap::placeholder<I>)
    {   std::cout << "placeholder matched\n";
        auto expr = yap::make_terminal(list[0_c]);
        return expr;
    }

    // Base case 2: Match any other terminal.  Return 0; non-placeholders do
    // not contribute to arity.
    // template <typename T>
    // auto operator() (boost::yap::expr_tag<boost::yap::expr_kind::terminal>, T &&)
    // {
    //     return 0_c;
    // }
};

template <typename... ExprList>
struct PlaceholderReplacement
{
    hana::tuple<ExprList...> exprList;

    PlaceholderReplacement(const hana::tuple<ExprList...> &list) : exprList(list) {}

    template <long long I>
    auto operator() (boost::yap::expr_tag<boost::yap::expr_kind::terminal>,
                     boost::yap::placeholder<I>)
    {   std::cout << "placeholder matched\n";
        // static_assert(I <= hana::length(exprList), "Number of placeholders overflow");
        // std::cout << "length(exprList) = " << hana::length(exprList) << std::endl;
        // static_assert(hana::llong_c<I> <= hana::length(exprList));
        // static_assert(hana::size_c<I> <= hana::length(exprList));
        
        auto expr = exprList[hana::llong_c<I - 1>];
        return expr;
    }
};

template <typename... RangeExprs>
struct ForEachRangeImpl {
    hana::tuple<RangeExprs...> rangeExprs;

    static_assert(sizeof...(RangeExprs) > 0, "Empty range expressions");

    ForEachRangeImpl(const RangeExprs &... ranges) : rangeExprs(ranges...) {
        yap::print(std::cout, rangeExprs[0_c]);
    }

    template<typename Expr>
    auto operator[](Expr && expr)
    {
        std::cout << "operator[] matched\n";
        yap::print(std::cout, expr);
        return yap::transform(expr, PlaceholderReplacement{rangeExprs});
    }

};

template <typename... Exprs>
auto for_each_range(const Exprs &... exprs) {
    return ForEachRangeImpl(exprs...);
}

int main ()
{
    using namespace boost::yap::literals;

    // These lambdas wrap our expressions as callables, and allow us to check
    // the arity of each as we call it.
{
    auto expr_1 = 1_p + 2.0;

    yap::print(std::cout, expr_1);
    auto expr2 = yap::transform(expr_1, tt{});
    yap::print(std::cout, expr2);
    auto x = yap::evaluate(expr2);
    std::cout << "x = " << x << std::endl;
}

{
    auto const cout = boost::yap::make_terminal(std::cout);
    auto expr_1 = cout << 1_p;
    yap::print(std::cout, expr_1);
    auto expr_2 = yap::transform(expr_1, tt{});
    yap::print(std::cout, expr_2);
    std::cout << "evaluate: ";
    yap::evaluate(expr_2);
    std::cout << std::endl;
}

{
    auto range_1 = yap::make_terminal(std::vector<int>{1, 2, 3});
    auto range_2 = yap::make_terminal(std::vector<int>{4, 5, 6});
    auto expr_1 = for_each_range(range_1, range_2)[
        1_p + 2_p
    ];
    std::cout << "After for_each_range: ";
    yap::print(std::cout, expr_1);
    
}
    return 0;
}
//]
