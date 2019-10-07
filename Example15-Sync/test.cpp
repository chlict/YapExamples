#include <boost/yap/expression.hpp>
#include <boost/hana/maximum.hpp>
#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <cassert>

#include <iostream>

namespace yap = boost::yap;
namespace hana = boost::hana;
using namespace hana::literals;

// A placeholder for temporary variables
template <long long I>
struct temp_placeholder : boost::hana::llong<I> {
    friend std::ostream& operator<< (std::ostream& os, const temp_placeholder<I> &p) {
        os << "temp" << I;
        return os;
    }
};

auto const _0 = yap::make_terminal(temp_placeholder<0>{});
auto const _1 = yap::make_terminal(temp_placeholder<1>{});
auto const _2 = yap::make_terminal(temp_placeholder<2>{});
auto const _3 = yap::make_terminal(temp_placeholder<3>{});
auto const _4 = yap::make_terminal(temp_placeholder<4>{});

template <typename IRList>
auto PrintIRList(IRList &&irList) {
    printf("IR list:\n");
    hana::for_each(irList,
        [](auto const &ir) {
            yap::print(std::cout, ir);
        }
    );
}

template <typename IRGenerator>
auto PrintIRGen(IRGenerator &&irGen) {
    printf("IR Generated:\n");
    // using T = typename std::remove_reference<decltype(irGen)>::type;
    // auto indecies = hana::make_tuple(std::make_index_sequence<hana::length(typename T::IRListSeq)>{});
    auto indicies = hana::make_tuple(hana::size_c<0>, hana::size_c<1>);
    hana::for_each(indicies, [&irGen](auto i) {
        std::cout << i << std::endl;
        yap::print(std::cout, irGen.mIRList[i]);
        if constexpr (hana::contains(decltype(hana::keys(irGen.mResources))(), i)) {
            auto resource = irGen.mResources[i];
            std::cout << "resource: " << resource << std::endl;
        }

    });
}

long memory[100] = {0};
struct Tensor {
    int id;

    constexpr Tensor(int id) : id(id) {}

    friend std::ostream& operator<< (std::ostream &os, const Tensor &t) {
        os << "tensor" << t.id << "{" << memory[t.id] << "}";
        return os;
    }

    long operator+ (const Tensor &other) {
        return memory[this->id] + memory[other.id];
    }
    long operator+ (const long &other) {
        return memory[this->id] + other;
    }
    long operator* (const Tensor &other) {
        return memory[this->id] * memory[other.id];
    }
    long operator* (const long &other) {
        return memory[this->id] * other;
    }
};

constexpr auto MakeTensor(int id) {
    return Tensor{id};
}

enum Resource {PIPE_M1, PIPE_M2, PIPE_ALU};

std::ostream& operator<< (std::ostream &os, Resource r) {
    switch (r) {
        case PIPE_M1:
            os << "PIPE_M1";
            break;
        case PIPE_M2:
            os << "PIPE_M2";
            break;
        case PIPE_ALU:
            os << "PIPE_ALU";
            break;
        default:
            os << "invalid resource";
    }
    return os;
}

struct ResourceList {};

template <typename Sequence, typename Stack, typename MapResources, typename MapDeps, long long I = 1>
struct GenIR {
    Sequence mIRList;
    Stack mStack;
    MapResources mResources;
    MapDeps mDepList;
    static const long long placeholder_index = I;

    using IRListSeq = Sequence;
    // static constexpr auto length = hana::length(Sequence{});

    constexpr GenIR(const Sequence &seq, const Stack &stk, const MapResources &res, const MapDeps &deps) :
        mIRList(seq), mStack(stk), mResources(res), mDepList(deps) {
        printf("GenIR constructor:\n");
        // printf("mIRList: "); hana::for_each(mIRList, [](const auto &ir) {yap::print(std::cout, ir);});
        // printf("mStack: "); hana::for_each(mStack, [](const auto &expr) {yap::print(std::cout, expr);});
        printf("mResources: "); hana::for_each(mResources, [](const auto &pair) {std::cout << hana::first(pair) << std::endl;});
    }

    template <typename T>
    auto operator() (yap::expr_tag<yap::expr_kind::terminal>, T &&t) {
        // printf("GenIR: terminal matched\n");
        // Push result onto stack
        auto stack = hana::append(mStack, std::move(yap::make_terminal(t)));
        return GenIR<decltype(mIRList), decltype(stack), decltype(mResources), decltype(mDepList), I>{mIRList, stack, mResources, mDepList};
    }

    template <yap::expr_kind Kind, typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<Kind>, Expr1 &&lhs, Expr2 &&rhs) {
        // printf("GenIR: binary op matched\n");
        auto genLhs = yap::transform(yap::as_expr(lhs), *this);
        // printf("genLhs:\n"); PrintIRList(genLhs.mIRList);
        auto genRhs = yap::transform(yap::as_expr(rhs), genLhs);
        // printf("genRhs:\n"); PrintIRList(genRhs.mIRList);

        auto constexpr index = decltype(genRhs)::placeholder_index;
        auto temp = yap::make_terminal(temp_placeholder<index>{});
        auto assign = yap::make_expression<yap::expr_kind::assign>(std::move(temp),
            yap::make_expression<Kind>(
                std::move(hana::back(genLhs.mStack)),  // lhs's result
                std::move(hana::back(genRhs.mStack))   // rhs's result
            )
        );
        // printf("append:\n"); yap::print(std::cout, assign);
        auto newIRList = hana::append(genRhs.mIRList, std::move(assign));

        // std::cout << "Add resource: " << hana::length(newIRList) - hana::size_c<1> << std::endl;
        auto newResources = hana::insert(genRhs.mResources, hana::make_pair(hana::length(newIRList) - hana::size_c<1>, PIPE_ALU));

        // Skip poping operands from mStack
        // Push result onto stack
        auto newStack = hana::append(mStack, std::move(temp));

        return GenIR<decltype(newIRList), decltype(newStack), decltype(newResources), decltype(mDepList), index + 1>(newIRList, newStack, newResources, mDepList);
    }

    template <typename Callable, typename ...Args>
    auto operator() (yap::expr_tag<yap::expr_kind::call>, Callable &&callable, Args &&...args) {
        // printf("GenIR: call matched\n");
        auto assign = yap::make_expression<yap::expr_kind::assign>(
            yap::make_terminal(temp_placeholder<I>{}),
            yap::make_expression<yap::expr_kind::call>(yap::as_expr(callable), yap::as_expr(args)...)
        );
        // printf("assign:\n"); yap::print(std::cout, assign);
        auto newIRList = hana::append(mIRList, std::move(assign));
        auto newStack = hana::append(mStack, std::move(yap::make_terminal(temp_placeholder<I>{})));
        return GenIR<decltype(newIRList), decltype(newStack), decltype(mResources), decltype(mDepList), I + 1>{newIRList, newStack, mResources, mDepList};
    }

};

template <typename Map>
struct AllocBufferXform {
    const Map &mMap; // A map from TempVar to expression

    constexpr AllocBufferXform(const Map &map) : mMap(map) {}

    template <typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, Expr1 &&lhs, Expr2 &&rhs) {
        printf("AllocBufferXform: common assign matched\n");
        assert(false && "Should not reach here");
    }

    template <typename Expr2, long long I>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, temp_placeholder<I> const &temp, Expr2 &&rhs) const {
        printf("AllocBufferXform: assign to temp matched\n");
        assert(false && "Should not reach here");
    }

    template <long long I, yap::expr_kind Binary, typename Expr1, typename Expr2>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, temp_placeholder<I> const &temp,
                     yap::expression<Binary, hana::tuple<Expr1, Expr2>> const &binaryExpr) {
        printf("AllocBufferXform: assign _temp = lhs op rhs matched\n");
        auto lhs = yap::left(binaryExpr);
        auto rhs = yap::right(binaryExpr);
        static_assert(decltype(lhs)::kind == yap::expr_kind::terminal);
        static_assert(decltype(rhs)::kind == yap::expr_kind::terminal);
        // TODO: use rhs's info to infer information for tensor
        auto tensor = MakeTensor(I);
        auto expr = yap::make_terminal(std::move(tensor));
        return hana::insert(mMap, hana::make_pair(hana::llong_c<I>, expr));
    }

    template <long long I, typename Fn, typename ...Args>
    auto operator() (yap::expr_tag<yap::expr_kind::assign>, temp_placeholder<I> const &temp,
                     yap::expression<yap::expr_kind::call, hana::tuple<Fn, Args...>> const &callExpr) {
        printf("AllocBufferXform: assign _temp = call matched\n");
        auto tensor = MakeTensor(I);
        auto expr = yap::make_terminal(std::move(tensor));
        return hana::insert(mMap, hana::make_pair(hana::llong_c<I>, expr));
    }
};

// Given a sequence of IRList, assign a tensor for each temp_placeholder and returns a map
// recording them.
template <typename Sequence>
auto AllocBuffer(const Sequence &irList) {
    // Given a map and an expression, returns a new map containing placeholder => tensor
    auto fn = [](auto &&map, auto &&ir) -> decltype(auto) {
        return yap::transform(ir, AllocBufferXform{map});
    };

    return hana::fold(irList, hana::make_map(), fn);
}

template <typename ExprMap>
struct SubstituteXform {

    const ExprMap &mMap;
    constexpr SubstituteXform(const ExprMap &map) : mMap(map) {}

    template<long long I>
    auto operator()(yap::expr_tag<boost::yap::expr_kind::terminal>, temp_placeholder<I> const &temp) {   
        static_assert(hana::contains(decltype(hana::keys(mMap))(),hana::llong_c<I>));
        return mMap[boost::hana::llong_c<I>];
    }   
};

// Given a sequence of IR and a map from temp_placeholders to expressions, substitute each
// occurence of temp_placeholder for the corresponding expression
template <typename Sequence, typename Map>
auto SubstituteTemps(Sequence &&irList, Map &&map) {
    return hana::transform(irList, [&map](auto const &ir) {
        return yap::transform(ir, SubstituteXform{map});
    });
}

struct CodeGenXform {
    template <yap::expr_kind BinaryOP, typename Expr1, typename Expr2>
    auto operator()(yap::expr_tag<boost::yap::expr_kind::assign>, Tensor const &lhs,
                    yap::expression<BinaryOP, hana::tuple<Expr1, Expr2>> const &rhs) {   
        printf("CodeGenXform: tensor = expr1 op expr2 matched\n");
        if constexpr (BinaryOP == yap::expr_kind::plus) {
            // memory[lhs.id] = yap::evaluate(yap::left(rhs) + yap::right(rhs));
            std::cout << "Executing TensorAdd`" << std::endl;
        }
        else if constexpr (BinaryOP == yap::expr_kind::multiplies) {
            // memory[lhs.id] = yap::evaluate(yap::left(rhs) * yap::right(rhs));
            std::cout << "Executing TensorMul" << std::endl;
        }
        // std::cout << lhs << std::endl;
        // return memory[lhs.id];
    }
};

template <typename Sequence>
auto CodeGen(Sequence &&irList) {
    hana::for_each(irList, [](const auto &ir) {
        yap::transform(ir, CodeGenXform{});
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
    // print_func_result_type(foo);
    long a = 10;
    long b = 20;
    {
        auto call_foo = yap::make_terminal(foo);
        // auto expr = yap::make_terminal(a) * 2 + call_foo() + yap::make_terminal(b) * 3;
        auto expr = yap::make_terminal(a) + yap::make_terminal(b) * 3;
        yap::print(std::cout, expr);
        auto gen = yap::transform(expr, GenIR{hana::make_tuple(), hana::make_tuple(), hana::make_map(), hana::make_map()});
        printf("After transform:\n");
        // PrintIRList(gen.mIRList);
        PrintIRGen(gen);

        auto &&map = AllocBuffer(gen.mIRList);
        auto irList2 = SubstituteTemps(gen.mIRList, map);
        printf("After AllocBuffer and SubstituteTemps:\n");
        PrintIRList(irList2);

        CodeGen(irList2);
        // printf("result = %ld\n", result);
    }
}
