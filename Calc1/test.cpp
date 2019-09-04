//[ calc1
#include <boost/yap/expression.hpp>
#include <boost/yap/print.hpp>

#include <iostream>


int main ()
{
    using namespace boost::yap::literals;

    auto expr = 1_p + 2.0;
    boost::yap::print(std::cout, expr);
    auto x = evaluate(expr, 3.0);
    // Displays "5"
    std::cout << evaluate( 1_p + 2.0, 3.0 ) << std::endl;

    // Displays "6"
    std::cout << evaluate( 1_p * 2_p, 3.0, 2.0 ) << std::endl;

    // Displays "0.5"
    std::cout << evaluate( (1_p - 2_p) / 2_p, 3.0, 2.0 ) << std::endl;

    return 0;
}
//]