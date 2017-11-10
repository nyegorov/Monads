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


template<class T> class Reader {
	string env;
	T val;
public:
	Reader(T val) : val(val) {};
	Reader(T val, string env) : val(val), env(env) {};
	T value() const { return val; }
	string get() const { return env; }
};

template <class A> struct fmap<Reader<A>> { template<class F> auto operator() (Reader<A>& w, F f) { return Reader<decltype(declval<A>() | f)>(w.value() | f, w.message()); }; };
template <class A> struct join<Reader<Reader<A>>> { auto operator() (Reader<Reader<A>>&& ww) { return Reader<A>{ ww.value().value(), ww.message() + ", " + ww.value().message() }; }; };


template<class T, class F> T runReader(Reader<T> r, F f) { return f(r.value()); };
template<class T> T ask(Reader<T> r) { return r.get(); };

int main()
{
	auto greeter = [](auto r) { auto name = ask(r); return Reader<string>("hello, " + name + "!"); };

	//auto r = runReader(Reader<string>{"", "world"}, greeter);
	return 0;
}
