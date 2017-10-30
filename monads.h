#pragma once

template <typename A, typename B> struct rebind { typedef B type; };
template <class A, class B> struct rebind<std::list<A>, B> { typedef std::list<B> type; };
template <class A, class B> struct rebind<std::vector<A>, B> { typedef std::vector<B> type; };

template <typename A> struct unwrap					{ typedef A type; };
template <class A> struct unwrap<std::list<A>>		{ typedef A type; };

//template <class C, class B> struct rebase1;// { typedef B type; };
//template <template<class...> class C, class A, class B> struct rebase1<C<A>, B> { typedef C<B> type; };

// generic
template <class X> struct fmap {
	template<class T, class F> auto operator() (T&& x, F&& f)		{ return f(std::forward<T>(x)); };
};

template <class X> struct join {
	template<class A> auto operator() (A&& a) { return std::forward<A>(a); };
};

// optional
template <class A> struct fmap<std::optional<A>> {
	template<class F> auto operator() (const std::optional<A>& o, F&& f) {
		return o ? std::optional<std::result_of_t<F(A)>>{f(o.value())} : std::optional<std::result_of_t<F(A)>>{};
	};
};

template <class A> struct join<std::optional<std::optional<A>>> {
	auto operator() (std::optional<std::optional<A>>&& o) {
		return o ? o.value() : std::optional<std::optional<A>::value_type>{};
	};
};

// list
template <class A> struct fmap<std::list<A>> {
	template<class F> auto operator() (std::list<A>& la, F&& f) {
		std::list<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(a | f);
		return lb;
	};
};

template <class A> struct join<std::list<std::list<A>>> {
	auto operator() (std::list<std::list<A>>&& la) {
		std::list<A> l;
		for(auto&& a : la)	l.splice(l.end(), a);
		return l;
	};
};

// generator
template <class A> struct fmap<std::experimental::generator<A>> {
	template<class F> auto operator() (std::experimental::generator<A>& ga, F&& f) {
		for(auto&& a : ga) co_yield a | f;
	};
};

/*template <class A> struct join<std::experimental::generator<std::experimental::generator<A>>> {
	auto operator() (std::experimental::generator<std::experimental::generator<A>>& gga) {
		for(auto& ga : gga)
			for(auto&& a : ga) co_yield a;
	};
};*/


// magic starts here...

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

template<class MA, class F> auto operator | (MA&& ma, F&& f)
{
	return join<decltype(fmap<std::remove_reference_t<MA>>()(ma, f))>()(fmap<std::remove_reference_t<MA>>()(forward<MA>(ma), forward<F>(f)));
}

template<class MA, class F> auto operator | (MA&& ma, std::pair<F, int>&& p) { return p.first(std::forward<MA>(ma)); };
template<class MA, class F> auto operator | (MA&& ma, filter_t<F>&& flt) { return flt(std::forward<MA>(ma)); };

template<typename F> auto filter(F&& f) { return filter_t<F>(std::forward<F>(f)); }

template<class F> auto transform(F&& f)
{
	return std::make_pair([f](auto&& la) {
		using LA = std::remove_reference_t<decltype(la)>;
		using A = LA::iterator::value_type;
		using B = std::result_of_t<F(A)>;
		using BU = unwrap<B>::type;
		using LB = rebind<LA, B>::type;
		LB lb;
		for(auto&& l : la)	lb.push_back(f(l));
		return lb;
	}, 1);
}

template<class F, class S> auto reduce(F&& f, S&& s)
{
	return make_pair([f, &s](auto&& l) {
		for(auto&& i : l)	f(s, i);
		return s;
	}, 1);
}
