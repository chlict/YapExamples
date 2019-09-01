// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//[hello_world

#include <boost/yap/expression.hpp>
#include <boost/yap/print.hpp>

#include <iostream>
#include <cmath>

template <typename T>
using term = boost::yap::terminal<boost::yap::expression, T>;

int main ()
{
    boost::yap::expression<
        boost::yap::expr_kind::plus,
        boost::hana::tuple<
            boost::yap::expression<
                boost::yap::expr_kind::call,
                boost::hana::tuple<
                    boost::yap::expression<
                        boost::yap::expr_kind::terminal,
                        boost::hana::tuple<double (*)(double)>
                    >,
                    boost::yap::expression<
                        boost::yap::expr_kind::terminal,
                        boost::hana::tuple<double>
                    >
                >
            >,
            boost::yap::expression<
                boost::yap::expr_kind::terminal,
                boost::hana::tuple<float>
            >
        >
    >
    yap_expr = term<double (*)(double)>{{std::sqrt}}(3.0) + 8.0f;
    auto yap_expr2 = term<double (*)(double)>{{std::sqrt}}(3.0);
    boost::yap::print(std::cout, yap_expr2);
    return 0;
}
//]
