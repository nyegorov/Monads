#pragma once

namespace monads {

// generic monad interface
template<class A, class F> constexpr auto mmap(A&& a, F f)						{ return f(std::forward<A>(a)); };
template<class...A, class F> constexpr auto mmap(const std::tuple<A...>& t, F f){ return std::apply(f, t); };
template<class...A, class F> constexpr auto mmap(std::tuple<A...>&& t, F f)		{ return std::apply(f, std::move(t)); };
template<class A> constexpr auto mjoin(A&& a)									{ return std::forward<A>(a); };
template<class MA, class F> constexpr auto mapply(MA&& ma, F f)					{ return mjoin(mmap(ma, f)); }

// monad binding operator
template<typename F> struct fwrap {const F f; };
template<class MA, typename F> constexpr auto operator | (MA&& ma, F f)			{ return mapply(std::forward<MA>(ma), f); }
template<class MA, typename F> constexpr auto operator | (MA&& ma, fwrap<F>& f)	{ return f.f(std::forward<MA>(ma)); };
template<class MA, typename F> constexpr auto operator | (MA&& ma, fwrap<F>&& f){ return f.f(std::forward<MA>(ma)); };
template<class F> constexpr auto operator ~ (F f)								{ return fwrap<F> {f}; };

template <class C, class B> struct rebind;
template <template<class...> class C, class A, class B> struct rebind<C<A>, B>	{ typedef C<B> type; };
template <template<class...> class C, class A, template<class> class AL, class B> struct rebind<C<A,AL<A>>, B> { typedef C<B,AL<B>> type; };

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
	F _f;
};
template<typename F> constexpr auto filter(F f) { return ~filter_t<F>(f); }

template<class F> constexpr auto transform(F f)
{
	return ~[f](auto&& ma) {
		rebind<std::decay_t<decltype(ma)>, decltype(f(*std::begin(ma)))>::type mb;
		std::transform(std::begin(ma), std::end(ma), std::inserter(mb, std::end(mb)), f);
		return mb;
	};
}

template<class F, class S> constexpr auto reduce(F f, S&& s)
{
	return ~[f, s = std::forward<S>(s)](auto&& ma) { return std::reduce(std::begin(ma), std::end(ma), s, f); };
}

// some standard monads 

using std::optional;
using std::list;
using std::experimental::generator;
using std::future;

// optional
template <class A, class F> constexpr auto mmap(optional<A>& o, F&& f)	{ return o ? optional{mapply(*o, std::forward<F>(f))} : std::nullopt; };
template <class A> constexpr auto mjoin(optional<optional<A>>&& o)		{ return o ? *o : std::nullopt; };

// list
template <class A, template<class> class AL, class F> constexpr auto mmap(list<A, AL<A>>& la, F f) {
	using B = decltype(mapply(std::declval<A>(), f));
	list<B, AL<B>> lb;
	std::transform(la.begin(), la.end(), std::back_inserter(lb), [f](auto&& a) {return mapply(a, f); });
	return lb;
};

// Not sure, do I want to flatten recursive lists or not?
/*template <class A> auto mjoin(std::list<std::list<A>>&& la) {
	std::list<A> l;
	for(auto&& a : la)	l.splice(l.end(), a);
	return l;
};*/

// generator
template <class A, class F> constexpr auto mmap(generator<A>& ga, F f) { 
	for(auto&& a : ga) co_yield mapply(a, f); 
};

template <class A> constexpr auto mjoin(generator<generator<A>> gga) {
	for(auto&& ga : gga)
		for(auto&& a : const_cast<generator<A>&>(ga)) co_yield a;
};

// future
template <class A, class F> constexpr auto mmap(future<A>& fa, F f) -> future<decltype(mapply(std::declval<A>(), f))> {
	co_return mapply(co_await fa, f);
};

template <class A> constexpr auto mjoin(future<future<A>> ffa) -> future<A> {
	co_return co_await co_await ffa;
};

}
