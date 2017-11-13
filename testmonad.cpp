#include "stdafx.h"
#include "monads.h"


using namespace monads;
using namespace std;
using namespace experimental;


using just = optional<int>;
auto nothing = just();

auto mul(int n) { return [n](int x) { return x * n; }; };
auto square_async = [](int x) { return async([](int n) { return n*n; }, x); };
auto half_async = [](int x) { return async([](int n) { return n % 2 ? nothing : just(n / 2); }, x); };


auto ints() { int n = 0; while(true) co_yield ++n; }
auto even(int n) { return n % 2 == 0; }
auto square = [](auto x) { return x*x; };
generator<int> double_gen(int x) { co_yield x; co_yield x; };
auto take(unsigned n) { return ~[n](auto g) { auto cnt = n; for(auto&& i : g) if(cnt--) co_yield i; else break; }; }
auto sum = [](auto&& g) { return accumulate(begin(g), end(g), 0, plus<int>()); };


template<class F> class Reader {
	F func;
public:
	Reader(F func) : func(func) {};
	template<class T> auto operator () (T x) const { return func(x); }
};

template<class F> auto make_reader(F f) { return Reader<F>(f); }

template <class A> struct fmap<Reader<A>> { template<class F> auto operator() (Reader<A>& r, F f) { return make_reader([r,f](auto e) { return f(r(e))(e); }); }; };
//template <class A> struct join<Reader<Reader<A>>> { auto operator() (Reader<Reader<A>>&& ww) { return Reader<A>{ ww.value().value(), ww.message() + ", " + ww.value().message() }; }; };
template <class T> auto mreturn(T x) { return make_reader([x](auto _) {return x; }); };
auto ask() { return make_reader([](auto x) {return x; }); };
template<class R, class E> auto runReader(R r, E e) { return r(e); };

int main()
{
	auto greeter = ask() | [](auto name) { return mreturn( "hello, "s + name + "!"); };

	auto r = runReader(greeter, "world");
	//auto r = runReader(ask(), 1234);
	return 0;
}
