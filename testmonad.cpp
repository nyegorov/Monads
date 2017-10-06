#include "stdafx.h"
#include "monads.h"

using namespace std;
using namespace std::experimental;

template <class T>
class mylist : public list<T> {
public:
	mylist() : list() { cout << "list()" << endl; };
	mylist(const mylist& other) : list(other) { cout << "list(const&)" << endl; };
	mylist(mylist&& other) : list(std::forward<list>(other)) { cout << "list(&&)" << endl; };
	~mylist()	{ cout << "~list" << endl; }
};

template <class A, class B> struct rebase<mylist<A>, B> { typedef mylist<B> type; };

template <class A> struct fmap<mylist<A>> {
	template<class F> auto operator() (mylist<A>& la, F&& f) {
		list<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(a | f);
		return lb;
	};
};

template <class A> struct join<mylist<mylist<A>>> {
	auto operator() (mylist<mylist<A>>&& la) {
		mylist<A> l;
		for(auto&& a : la)	l.splice(l.end(), a);
		return l;
	};
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

generator<string_view> split_async(string_view src, char separator = ' ')
{
	size_t start = 0, end;
	while((end = src.find(separator, start)) != string_view::npos) {
		co_yield src.substr(start, end - start);
		start = end + 1;
	}
	co_yield src.substr(start);
}

auto split(char c) { return [c](string_view s) {return split(s, c); }; }
auto split_async(char c) { return [c](string_view s) {return split_async(s, c); }; }
auto to_pair = [](auto&& l) { return make_pair( l.front(), l.back() ); };
auto insert = [](auto&& m, auto&& e) { m.insert(e); };
template<class T> optional<T> parse(string_view s);
template<> optional<int> parse<int>(string_view s)	{ try { return optional<int>(stoi(s.data())); } catch(...) {} return optional<int>{}; };



int main()
{
	//int x = 3, y = 5;
	//auto z = x | y;

	auto x = 3 
		| [](auto n) {return optional<int>{n}; }
		| [](auto n) {return n*n; }
	;

/*	auto g = split_async('\n')("a=3\nb=xyz\nnoval\n\n");
	for(auto&& i : g) {
		auto gg = split_async('=')(i);
		for(auto&& j : gg)
			cout << j;
	}*/

	/*string s = "a=3\nb=xyz\nnoval\n\n";
	auto r = s 
		| split_async('\n')
		| split_async('=');

	for(auto&& i : r) {
		//for(auto&& j : i)	{
			//cout << j << endl; 
		//}
		//cout << i;
	}*/

/*	auto t1 = std::chrono::high_resolution_clock::now();
	auto s = "a=3\nb=xyz\nnoval\n\n"sv;
	map<string, string> m;
	auto r = s
		| split('\n')
		| transform(split('='))
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
		| parse<int>
		| [](auto n) { return n*n; }
		| [](auto n) { return to_string(n); }
		;
		*/
	return 0;
}

