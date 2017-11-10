#pragma once

namespace monads {

using std::declval;
using std::forward;

template <typename A, typename B> struct rebind { typedef B type; };
template <class A, class B> struct rebind<std::list<A>, B> { typedef std::list<B> type; };
template <class A, class B> struct rebind<std::vector<A>, B> { typedef std::vector<B> type; };

//template <class C, class B> struct rebase1;// { typedef B type; };
//template <template<class...> class C, class A, class B> struct rebase1<C<A>, B> { typedef C<B> type; };

// generic
template <class X> struct fmap {
	template<class T, class F> auto operator() (T&& x, F f) { return f(forward<T>(x)); };
};

template <class X> struct join {
	template<class A> auto operator() (A&& a) { return forward<A>(a); };
};

template<class MA, typename F> auto mapply(MA&& ma, F f)
{
	return join<decltype(fmap<std::decay_t<MA>>()(ma, f))>()(fmap<std::decay_t<MA>>()(forward<MA>(ma), f));
}

// optional
template <class A> struct fmap<std::optional<A>> {
	template<class F> auto operator() (const std::optional<A>& o, F f) {
		return o ?	std::optional<decltype(mapply(declval<A>(), f))>{mapply(o.value(), f)} : 
					std::optional<decltype(mapply(declval<A>(), f))>{};
	};
};

template <class A> struct join<std::optional<std::optional<A>>> {
	auto operator() (std::optional<std::optional<A>>&& o) {
		return o ? o.value() : std::optional<A>{};
	};
};

// list
template <class A> struct fmap<std::list<A>> {
	template<class F> auto operator() (std::list<A>& la, F f) {
		std::list<decltype(mapply(std::declval<A>(), f))> lb;
		for(auto&& a : la)	lb.push_back(mapply(a, f));
		return lb;
	};
};

/*template <class A> struct join<std::list<std::list<A>>> {
	auto operator() (std::list<std::list<A>>&& la) {
		std::list<A> l;
		for(auto&& a : la)	l.splice(l.end(), a);
		return l;
	};
};*/

// generator
template <class A> struct fmap<std::experimental::generator<A>> {
	template<class F> auto operator() (std::experimental::generator<A>& ga, F f) {
		for(auto&& a : ga) co_yield mapply(a, f);
	};
};

template <class A> struct join<std::experimental::generator<std::experimental::generator<A>>> {
	auto operator() (std::experimental::generator<std::experimental::generator<A>> gga) {
		for(auto& ga : gga)
			for(auto&& a : const_cast<std::experimental::generator<A>&>(ga))
				co_yield a;
	};
};

// future
template <class A> struct fmap<std::future<A>> {
	template<class F> auto operator() (std::future<A>& fut, F f) -> std::future<decltype(std::declval<A>() | f)> {
		co_return co_await fut | f;
	};
};

template <class A> struct join<std::future<std::future<A>>> {
	auto operator() (std::future<std::future<A>> ff) -> std::future<A> {
		co_return co_await co_await ff;
	};
};


// magic starts here...

template<typename F> struct fwrap { F f; };

template<class MA, typename F> auto operator | (MA&& ma, F&& f)			{ return mapply(forward<MA>(ma), forward<F>(f)); }
template<class MA, typename F> auto operator | (MA&& ma, fwrap<F>&& f)	{ return f.f(forward<MA>(ma)); };
template<class F> auto operator ~ (F f)									{ return fwrap<F> {f}; };

template<typename F> struct filter_t
{
	filter_t(F f) : _f(f) { }
	template<typename MA> auto operator()(MA&& ma) const {
		MA res;
		for(auto&& a : ma)	if(_f(a)) res.push_back(a);
		return res;
	}
	template<typename A> auto operator()(std::experimental::generator<A> ga) const {
		for(auto&& a : ga)	if(_f(a)) co_yield a;
	}
	const F _f;
};
template<typename F> auto filter(F f) { return ~filter_t<F>(f); }

template<class F> auto transform(F f)
{
	return ~[f](auto&& la) {
		rebind<std::decay_t<decltype(la)>, decltype(f(*std::begin(la)))>::type lb;
		std::transform(std::begin(la), std::end(la), std::inserter(lb, end(lb)), f);
		return lb;
	};
}

template<class F, class S> auto reduce(F f, S&& s)
{
	return ~[f, s = forward<S>(s)](auto&& l) { return std::accumulate(begin(l), end(l), s, f); };
}


}
