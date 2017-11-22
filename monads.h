#pragma once

namespace monads {

using std::decay_t;
using std::declval;
using std::forward;

// generic monad interface
template <class X> struct fmap {
	template<class T, class F> constexpr auto operator() (T&& x, F f) { return f(forward<T>(x)); };
};

template <class X> struct join {
	template<class A> constexpr auto operator() (A&& a) { return forward<A>(a); };
};

template<class MA, typename F> constexpr auto mapply(MA&& ma, F f)
{
	return join<decltype(fmap<decay_t<MA>>()(ma, f))>()(fmap<decay_t<MA>>()(forward<MA>(ma), f));
}

// magic starts here...

template<typename F> struct fwrap { F f; };

template<class MA, typename F> constexpr auto operator | (MA&& ma, F&& f)			{ return mapply(forward<MA>(ma), forward<F>(f)); }
template<class MA, typename F> constexpr auto operator | (MA&& ma, fwrap<F>&& f)	{ return f.f(forward<MA>(ma)); };
template<class F> constexpr auto operator ~ (F f)									{ return fwrap<F> {f}; };


template <typename A, typename B> struct rebind { typedef B type; };

//template <class C, class B> struct rebase1;// { typedef B type; };
//template <template<class...> class C, class A, class B> struct rebase1<C<A>, B> { typedef C<B> type; };

// special aggregate functions 
template<typename F> struct filter_t
{
	filter_t(F f) : _f(f) { }
	template<typename MA> constexpr auto operator()(MA&& ma) const {
		MA res;
		copy_if(std::begin(ma), std::end(ma), std::inserter(res, std::end(res), _f));
		return res;
	}
	template<typename A> constexpr auto operator()(std::experimental::generator<A> ga) const {
		for(auto&& a : ga)	if(_f(a)) co_yield a;
	}
	const F _f;
};
template<typename F> constexpr auto filter(F f) { return ~filter_t<F>(f); }

template<class F> constexpr auto transform(F f)
{
	return ~[f](auto&& ma) {
		rebind<decay_t<decltype(ma)>, decltype(f(*std::begin(ma)))>::type mb;
		std::transform(std::begin(ma), std::end(ma), std::inserter(mb, std::end(mb)), f);
		return lb;
	};
}

template<class F, class S> constexpr auto reduce(F f, S&& s)
{
	return ~[f, s = forward<S>(s)](auto&& l) { return std::accumulate(std::begin(l), std::end(l), s, f); };
}

// some standard monads 

using std::optional;
using std::list;
using std::experimental::generator;
using std::future;

// optional
template <class A> struct fmap<optional<A>> {
	template<class F> constexpr auto operator() (const optional<A>& o, F f) {
		return o ? optional<decltype(mapply(declval<A>(), f))>{mapply(o.value(), f)} :
			optional<decltype(mapply(declval<A>(), f))>{};
	};
};

template <class A> struct join<optional<optional<A>>> {
	constexpr auto operator() (optional<optional<A>>&& o) {
		return o ? o.value() : optional<A>{};
	};
};

// list
template <class A, class B> struct rebind<list<A>, B> { typedef list<B> type; };

template <class A> struct fmap<list<A>> {
	template<class F> constexpr auto operator() (list<A>& la, F f) {
		list<decltype(mapply(declval<A>(), f))> lb;
		std::transform(la.begin(), la.end(), std::back_inserter(lb), [f](auto &&a) {return mapply(a, f); });
		return lb;
	};
};

/*// Not sure, do I want to flatten recursive lists or not?

template <class A> struct join<std::list<std::list<A>>> {
	auto operator() (std::list<std::list<A>>&& la) {
		std::list<A> l;
		for(auto&& a : la)	l.splice(l.end(), a);
		return l;
	};
};*/

// generator
template <class A> struct fmap<generator<A>> {
	template<class F> constexpr auto operator() (generator<A>& ga, F f) {
		for(auto&& a : ga) co_yield mapply(a, f);
	};
};

template <class A> struct join<generator<generator<A>>> {
	constexpr auto operator() (generator<generator<A>> gga) {
		for(auto&& ga : gga)
			for(auto&& a : const_cast<generator<A>&>(ga)) co_yield a;
	};
};

// future
template <class A> struct fmap<future<A>> {
	template<class F> constexpr auto operator() (future<A>& fa, F f) -> future<decltype(mapply(declval<A>(), f))> {
		co_return mapply(co_await fa, f);
	};
};

template <class A> struct join<future<future<A>>> {
	constexpr auto operator() (future<future<A>> ffa) -> future<A> {
		co_return co_await co_await ffa;
	};
};


}
