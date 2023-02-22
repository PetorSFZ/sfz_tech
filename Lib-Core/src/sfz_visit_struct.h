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
#ifndef SFZ_VISIT_STRUCT_HPP
#define SFZ_VISIT_STRUCT_HPP

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

// visit struct type traits
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

// Function to check if two types are the same. This is useful if using a function as visitor to
// sfzForEachMember() with constexpr if. Example:
//
// template<typename T>
// void visitFunc(const char* name, T& member)
// {
//     if constexpr (sfzIsSameType<T, int>()) {
//         // Path if member type is int
//     }
//     if constexpr (sfzIsSameType<T, float>()) {
//         // Path if member type is float
//     }
// }

template<typename T1, typename T2> struct SfzIsSameType { static constexpr bool value = false; };
template<typename T> struct SfzIsSameType<T, T> { static constexpr bool value = true; };
template<typename T1, typename T2> constexpr bool sfzIsSameType() { return SfzIsSameType<T1, T2>::value; }

#endif // __cplusplus

// Visit struct
// ------------------------------------------------------------------------------------------------

// A simplified implementation of visit_struct (https://github.com/garbageslam/visit_struct), but
// uses the new C++20 "__VA_OPT__ " to implement a significantly better for each macro than what
// was possible before.
//
// Usage:
//
// struct Foo {
//     int bar;
//     float car;
// };
// SFZ_VISITABLE(Foo, bar, car);
//
// struct PrintVisitor {
//     void operator()(const char* name, int val) {
//         printf("%s: %i\n", name, val);
//     }
//     void operator() (const char* name, float val) {
//         printf("%s: %f\n", name, val);
//     }
// };
//
// PrintVisitor v;
// Foo foo = { 2, 3.0f };
// sfzForEachMember(v, foo);
// static_assert(sfzIsVisitable<Foo>(), "Can use this to check if a type is visitable");

#ifdef __cplusplus

#define SFZ_VISIT_MEMBER(x) v(#x, type.x);

#define SFZ_VISITABLE(T, ...) \
template<typename V> \
void sfzForEachMember(T& type, V& v) \
{ \
	SFZ_FOR_EACH(SFZ_VISIT_MEMBER, __VA_ARGS__) \
} \
template<typename V> \
void sfzForEachMember(T& type, V&& v) \
{ \
	SFZ_FOR_EACH(SFZ_VISIT_MEMBER, __VA_ARGS__) \
} \
template<typename V> \
void sfzForEachMember(const T& type, V& v) \
{ \
	SFZ_FOR_EACH(SFZ_VISIT_MEMBER, __VA_ARGS__) \
} \
template<typename V> \
void sfzForEachMember(const T& type, V&& v) \
{ \
	SFZ_FOR_EACH(SFZ_VISIT_MEMBER, __VA_ARGS__) \
} \
template<> constexpr bool sfzIsVisitable<T>() { return true; }

template<typename T>
constexpr bool sfzIsVisitable() { return false; }

#else // __cplusplus

// Can't implement this in C, so do nothing.
#define SFZ_VISITABLE(T, ...)

#endif // __cplusplus

#endif // SFZ_VISIT_STRUCT_HPP
