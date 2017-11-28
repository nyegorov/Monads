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


auto ints() { 
	int n = 0; 
	while(true) 
		co_yield ++n; 
}
auto even(int n) { return n % 2 == 0; }
auto square = [](auto x) { return x*x; };
generator<int> double_gen(int x) { co_yield x; co_yield x; };
auto take(unsigned n) { 
	return ~[n](auto g) { 
		auto cnt = n; 
		for(auto&& i : g) 
			if(cnt--) 
				co_yield i; 
			else 
				break; 
	}; 
}
auto sum = [](auto&& g) { 
	return accumulate(begin(g), end(g), 0, plus<int>()); 
};


template <class T> struct str1
{
	T t;
};

template <template<class> class T> struct str2
{
	T<int> t;
};



int main()
{
	auto square_async = [](int x) { return async([](int n) { return n*n; }, x); };
	auto half_async = [](int x) { return async([](int n) { return n % 2 ? nothing : just(n / 2); }, x); };
	auto get = [](auto&& f) { return f.get(); };
	auto process_async = [](int x) { return async([](int n) { this_thread::sleep_for(1s); return n*n; }, x); };

	auto res = list<int>{ 1, 2, 3 } | process_async | transform(get) | ~sum;

	str2<str1> mystr2;
	mystr2.t.t = 5;

}
