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
auto sum = reduce(plus<int>(), 0);

using value_t = int;

template<class F> auto loop(istream& is, F f) {
	return [&is, f](auto e) {
		optional<value_t> r;
		for(; !is.eof() && (r = e | f); e = r.value()) {
		} 
		return e;
	};
};

template<class F> auto is_char (istream& is, char ch, F f) { 
	return [&is, ch, f](auto&& e) { 
		char c; is >> c;
		return c == ch ? f(e) : (is.unget(), e); 
	}; 
};

template<class T> optional<T> parse(istream& is);
template<> optional<char> parse<char>(istream& is) { char c = is.get(); if(is.fail()) return {}; else return { c }; };
template<> optional<int> parse<int>(istream& is) { int res; is >> res; if(is.fail()) return {}; else return { res }; };
//template<> optional<value_t> parse<value_t>(istream& is) { return parse<int>(is); };

optional<value_t> parse_mul(istream& is) {
	return parse<value_t>(is)
		| loop(is, [&](auto&& left) { return left
			| is_char(is, '*', [&](auto&& left) { return parse<value_t>(is) | [&](auto&& right) {return left * right; }; })
			| is_char(is, '/', [&](auto&& left) { return parse<value_t>(is) | [&](auto&& right) {return left / right; }; })
			;
		});
		;
};

optional<value_t> parse(istream& is) {
/*	auto left = parse_mul(is);
	while(!is.eof()) {
		char c=0; is >> c;
		auto right = parse_mul(is);
		left = left.value() - right.value();
	}*/

	return parse_mul(is)
		| loop(is, [&](auto&& left) { return left
			| is_char(is, '+', [&](auto&& left) { return parse_mul(is) | [&](auto&& right) {return left + right; }; })
			| is_char(is, '-', [&](auto&& left) { return parse_mul(is) | [&](auto&& right) {return left - right; }; })
			;
	});
	;


	/*return parse_mul(is)
		| is_char(is, '+', [&](auto&& left) { return parse_mul(is) | [&](auto&& right) {return left + right; }; })
		| is_char(is, '-', [&](auto&& left) { return parse_mul(is) | [&](auto&& right) {return left - right; }; })
	;*/
//	return left;
};


int main()
{
	istringstream iss1("3");
	auto i = parse<int>(iss1);

	istringstream iss("5 -2 - 3");
	auto res = parse(iss);

}
