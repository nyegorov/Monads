#include "stdafx.h"
#include "CppUnitTest.h"
#include "../monads.h"

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
auto sum() { return ~[](auto&& g) { return accumulate(begin(g), end(g), 0, plus<int>()); }; }
auto insert = [](auto&& m, auto&& e) { m.insert(std::move(e)); return m; };

auto split(char c) { return [c](string_view s) {return split(s, c); }; }
auto split_gen(char c) { return [c](string_view s) {return split_gen(s, c); }; }
template<class T> optional<T> parse(string_view s);
template<> optional<int> parse<int>(string_view s) { try { return optional<int>(stoi(s.data())); } catch(...) {} return optional<int>{}; };

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
template <class A> struct fmap<bank_account<A>> { template<class F> auto operator() (bank_account<A>& ma, F f) { return bank_account<decltype(declval<A>() | f)>(ma.amount() | f); }; };
template <class A> struct join<bank_account<bank_account<A>>> { auto operator() (bank_account<bank_account<A>>&& mma) { return mma.amount(); }; };

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

		TEST_METHOD(Optional)
		{
			auto half = [](int x) { return x % 2 ? nothing : just(x/2); };
			auto chain = [=](auto x) { return x | square | half | square; };
			Assert::AreEqual(just(64), 4 | chain);
			Assert::AreEqual(nothing,  5 | chain);
			Assert::AreEqual(just(64), just(4) | chain);
			Assert::AreEqual(nothing,  nothing | chain);
		}

		TEST_METHOD(Generator)
		{
			Assert::AreEqual(20, ints() | square | take(5) | filter(even) | sum());
		}

		TEST_METHOD(Parsing)
		{
			auto l = "3,4,x6,5"
				| split(',')
				| parse<int>
				| square
				| [](auto n) { return to_string(n); };
			Assert::AreEqual(4u, l.size());
			Assert::AreEqual("9"s, l.front().value());	l.pop_front();
			Assert::AreEqual("16"s, l.front().value());	l.pop_front();
			Assert::AreEqual(false, (bool)l.front());	l.pop_front();
			Assert::AreEqual("25"s, l.front().value());	l.pop_front();
		}

		TEST_METHOD(CustomType)
		{
			using Bank = bank_account<int>;
			auto deposit  = [](auto dx) { return [dx](auto x) {return x + dx; }; };
			auto withdraw = [](auto dx) { return [dx](auto x) {return x - dx; }; };
			auto check	  = [](auto x)  { return x < 0 ? nothing : just(x); };
			auto amount   = [](auto&& b){ return b.amount(); };
			Assert::AreEqual(just(49), Bank(5)
				| deposit(4)
				| withdraw(5)
				| deposit(3)
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
			Assert::AreEqual(0, copy_ctors);
			Assert::AreEqual(8, move_ctors);
			Assert::AreEqual(10, bank_ctors);
			Assert::AreEqual(18, bank_dtors);
		}

		TEST_METHOD(Performance)
		{
			map<string, string> m;
#ifdef _DEBUG
			for(int i = 0; i < 1'000; i++) {
#else
			for(int i = 0; i < 100'000; i++) {
#endif
				m = "a=3\nb=xyz\nnoval\n\n"
					| split_gen('\n')
					| [](auto&& sv) { return sv 
						| split_gen('=') 
						| reduce([](auto&& psv, auto&& sv) { if(psv.first.empty()) psv.first = sv; else psv.second = sv; return psv; }, pair<string_view, string_view>()); }
					| filter([](auto&& psv) {return !psv.second.empty(); })
					| ~[](auto&& g) { return map<string, string>(begin(g), end(g)); };
			}
			Assert::AreEqual(2u, m.size());
			Assert::AreEqual("3"s, m["a"s]);
			Assert::AreEqual("xyz"s, m["b"s]);
		}

	};
}