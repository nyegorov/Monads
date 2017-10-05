#include "stdafx.h"
#include <chrono>
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

template <class T>
class mylist : public list<T> {
public:
	mylist() : list() { cout << "list()" << endl; };
	mylist(const mylist& other) : list(other) { cout << "list(const&)" << endl; };
	mylist(mylist&& other) : list(other) { cout << "list(&&)" << endl; };
	~mylist()	{ cout << "~list" << endl; }
};


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
auto to_pair = [](auto&& l) { return make_pair( l.front(), l.back() ); };
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

template <class A> struct fmap<vector<A>> {
	template<class F> auto operator() (vector<A>& la, F&& f) {
		vector<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(a | f);

		return lb;
	};
};

template <class A> struct fmap<mylist<A>> {
	template<class F> auto operator() (mylist<A>& la, F&& f) {
		list<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(a | f);

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

template<class F> auto transform(F&& f)
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
	int x = 3, y = 5;
	auto z = x | y;

	auto t1 = std::chrono::high_resolution_clock::now();
	string s = "a=3\nb=xyz\nnoval\n\n";
	map<string, string> m;
	auto r = s
		| split('\n')
		| split('=')
		| filter([](auto&& p) {return p.size() == 2; })
		| transform(to_pair)
		| reduce(insert, m)
		;
	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> ms = t2 - t1;
	std::cout << "" << m.size() << " took " << ms.count() << " ms\n";


	cout << "---" << endl;
	auto l = "3,4,x6,5"
		| split(',')
		| [](auto s) { try { return optional<int>(stoi(s.data())); } catch(...) {} return optional<int>{}; }
		| [](auto n) { return n*n; }
		| [](auto n) { return to_string(n); }
		;

	return 0;
}

