#include "stdafx.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

using namespace std;

// building blocks

template <typename K, typename V> ostream& operator<<(ostream& os, const map<K, V>& m) {
	os << "{";
	for(auto& [k, v] : m) {
		os << k << ":" << v << " ";
	}
	os << "}\n";
	return os;
}

auto split(string_view src, char separator = ' ')
{
	list<string_view> result;
	size_t start = 0, end;
	while((end = src.find(separator, start)) != string_view::npos) {
		result.push_back(src.substr(start, end - start));
		start = end + 1;
	}
	result.push_back(src.substr(start));
	return result;
}

auto split(char c) { return [c](string_view s) {return split(s, c); }; }

auto to_pair = [](auto&& l) {
	using A = remove_reference<decltype(l)>::type::value_type;
	return	l.size() == 2 ? pair<A, A>{ *l.begin(), *++l.begin() } :
			l.size() == 1 ? pair<A, A>{ *l.begin(), {} } :
							pair<A, A>{ {}, {} };
};

auto insert = [](auto&& m, auto&& e) { m.insert(e); };

// some ingredients

template <typename A, typename B> struct rebase { typedef B type; };
template <class A, class B> struct rebase<std::list<A>, B> { typedef std::list<B> type; };
template <class A, class B> struct rebase<std::vector<A>, B> { typedef std::vector<B> type; };

//template <class C, class B> struct rebase1;// { typedef B type; };
//template <template<class...> class C, class A, class B> struct rebase1<C<A>, B> { typedef C<B> type; };

template <class X> struct fmap {
	template<class F> auto operator() (X& x, F&& f) { return f(forward<X>(x)); };
};

template <class A> struct fmap<optional<A>> {
	template<class F> auto operator() (optional<A>& o, F&& f) {
		return o ? optional<result_of_t<F(A)>>{f(o.value())} : optional<result_of_t<F(A)>>{};
	};
};

template <class A> struct fmap<list<A>> {
	template<class F> auto operator() (list<A>& la, F&& f) {
		list<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(a|f);

		return lb;
	};
};

// magic starts here...

template<class A, class F> auto operator | (A&& a, F&& f)
{
	return fmap<remove_reference_t<A>>()(forward<A>(a), forward<F>(f));
};

template<class A, class F> auto operator | (A&& a, pair<F, int>&& p)
{
	return p.first(forward<A>(a));
};

template<class F> auto apply(F&& f)
{
	return make_pair([f](auto&& la) {
		using LA = remove_reference_t<decltype(la)>;
		using A = LA::value_type;
		using B = result_of_t<F(A)>;
		using LB = rebase<LA, B>::type;

		LB lb;
		std::transform(begin(la), end(la), std::inserter(lb, begin(lb)), f);
		return lb; 
	}, 1);
}

template<class F> auto filter(F&& f)
{
	return make_pair( [f](auto&& la) {
		std::remove_reference_t<decltype(la)> res;
		std::copy_if(begin(la), end(la), std::inserter(res, end(res)), f);
		return res;
	}, 1 );
}

template<class F, class S> auto reduce(F&& f, S&& s)
{
	return make_pair([f, &s](auto&& l) {
		for(auto&& i : l)	f(s, i);
		return s;
	}, 1);
}

int main()
{
	string s = "a=3\nb=xyz\nnoval\n\n";
	map<string, string> m;

	auto r = s	
		| split('\n')
		| split('=')
		| apply(to_pair)
		| filter([](auto&& p) {return !p.first.empty(); })
		| reduce(insert, m)
		;

	auto l = "3,4,x6,5"
		| split(',')
		| [](auto s) { try { return optional<int>(stoi(s.data())); } catch(...) {} return optional<int>{}; }
		| [](auto n) { return n*n; }
		| [](auto n) { return to_string(n); }
		;

	return 0;
}

