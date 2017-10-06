#pragma once

template <typename A, typename B> struct rebase { typedef B type; };
template <class A, class B> struct rebase<std::list<A>, B> { typedef std::list<B> type; };
template <class A, class B> struct rebase<std::vector<A>, B> { typedef std::vector<B> type; };

//template <class C, class B> struct rebase1;// { typedef B type; };
//template <template<class...> class C, class A, class B> struct rebase1<C<A>, B> { typedef C<B> type; };

// generic
template <class X> struct fmap {
	template<class F> auto operator() (const X& x, F&& f)	{ return f(x); };
	template<class F> auto operator() (X&& x, F&& f)		{ return f(std::forward<X>(x)); };
};

template <class X> struct join {
	auto operator() (const X& x)  { return x; };
	auto operator() (X&& x)		  { return std::move(x); };
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
	template<class F> auto operator() (std::list<A>&& la, F&& f) {
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
	//template<class F/*, typename = enable_if_t<is_same_v<result_of_t<F(A)>, generator<A>>>*/> auto operator() (const std::experimental::generator<A>& ga, F&& f) {
		//for(auto&& a : ga)	co_yield a | f;
	//};
	template<class F> auto operator() (std::experimental::generator<A>&& ga, F&& f) {
		for(auto&& a : ga)	co_yield a | f;
	};
	/*	template<class F, typename = enable_if_t<!is_same_v<result_of_t<F(A)>, generator<A>>>> auto operator() (generator<A>& la, F&& f) {
	for(auto&& a : la)	co_yield(a | f);
	};*/
	/*	template<class F, typename = enable_if_t<is_same_v<A, int>>> auto operator() (generator<A>& la, F&& f) {
	for(auto&& a : la)	co_yield(a | f);
	};*/
};

template <class A> struct join<std::experimental::generator<std::experimental::generator<A>>> {
	auto operator() (std::experimental::generator<std::experimental::generator<A>>&& gga) {
		for(auto& ga : gga)
			for(auto& a : ga)	co_yield a;
	};
};

// vector
/*template <class A> struct fmap<vector<A>> {
	template<class F> auto operator() (vector<A>& la, F&& f) {
		vector<decltype(declval<A>() | f)> lb;
		for(auto&& a : la)	lb.push_back(f(a));

		return lb;
	};
};*/

// magic starts here...

template<class A, class F> auto operator | (const A& a, F&& f)
{
	auto&& mmb = fmap<std::remove_reference_t<A>>()(a, forward<F>(f));
	return join<std::remove_reference_t<decltype(mmb)>>()(std::forward<std::remove_reference_t<decltype(mmb)>>(mmb));
};

template<class A, class F> auto operator | (A&& a, F&& f)
{
	//return fmap<remove_reference_t<A>>()(forward<A>(a), forward<F>(f));
	auto mmb = fmap<std::remove_reference_t<A>>()(forward<A>(a), forward<F>(f));
	return join<std::remove_reference_t<decltype(mmb)>>()(std::forward<decltype(mmb)>(mmb));
};

template<class A, class F> auto operator | (const A& a, std::pair<F, int>&& p)	{ return p.first(a); };
template<class A, class F> auto operator | (A&& a, std::pair<F, int>&& p)		{ return p.first(forward<A>(a)); };

template<class F> auto transform(F&& f)
{
	return std::make_pair([f](const auto&& la) {
		using LA = std::remove_const_t<std::remove_reference_t<decltype(la)>>;
		using A = LA::value_type;
		using B = std::result_of_t<F(A)>;
		using LB = rebase<LA, B>::type;

		LB lb;
		std::transform(begin(la), end(la), std::inserter(lb, begin(lb)), f);
		return lb;
	}, 1);
}

template<class F> auto filter(F&& f)
{
	return make_pair([f](const auto&& la) {
		std::remove_const_t<std::remove_reference_t<decltype(la)>> res;
		std::copy_if(begin(la), end(la), std::inserter(res, end(res)), f);
		return res;
	}, 1);
}

template<class F, class S> auto reduce(F&& f, S&& s)
{
	return make_pair([f, &s](const auto&& l) {
		for(auto&& i : l)	f(s, i);
		return s;
	}, 1);
}