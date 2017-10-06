#include "stdafx.h"

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

// some ingredients

template <typename A, typename B> struct rebase { typedef B type; };
template <class A, class B> struct rebase<std::list<A>, B> { typedef std::list<B> type; };
template <class A, class B> struct rebase<mylist<A>, B> { typedef mylist<B> type; };
template <class A, class B> struct rebase<std::vector<A>, B> { typedef std::vector<B> type; };

//template <class C, class B> struct rebase1;// { typedef B type; };
//template <template<class...> class C, class A, class B> struct rebase1<C<A>, B> { typedef C<B> type; };

template <class X> struct fmap {
	template<class F> auto operator() (X& x, F&& f) { return f(forward<X>(x)); };
};

template <class X> struct join {
	auto operator() (X& x) { return x;	};
	auto operator() (X&& x) { return std::move(x); };
};

// optional
template <class A> struct fmap<optional<A>> {
	template<class F> auto operator() (optional<A>& o, F&& f) {
		return o ? optional<result_of_t<F(A)>>{f(o.value())} : optional<result_of_t<F(A)>>{};
	};
};

template <class A> struct join<optional<optional<A>>> {
	auto operator() (optional<optional<A>>& o) {
		return o ? o.value() : optional<optional<A>::value_type>{};
	};
};

// list
template <class A> struct fmap<list<A>> {
	template<class F> auto operator() (list<A>& la, F&& f) {
		list<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(a | f);
		return lb;
	};
};

template <class A> struct join<list<list<A>>> {
	auto operator() (list<list<A>>& la) {
		list<A> l;
		for(auto&& a : la)	l.splice(l.end(), a);
		return l;
	};
};

// generator
template <class A> struct fmap<generator<A>> {
	template<class F/*, typename = enable_if_t<is_same_v<result_of_t<F(A)>, generator<A>>>*/> auto operator() (const generator<A>& ga, F&& f) {
		for(auto&& a : ga)	co_yield a|f;
	};
/*	template<class F, typename = enable_if_t<!is_same_v<result_of_t<F(A)>, generator<A>>>> auto operator() (generator<A>& la, F&& f) {
		for(auto&& a : la)	co_yield(a | f);
	};*/
/*	template<class F, typename = enable_if_t<is_same_v<A, int>>> auto operator() (generator<A>& la, F&& f) {
	for(auto&& a : la)	co_yield(a | f);
	};*/
};

template <class A> struct join<generator<generator<A>>> {
	auto operator() (generator<generator<A>>& gga) {
		for(auto& ga : gga)
			for(auto& a : ga)	co_yield a;
	};
};

template <class A> struct fmap<vector<A>> {
	template<class F> auto operator() (vector<A>& la, F&& f) {
		vector<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(f(a));

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

template <class A> struct join<mylist<mylist<A>>> {
	auto operator() (mylist<mylist<A>>&& la) {
		mylist<A> l;
		for(auto&& a : la)	l.splice(l.end(), a);
		return l;
	};
};

// magic starts here...

template<class A, class F> auto operator | (A&& a, F&& f)
{
	//return fmap<remove_reference_t<A>>()(forward<A>(a), forward<F>(f));
	auto&& mmb = fmap<remove_reference_t<A>>()(a, forward<F>(f));
	return join<remove_reference_t<decltype(mmb)>>()(std::forward<remove_reference_t<decltype(mmb)>>(mmb));
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

	auto t1 = std::chrono::high_resolution_clock::now();
	string s = "a=3\nb=xyz\nnoval\n\n";
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

	return 0;
}

