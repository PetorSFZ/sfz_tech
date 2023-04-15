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
#ifndef SFZ_PRETTY_PRINT_H
#define SFZ_PRETTY_PRINT_H

#include <sfz.h>
#include <sfz_reflection.h>

#include <skipifzero_strings.hpp>

// Pretty print (API)
// ------------------------------------------------------------------------------------------------

// WARNING: This is incomplete and hilariously slow (quadratic behaviour because of sfzStrAppendf()).
//          But it's a proof of concept that could be cleaned up and improved upon.

// Pretty prints a visitable (i.e. SFZ_VISITABLE) variable using syntax that can be copy pasted
// into a C++ source document in order to create an identical instance.
// 
// Note: The resulting buffer is guaranteed to be null-terminated, but there is no guarantee that
//       the entire string will fit in it.
// 
// Prefer to use the SFZ_PRETTY_PRINT() macro below to automatically get the name of the variable.
template<typename T>
void sfzPrettyPrint(SfzStrView dst, const char* variable_name, const T& variable);

// This macro pretty prints a visitable (SFZ_VISITABLE) variable to the destination buffer.
#define SFZ_PRETTY_PRINT(dst, variable) sfzPrettyPrint((dst), (#variable), (variable))

// Type name
// ------------------------------------------------------------------------------------------------

#ifdef __cplusplus

// Experimental function to get the name of a type.
// See: https://blog.molecular-matters.com/2015/12/11/getting-the-type-of-a-template-argument-as-string-without-rtti/
template<typename T>
constexpr SfzStrLit64 sfzTypeName()
{
#ifdef _WIN32
	constexpr char PREFIX[] = "struct SfzStrLit64 __cdecl sfzTypeName<";
	constexpr char POSTFIX[] = ">(void)";
	constexpr i32 PREFIX_LEN = sizeof(PREFIX) - 1;
	constexpr i32 POSTFIX_LEN = sizeof(POSTFIX) - 1;
	constexpr i32 SIG_LEN = sizeof(__FUNCSIG__) - 1;
	i32 NAME_LEN = SIG_LEN - PREFIX_LEN - POSTFIX_LEN;
	const char* NAME_STR = __FUNCSIG__ + PREFIX_LEN;
	if (7 < NAME_LEN &&
		NAME_STR[0] == 's' &&
		NAME_STR[1] == 't' &&
		NAME_STR[2] == 'r' &&
		NAME_STR[3] == 'u' &&
		NAME_STR[4] == 'c' &&
		NAME_STR[5] == 't' &&
		NAME_STR[6] == ' ') {
		NAME_STR += 7;
		NAME_LEN -= 7;
	}
	else if (6 < NAME_LEN &&
		NAME_STR[0] == 'c' &&
		NAME_STR[1] == 'l' &&
		NAME_STR[2] == 'a' &&
		NAME_STR[3] == 's' &&
		NAME_STR[4] == 's' &&
		NAME_STR[5] == ' ') {
		NAME_STR += 6;
		NAME_LEN -= 6;
	}
#else
#error "Need to implement for this compiler"
#endif
	SfzStrLit64 name = {};
	name.len = NAME_LEN;
	for (i32 i = 0; i < NAME_LEN; i++) {
		name.str[i] = NAME_STR[i];
	}
	return name;
}

template<> constexpr SfzStrLit64 sfzTypeName<bool>() { return SfzStrLit64("bool"); }
template<> constexpr SfzStrLit64 sfzTypeName<char>() { return SfzStrLit64("char"); }

template<> constexpr SfzStrLit64 sfzTypeName<i8>() { return SfzStrLit64("i8"); }
template<> constexpr SfzStrLit64 sfzTypeName<i16>() { return SfzStrLit64("i16"); }
template<> constexpr SfzStrLit64 sfzTypeName<i32>() { return SfzStrLit64("i32"); }
template<> constexpr SfzStrLit64 sfzTypeName<i64>() { return SfzStrLit64("i64"); }

template<> constexpr SfzStrLit64 sfzTypeName<u8>() { return SfzStrLit64("u8"); }
template<> constexpr SfzStrLit64 sfzTypeName<u16>() { return SfzStrLit64("u16"); }
template<> constexpr SfzStrLit64 sfzTypeName<u32>() { return SfzStrLit64("u32"); }
template<> constexpr SfzStrLit64 sfzTypeName<u64>() { return SfzStrLit64("u64"); }

template<> constexpr SfzStrLit64 sfzTypeName<f32>() { return SfzStrLit64("f32"); }
template<> constexpr SfzStrLit64 sfzTypeName<f64>() { return SfzStrLit64("f64"); }

#endif // __cplusplus

// Pretty print
// ------------------------------------------------------------------------------------------------

struct SfzPrettyPrintVisitor {

	SfzStrView dst = {};
	i32 indent = 0;

	void applyIndent(i32 num_extra = 0)
	{
		for (i32 i = 0; i < (indent + num_extra); i++) sfzStrAppendChars(dst, "\t", 1);
	}

	template<typename T>
	void printPrimitiveValue(const T& member)
	{
		if constexpr (sfzIsSameType<T, bool>()) {
			const char* val = member ? "true" : "false";
			sfzStrAppendf(dst, "%s", val);
		}
		else if constexpr (sfzIsSameType<T, char>()) {
			sfzStrAppendf(dst, "%c", member);
		}
		else if constexpr (
			sfzIsSameType<T, i8>() ||
			sfzIsSameType<T, i16>() ||
			sfzIsSameType<T, i32>() ||
			sfzIsSameType<T, i64>()) {

			const i64 val = i64(member);
			sfzStrAppendf(dst, "%lli", val);
		}
		else if constexpr (
			sfzIsSameType<T, u8>() ||
			sfzIsSameType<T, u16>() ||
			sfzIsSameType<T, u32>() ||
			sfzIsSameType<T, u64>()) {

			const u64 val = u64(member);
			sfzStrAppendf(dst, "%llu", val);
		}
		else if constexpr (
			sfzIsSameType<T, f32>() ||
			sfzIsSameType<T, f64>()) {

			const f64 val = f64(member);
			sfzStrAppendf(dst, "%f", val);
		}
		
		else if constexpr (sfzIsSameType<T, i32x2>()) {
			sfzStrAppendf(dst, "i32x2{ %i, %i }", member.x, member.y);
		}
		else if constexpr (sfzIsSameType<T, i32x3>()) {
			sfzStrAppendf(dst, "i32x3{ %i, %i, %i }", member.x, member.y, member.z);
		}
		else if constexpr (sfzIsSameType<T, i32x4>()) {
			sfzStrAppendf(dst, "i32x4{ %i, %i, %i, %i }", member.x, member.y, member.z, member.w);
		}

		else if constexpr (sfzIsSameType<T, f32x2>()) {
			sfzStrAppendf(dst, "f32x2{ %f, %f }", member.x, member.y);
		}
		else if constexpr (sfzIsSameType<T, f32x3>()) {
			sfzStrAppendf(dst, "f32x3{ %f, %f, %f }", member.x, member.y, member.z);
		}
		else if constexpr (sfzIsSameType<T, f32x4>()) {
			sfzStrAppendf(dst, "f32x4{ %f, %f, %f, %f }", member.x, member.y, member.z, member.w);
		}

		else if constexpr (sfzIsSameType<T, u8x2>()) {
			sfzStrAppendf(dst, "u8x2{ %u, %u }", u32(member.x), u32(member.y));
		}
		else if constexpr (sfzIsSameType<T, u8x4>()) {
			sfzStrAppendf(dst, "u8x4{ %u, %u, %u, %u }", u32(member.x), u32(member.y), u32(member.z), u32(member.w));
		}

		else {
			static_assert(false, "This is not a known primitive, can't print it");
		}
	}

	template<const SfzMemberMeta meta, typename T>
	void visit(const T& member)
	{
		// If the member is a primitive value, print it
		if constexpr (
			sfzIsSameType<T, bool>() ||
			sfzIsSameType<T, char>() ||

			sfzIsSameType<T, i8>() ||
			sfzIsSameType<T, i16>() ||
			sfzIsSameType<T, i32>() ||
			sfzIsSameType<T, i64>() ||

			sfzIsSameType<T, u8>() ||
			sfzIsSameType<T, u16>() ||
			sfzIsSameType<T, u32>() ||
			sfzIsSameType<T, u64>() ||

			sfzIsSameType<T, f32>() ||
			sfzIsSameType<T, f64>() ||

			sfzIsSameType<T, i32x2>() ||
			sfzIsSameType<T, i32x3>() ||
			sfzIsSameType<T, i32x4>() ||

			sfzIsSameType<T, f32x2>() ||
			sfzIsSameType<T, f32x3>() ||
			sfzIsSameType<T, f32x4>() ||
			
			sfzIsSameType<T, u8x2>() ||
			sfzIsSameType<T, u8x4>()) {

			applyIndent();
			sfzStrAppendf(dst, ".%s = ", meta.name.str);
			printPrimitiveValue(member);
			sfzStrAppendf(dst, ",\n");
		}

		// If the member is a C-array, print all of its members
		else if constexpr (sfzIsCArray<T>()) {
			using ElemT = SfzCArrayInfo<T>::ElemT;
			constexpr u32 ARRAY_SIZE = SfzCArrayInfo<T>::size;

			// Special case for strings
			if constexpr (sfzIsSameType<ElemT, char>()) {
				applyIndent();
				sfzStrAppendf(dst, ".%s = \"", meta.name.str);
				for (u32 i = 0; i < ARRAY_SIZE; i++) {
					char c = member[i];
					if (c == '\0') break;
					sfzStrAppendChars(dst, &c, 1);
				}
				sfzStrAppendf(dst, "\",\n");
			}
			else {
				applyIndent();
				sfzStrAppendf(dst, ".%s = {\n", meta.name.str);
				for (u32 i = 0; i < ARRAY_SIZE; i++) {
					applyIndent(1);
					// Note: This doesn't ~quite~ work. If it's an array of non-primitives then it's
					//       going to fail.
					printPrimitiveValue(member[i]);
					sfzStrAppendf(dst, ",\n");
				}
				applyIndent();
				sfzStrAppendf(dst, "},\n");
			}
		}

		else if constexpr (sfzIsVisitable<T>()) {
			applyIndent();
			sfzStrAppendf(dst, ".%s = %s{\n", meta.name.str, sfzTypeName<T>().str);

			SfzPrettyPrintVisitor inner_visitor = *this;
			inner_visitor.indent += 1;
			sfzVisit(member, inner_visitor);

			applyIndent();
			sfzStrAppendf(dst, "},\n");
		}

		else {
			static_assert(false, "Unknown type, did you forget to register it?");
		}
	}
};

template<typename T>
void sfzPrettyPrint(SfzStrView dst, const char* variable_name, const T& variable)
{
	sfzStrClear(dst);
	sfzStrAppendf(dst, "%s %s = {\n", sfzTypeName<T>().str, variable_name);
	SfzPrettyPrintVisitor v = { .dst = dst, .indent = 1, };
	sfzVisit(variable, v);
	sfzStrAppendf(dst, "};\n");
}

#endif // SFZ_PRETTY_PRINT_H
