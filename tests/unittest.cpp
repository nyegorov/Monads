#include "stdafx.h"
#include "CppUnitTest.h"
#include "../monads.h"

#ifdef _DEBUG
const size_t ITER_COUNT = 2'000;
#else
const size_t ITER_COUNT = 100'000;
#endif		

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace std::experimental;
using namespace monads;

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

generator<string_view> split_gen(string_view src, char separator = ' ')
{
	size_t start = 0, end;
	while((end = src.find(separator, start)) != string_view::npos) {
		co_yield src.substr(start, end - start);
		start = end + 1;
	}
	co_yield src.substr(start);
}

auto even(int n) { return n % 2 == 0; }
auto square = [](auto x) { return x*x; };
using just = optional<int>;
auto nothing = just();
auto ints() { int n = 0; while(true) co_yield ++n; }
auto take(unsigned n) { return ~[n](auto g) { auto cnt = n; for(auto&& i : g) if(cnt--) co_yield i; else break; }; }
auto sum = reduce(plus<int>(), 0);
auto insert = [](auto&& m, auto&& e) { m.insert(std::move(e)); return m; };
auto mul(int n) { return [n](int x) { return x * n; }; };
generator<int> double_gen(int x) { co_yield x; co_yield x; };

auto split(char c) { return [c](string_view s) {return split(s, c); }; }
auto split_gen(char c) { return [c](string_view s) {return split_gen(s, c); }; }
template<class T> optional<T> parse(string_view s);
template<> optional<int> parse<int>(string_view s) { try { return { stoi(s.data()) }; } catch(...) { return {}; } };

static int bank_ctors = 0;
static int copy_ctors = 0;
static int move_ctors = 0;
static int bank_dtors = 0;

template <class T> class bank_account {
	T val;
public:
	bank_account(T val) : val(val) { bank_ctors++; }
	bank_account(const bank_account& other) { copy_ctors++; val = other.val; }
	bank_account(bank_account&& other) { move_ctors++; val = other.val; other.val = 0; }
	~bank_account() { bank_dtors++; }
	T amount() const { return val; }
};
template <class A, class B> struct rebind<bank_account<A>, B> { typedef bank_account<B> type; };
template <class A, class F> auto mmap(bank_account<A>& ma, F f) { return bank_account<decltype(declval<A>() | f)>(ma.amount() | f); };
template <class A> auto mjoin(bank_account<bank_account<A>>&& mma) { return bank_account<A>{ mma.amount().amount() }; };

struct null_t {};
template<class T> class Writer {
	T val;
	string msg;
public:
	Writer(T val) : val(val) {};
	Writer(T val, string msg) : val(val), msg(msg) {};
	T value() const { return val; }
	string message() const { return msg; }
};
template<class T> auto make_writer(T x, string msg) { return Writer<T>(x, msg); }

template <class A, class F> auto mmap(Writer<A>& w, F f) { return make_writer(f(w.value()), w.message()); };
template <class A> auto mjoin(Writer<Writer<A>>&& ww) { return Writer<A>{ ww.value().value(), ww.message() + ww.value().message() }; };

template<class T> tuple<T, string> runWriter(Writer<T> w) { return { w.value(), w.message() }; };
Writer<null_t> tell(string msg) { return { null_t{}, msg }; };

template<class F> class Reader {
	F func;
public:
	Reader(F func) : func(func) {};
	template<class T> auto operator () (T x) const { return func(x); }
};
template<class F> auto make_reader(F f) { return Reader<F>(f); }

template <class A, class F> auto mmap(Reader<A>& r, F f) { return make_reader([r, f](auto e) { return f(r(e))(e); }); };
template <class T> auto returnR(T x)			{ return make_reader([x](auto _) {return x; }); };
auto ask() { return make_reader([](auto x) {return x; }); };
template<class R, class E> auto runReader(R r, E e) { return r(e); };


namespace Microsoft::VisualStudio::CppUnitTestFramework {
template<> inline std::wstring ToString<just>(const just& v) { return v ? to_wstring(v.value()) : L"nothing"; }
}

namespace tests
{		
	TEST_CLASS(Monads)
	{
	public:
		
		TEST_METHOD(Compatibility)
		{
			int x = 3, y = 5;
			auto z = x | ~y;
			Assert::AreEqual(z, -5);
		}

		TEST_METHOD(Parsing)
		{
			auto l = "3,4,x6,5"
				| split(',')
				| parse<int>
				| square
				| [](auto n) { return to_string(n); };
			Assert::AreEqual(size_t(4u), l.size());
			Assert::AreEqual("9"s, l.front().value());	l.pop_front();
			Assert::AreEqual("16"s, l.front().value());	l.pop_front();
			Assert::AreEqual(false, (bool)l.front());	l.pop_front();
			Assert::AreEqual("25"s, l.front().value());	l.pop_front();
		}

		TEST_METHOD(MaybeMonad)
		{
			auto half = [](int x) { return x % 2 ? nothing : just(x/2); };
			auto chain = [=](auto x) { return x | square | half | square; };
			Assert::AreEqual(just(64), 4 | chain);
			Assert::AreEqual(nothing,  5 | chain);
			Assert::AreEqual(just(64), just(4) | chain);
			Assert::AreEqual(nothing,  nothing | chain);
		}

		TEST_METHOD(GeneratorMonad)
		{
			Assert::AreEqual(40, ints() | square | take(5) | filter(even) | double_gen | sum);
		}

		TEST_METHOD(AsyncMonad)
		{
			auto square_async = [](int x) { return async([](int n) { return n * n; }, x); };
			auto half_async   = [](int x) { return async([](int n) { return n % 2 ? nothing : just(n / 2); }, x); };
			auto get = [](auto&& f) { return f.get(); };

			Assert::AreEqual(just(160), 8 | square_async | mul(5) | half_async | ~get);
			Assert::AreEqual(98, list<int>{ 1, 2, 3 } | square_async | square_async | transform(get) | sum);
		}

		TEST_METHOD(WriterMonad)
		{
			auto half = [](int x) { return tell("I just halved " + to_string(x) + "!") | [x](auto) {return x / 2; }; };
			auto [val, msg] = runWriter(8 | half | half);
			Assert::AreEqual(2, val);
			Assert::AreEqual("I just halved 8!I just halved 4!"s, msg);
		}

		TEST_METHOD(ReaderMonad)
		{
			Assert::AreEqual(1234, runReader(ask(), 1234));
			auto greeter = ask() | [](auto name) { return returnR("hello, "s + name + "!"); };
			auto val = runReader(greeter, "world");
			Assert::AreEqual("hello, world!"s, val);
		}

		TEST_METHOD(CustomMonad)
		{
			using Bank = bank_account<int>;
			auto deposit  = [](auto money)	{ return [money](auto account) {return account + money; }; };
			auto withdraw = [](auto money)	{ return [money](auto account) {return account - money; }; };
			auto interest = [](auto pcent)	{ return [pcent](auto account) {return Bank{ account + account * pcent / 100 }; }; };
			auto check	  = [](auto account){ return account < 0 ? nothing : just(account); };
			auto amount   = [](auto&& b){ return b.amount(); };
			Assert::AreEqual(just(144), Bank(5)
				| deposit(4)
				| withdraw(5)
				| deposit(6)
				| interest(20)
				| check
				| square
				| ~amount
			);
			Assert::AreEqual(nothing, Bank(5)
				| withdraw(50)
				| deposit(3)
				| check
				| ~amount
			);
			Assert::AreEqual(2, copy_ctors);
			Assert::AreEqual(9, move_ctors);
			Assert::AreEqual(13, bank_ctors);
			Assert::AreEqual(24, bank_dtors);
		}

		TEST_METHOD(Performance)
		{
			map<string, string> m;
			for(int i = 0; i < ITER_COUNT; i++) {
				m = "a=3\nb=xyz\nnoval\n\n"
					| split_gen('\n')
					| [](auto&& sv) { return sv 
						| split_gen('=') 
						| reduce([](auto&& psv, auto&& sv) { if(psv.first.empty()) psv.first = sv; else psv.second = sv; return psv; }, pair<string_view, string_view>()); }
					| filter([](auto&& psv) {return !psv.second.empty(); })
					| ~[](auto&& g) { return map<string, string>(begin(g), end(g)); };
			}
			Assert::AreEqual(size_t(2u), m.size());
			Assert::AreEqual("3"s, m["a"s]);
			Assert::AreEqual("xyz"s, m["b"s]);
		}

	};

	TEST_CLASS(Reference) {
		TEST_METHOD(Performance) 
		{
			map<string, string> m;
			for(int i = 0; i < ITER_COUNT; i++) {
				auto l = split("a=3\nb=xyz\nnoval\n\n", '\n');
				for(auto&& sv : l) {
					auto kv = split(sv, '=');
					pair<string_view, string_view> p;
					if(!kv.empty())	p.first  = kv.front(), kv.pop_front();
					if(!kv.empty())	p.second = kv.front(), m.insert(p);
				}
			}
			Assert::AreEqual(size_t(2u), m.size());
			Assert::AreEqual("3"s, m["a"s]);
			Assert::AreEqual("xyz"s, m["b"s]);
		}
	};
}