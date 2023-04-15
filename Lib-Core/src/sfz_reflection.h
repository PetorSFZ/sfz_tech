// Copyright (c) Peter Hillerström (skipifzero.com, peter@hstroem.se)
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#pragma once
#ifndef SFZ_REFLECTION_H
#define SFZ_REFLECTION_H

#include <sfz.h>

// For each macro
// ------------------------------------------------------------------------------------------------

// For each macro implemented using the new C++20 "__VA_OPT__", signficantly cleaner and better
// than what was possible previously. Requires "/Zc:preprocessor" on MSVC.
//
// Implementation from: https://www.scs.stanford.edu/~dm/blog/va-opt.html#the-for_each-macro
// (Note: "I place all the cpp macros in this blog post in the public domain.")
// Also see: https://docs.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview?view=msvc-170

#define SFZ_PARENS ()

#define SFZ_EXPAND(...) SFZ_EXPAND4(SFZ_EXPAND4(SFZ_EXPAND4(SFZ_EXPAND4(__VA_ARGS__))))
#define SFZ_EXPAND4(...) SFZ_EXPAND3(SFZ_EXPAND3(SFZ_EXPAND3(SFZ_EXPAND3(__VA_ARGS__))))
#define SFZ_EXPAND3(...) SFZ_EXPAND2(SFZ_EXPAND2(SFZ_EXPAND2(SFZ_EXPAND2(__VA_ARGS__))))
#define SFZ_EXPAND2(...) SFZ_EXPAND1(SFZ_EXPAND1(SFZ_EXPAND1(SFZ_EXPAND1(__VA_ARGS__))))
#define SFZ_EXPAND1(...) __VA_ARGS__

#define SFZ_FOR_EACH(macro, ...) \
	__VA_OPT__(SFZ_EXPAND(SFZ_FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define SFZ_FOR_EACH_HELPER(macro, a1, ...) \
	macro(a1) \
	__VA_OPT__(SFZ_FOR_EACH_AGAIN SFZ_PARENS (macro, __VA_ARGS__))
#define SFZ_FOR_EACH_AGAIN() SFZ_FOR_EACH_HELPER

// Variant of the for each macro that takes 2 arguments from the va list per iteration.

#define SFZ_FOR_EACH_2(macro, ...) \
	__VA_OPT__(SFZ_EXPAND(SFZ_FOR_EACH_HELPER_2(macro, __VA_ARGS__)))
#define SFZ_FOR_EACH_HELPER_2(macro, a1, a2, ...) \
	macro(a1, a2) \
	__VA_OPT__(SFZ_FOR_EACH_AGAIN_2 SFZ_PARENS (macro, __VA_ARGS__))
#define SFZ_FOR_EACH_AGAIN_2() SFZ_FOR_EACH_HELPER_2

// Concat name macro
// ------------------------------------------------------------------------------------------------

// Simple macro used to concatinate 2 strings into a single one (for function or variable names).
// It uses 2-passes in order to make it work with special macros such as __COUNTER__ and similar.

#define SFZ_CONCAT_NAME_INNER(x, y) x##y
#define SFZ_CONCAT_NAME(x, y) SFZ_CONCAT_NAME_INNER(x, y)

// Compare types
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

// Function to check if two types are the same. This is useful in combination with constexpr if:
//
// template<typename T>
// void foo(T bar)
// {
//     if constexpr (sfzIsSameType<T, int>()) {
//         // int path
//     }
//     else if constexpr (sfzIsSameType<T, float>()) {
//         // float path
//     }
// }

template<typename T1, typename T2> struct SfzIsSameType { static constexpr bool value = false; };
template<typename T> struct SfzIsSameType<T, T> { static constexpr bool value = true; };
template<typename T1, typename T2> constexpr bool sfzIsSameType() { return SfzIsSameType<T1, T2>::value; }

// Helper to check for info about C-arrays
template<typename T> struct SfzCArrayInfo { static constexpr bool is_array = false; };
template<typename T, u32 SIZE>
struct SfzCArrayInfo<T[SIZE]> {
	static constexpr bool is_array = true;
	using ElemT = T;
	static constexpr u32 size = SIZE;
};
template<typename T> constexpr bool sfzIsCArray() { return SfzCArrayInfo<T>::is_array; }

#endif // __cplusplus

// String literal
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

// Simple compile-time string literal type. Used in order to be able to send string literals as
// template parameters.
struct SfzStrLit64 {
	char str[64] = {};
	u32 len = 0;

	constexpr SfzStrLit64() = default;

	template<u32 N>
	constexpr SfzStrLit64(const char(&str_in)[N])
	{
		static_assert(N < 64, "String too long");
		for (u32 i = 0; i < N; i++) this->str[i] = str_in[i];
		this->len = N - 1;
	}

	constexpr bool operator== (const char* o) const
	{
		for (u32 i = 0; i < len; i++) {
			if (str[i] != o[i]) return false;
		}
		return true;
	}
	constexpr bool operator!= (const char* o) const { return !(*this == o); }
};

#endif

// Member tag
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

struct SfzMemberTag {

	// If the member is read optional it doesn't need to be read from serialized data if it doesn't
	// exist there. It does not imply "write-optional", unless otherwise specified it still needs
	// to be written to serialized data.
	bool read_optional = false;
};

#endif // __cplusplus

// Member metadata
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

struct SfzMemberMeta {
	SfzStrLit64 name;
	SfzMemberTag tag;

	template<u32 N>
	constexpr SfzMemberMeta(const char(&name_in)[N], const SfzMemberTag& tag_in) : name(name_in), tag(tag_in) {}
};

#endif // __cplusplus

// Visit struct
// ------------------------------------------------------------------------------------------------

// A simplified implementation of visit_struct (https://github.com/garbageslam/visit_struct), but
// uses the new C++20 "__VA_OPT__ " for a cleaner implementation. Additionally, allows for tagging
// members with additional metadata, and passes the name and the tagging info as a compile-time
// parameter so that it can be used in static_assert and constexpr if.
//
// Usage:
//
// struct Foo {
//     int bar;
//     float car;
// };
// SFZ_VISITABLE(Foo,
//    bar, {},
//    car, { .read_optional = true });
//
// struct PrintVisitor {
//     
//     template<const SfzMemberMeta meta, typename T>
//     void visit(T member)
//     {
//         if constexpr (sfzIsSameType<T, int>()) {
//             printf("%s: %i\n", meta.name.str, member);
//         }
//         else if constexpr (sfzIsSameType<T, float>()) {
//             printf("%s: %f\n", meta.name.str, member);
//         }
//         else {
//             static_assert(false, "Unhandled type");
//         }
//     }
// };
//
// PrintVisitor v;
// Foo foo = { 2, 3.0f };
// sfzVisit(v, foo);
// static_assert(sfzIsVisitable<Foo>(), "Can use this to check if a type is visitable");
//
//
// You can also visit two instances at the same time (e.g. for implementing copying or comparision):
//
// struct EqualityVisitor {
//     bool equals = true;
//
//     template<const SfzMemberMeta meta, typename T>
//     void visit(const T& member1, const T& member2)
//     {
//         if (member1 != member2) {
//             equals = false;
//         }
//     }
// };
//
// EqualityVisitor eq_visitor = {};
// sfzVisit2(foo1, foo2, eq_visitor);
// const bool equals = eq_visitor.equals;

#ifdef __cplusplus

#define SFZ_VISIT_MEMBER_VISITOR(member, tag) v.visit<SfzMemberMeta(#member, tag)>(type.member);
#define SFZ_VISIT_MEMBER2_VISITOR(member, tag) v.visit<SfzMemberMeta(#member, tag)>(type1.member, type2.member);
#define SFZ_VISIT_MEMBER_LAMBDA(member, tag) v(SfzMemberMeta(#member, tag), type.member);
#define SFZ_VISIT_MEMBER2_LAMBDA(member, tag) v(SfzMemberMeta(#member, tag), type1.member, type2.member);

#define SFZ_VISITABLE(T, ...) \
template<typename GenericT, typename V> \
void SFZ_CONCAT_NAME(sfzVisit_, T)(GenericT type, V v) \
{ \
	SFZ_FOR_EACH_2(SFZ_VISIT_MEMBER_VISITOR, __VA_ARGS__) \
} \
template<typename V> void sfzVisit(T& type, V& v) { SFZ_CONCAT_NAME(sfzVisit_, T)<T&, V&>(type, v); } \
template<typename V> void sfzVisit(const T& type, V& v) { SFZ_CONCAT_NAME(sfzVisit_, T)<const T&, V&>(type, v); } \
\
template<typename GenericT1, typename GenericT2, typename V> \
void SFZ_CONCAT_NAME(sfzVisit2_, T)(GenericT1 type1, GenericT2 type2, V v) \
{ \
	SFZ_FOR_EACH_2(SFZ_VISIT_MEMBER2_VISITOR, __VA_ARGS__) \
} \
template<typename V> void sfzVisit2(T& type1, T& type2, V& v) { SFZ_CONCAT_NAME(sfzVisit2_, T)<T&, T&, V&>(type1, type2, v); } \
template<typename V> void sfzVisit2(const T& type, const T& type2, V& v) { SFZ_CONCAT_NAME(sfzVisit2_, T)<const T&, const T&, V&>(type1, type2, v); } \
\
template<typename GenericT, typename V> \
void SFZ_CONCAT_NAME(sfzVisitLambda_, T)(GenericT type, V v) \
{ \
	SFZ_FOR_EACH_2(SFZ_VISIT_MEMBER_LAMBDA, __VA_ARGS__) \
} \
template<typename V> void sfzVisitLambda(T& type, V&& v) { SFZ_CONCAT_NAME(sfzVisitLambda_, T)<T&, V&&>(type, static_cast<V&&>(v)); } \
template<typename V> void sfzVisitLambda(const T& type, V&& v) { SFZ_CONCAT_NAME(sfzVisitLambda_, T)<const T&, V&&>(type, static_cast<V&&>(v)); } \
\
template<typename GenericT1, typename GenericT2, typename V> \
void SFZ_CONCAT_NAME(sfzVisit2Lambda_, T)(GenericT1 type1, GenericT2 type2, V v) \
{ \
	SFZ_FOR_EACH_2(SFZ_VISIT_MEMBER2_LAMBDA, __VA_ARGS__) \
} \
template<typename V> void sfzVisit2Lambda(T& type1, T& type2, V&& v) { SFZ_CONCAT_NAME(sfzVisit2Lambda_, T)<T&, T&, V&&>(type1, type2, static_cast<V&&>(v)); } \
template<typename V> void sfzVisit2Lambda(const T& type1, const T& type2, V&& v) { SFZ_CONCAT_NAME(sfzVisit2Lambda_, T)<const T&, const T&, V&&>(type1, type2, static_cast<V&&>(v)); } \
\
template<> constexpr bool sfzIsVisitable<T>() { return true; }

template<typename T>
constexpr bool sfzIsVisitable() { return false; }

#else // __cplusplus

// Can't implement this in C, so do nothing.
#define SFZ_VISITABLE(T, ...)

#endif // __cplusplus

#endif // SFZ_REFLECTION_H
