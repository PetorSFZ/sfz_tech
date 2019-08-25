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

namespace sfz {

// Vector struct declaration: Vector<T,N>
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr Vector<T,N>::Vector(const T* arrayPtr) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		elements[i] = arrayPtr[i];
	}
}

template<typename T, uint32_t N>
template<typename T2>
constexpr Vector<T,N>::Vector(const Vector<T2,N>& other) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		elements[i] = static_cast<T>(other[i]);
	}
}

template<typename T, uint32_t N>
constexpr T& Vector<T,N>::operator[] (uint32_t index) noexcept
{
	return elements[index];
}

template<typename T, uint32_t N>
constexpr T Vector<T,N>::operator[] (uint32_t index) const noexcept
{
	return elements[index];
}

// Vector struct declaration: Vector<T,2>
// ------------------------------------------------------------------------------------------------

template<typename T>
constexpr Vector<T,2>::Vector(const T* arrayPtr) noexcept
:
	x(arrayPtr[0]),
	y(arrayPtr[1])
{ }

template<typename T>
constexpr Vector<T,2>::Vector(T value) noexcept
:
	x(value),
	y(value)
{ }

template<typename T>
constexpr Vector<T,2>::Vector(T x, T y) noexcept
:
	x(x),
	y(y)
{ }

template<typename T>
template<typename T2>
constexpr Vector<T,2>::Vector(const Vector<T2,2>& other) noexcept
:
	x(static_cast<T>(other.x)),
	y(static_cast<T>(other.y))
{ }

template<typename T>
constexpr T& Vector<T,2>::operator[] (uint32_t index) noexcept
{
	return data()[index];
}

template<typename T>
constexpr T Vector<T,2>::operator[] (uint32_t index) const noexcept
{
	return data()[index];
}

// Vector struct declaration: Vector<T,3>
// ------------------------------------------------------------------------------------------------

template<typename T>
constexpr Vector<T,3>::Vector(const T* arrayPtr) noexcept
:
	x(arrayPtr[0]),
	y(arrayPtr[1]),
	z(arrayPtr[2])
{ }

template<typename T>
constexpr Vector<T,3>::Vector(T value) noexcept
:
	x(value),
	y(value),
	z(value)
{ }

template<typename T>
constexpr Vector<T,3>::Vector(T x, T y, T z) noexcept
:
	x(x),
	y(y),
	z(z)
{ }

template<typename T>
constexpr Vector<T,3>::Vector(Vector<T,2> xy, T z) noexcept
:
	x(xy.x),
	y(xy.y),
	z(z)
{ }

template<typename T>
constexpr Vector<T,3>::Vector(T x, Vector<T,2> yz) noexcept
:
	x(x),
	y(yz.x),
	z(yz.y)
{ }

template<typename T>
template<typename T2>
constexpr Vector<T,3>::Vector(const Vector<T2,3>& other) noexcept
:
	x(static_cast<T>(other.x)),
	y(static_cast<T>(other.y)),
	z(static_cast<T>(other.z))
{ }

template<typename T>
constexpr T& Vector<T,3>::operator[] (uint32_t index) noexcept
{
	return data()[index];
}

template<typename T>
constexpr T Vector<T,3>::operator[] (uint32_t index) const noexcept
{
	return data()[index];
}

// Vector struct declaration: Vector<T,4>
// ------------------------------------------------------------------------------------------------

template<typename T>
constexpr Vector<T,4>::Vector(const T* arrayPtr) noexcept
:
	x(arrayPtr[0]),
	y(arrayPtr[1]),
	z(arrayPtr[2]),
	w(arrayPtr[3])
{ }

template<typename T>
constexpr Vector<T,4>::Vector(T value) noexcept
:
	x(value),
	y(value),
	z(value),
	w(value)
{ }

template<typename T>
constexpr Vector<T,4>::Vector(T x, T y, T z, T w) noexcept
:
	x(x),
	y(y),
	z(z),
	w(w)
{ }

template<typename T>
constexpr Vector<T,4>::Vector(Vector<T,3> xyz, T w) noexcept
:
	x(xyz.x),
	y(xyz.y),
	z(xyz.z),
	w(w)
{ }

template<typename T>
constexpr Vector<T,4>::Vector(T x, Vector<T,3> yzw) noexcept
:
	x(x),
	y(yzw.x),
	z(yzw.y),
	w(yzw.z)
{ }

template<typename T>
constexpr Vector<T,4>::Vector(Vector<T,2> xy, Vector<T,2> zw) noexcept
:
	x(xy.x),
	y(xy.y),
	z(zw.x),
	w(zw.y)
{ }

template<typename T>
constexpr Vector<T,4>::Vector(Vector<T,2> xy, T z, T w) noexcept
:
	x(xy.x),
	y(xy.y),
	z(z),
	w(w)
{ }

template<typename T>
constexpr Vector<T,4>::Vector(T x, Vector<T,2> yz, T w) noexcept
:
	x(x),
	y(yz.x),
	z(yz.y),
	w(w)
{ }

template<typename T>
constexpr Vector<T,4>::Vector(T x, T y, Vector<T,2> zw) noexcept
:
	x(x),
	y(y),
	z(zw.x),
	w(zw.y)
{ }

template<typename T>
template<typename T2>
constexpr Vector<T,4>::Vector(const Vector<T2,4>& other) noexcept
:
	x(static_cast<T>(other.x)),
	y(static_cast<T>(other.y)),
	z(static_cast<T>(other.z)),
	w(static_cast<T>(other.w))
{ }

template<typename T>
constexpr T& Vector<T,4>::operator[] (uint32_t index) noexcept
{
	return data()[index];
}

template<typename T>
constexpr T Vector<T,4>::operator[] (uint32_t index) const noexcept
{
	return data()[index];
}

// Vector functions: dot()
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr T dot(Vector<T,N> lhs, Vector<T,N> rhs) noexcept
{
	T product = T(0);
	for (uint32_t i = 0; i < N; i++) {
		product += lhs[i] * rhs[i];
	}
	return product;
}

template<typename T>
constexpr T dot(Vector<T,2> lhs, Vector<T,2> rhs) noexcept
{
	return lhs.x * rhs.x
	     + lhs.y * rhs.y;
}

template<typename T>
constexpr T dot(Vector<T,3> lhs, Vector<T,3> rhs) noexcept
{
	return lhs.x * rhs.x
	     + lhs.y * rhs.y
	     + lhs.z * rhs.z;
}

template<typename T>
constexpr T dot(Vector<T,4> lhs, Vector<T,4> rhs) noexcept
{
	return lhs.x * rhs.x
	     + lhs.y * rhs.y
	     + lhs.z * rhs.z
	     + lhs.w * rhs.w;
}

// Vector functions: length()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float length(vec2 v) noexcept
{
	return std::sqrt(dot(v, v));
}

SFZ_CUDA_CALL float length(vec3 v) noexcept
{
	return std::sqrt(dot(v, v));
}

SFZ_CUDA_CALL float length(vec4 v) noexcept
{
	return std::sqrt(dot(v, v));
}

// Vector functions: normalize()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2 normalize(vec2 v) noexcept
{
	return v * (1.0f / sfz::length(v));
}

SFZ_CUDA_CALL vec3 normalize(vec3 v) noexcept
{
	return v * (1.0f / sfz::length(v));
}

SFZ_CUDA_CALL vec4 normalize(vec4 v) noexcept
{
	return v * (1.0f / sfz::length(v));
}

// Vector functions: safeNormalize()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2 safeNormalize(vec2 v) noexcept
{
	float lengthTmp = sfz::length(v);
	if (lengthTmp == 0.0f || lengthTmp == -0.0f) return vec2(0.0f);
	return v / lengthTmp;
}

SFZ_CUDA_CALL vec3 safeNormalize(vec3 v) noexcept
{
	float lengthTmp = sfz::length(v);
	if (lengthTmp == 0.0f || lengthTmp == -0.0f) return vec3(0.0f);
	return v / lengthTmp;
}

SFZ_CUDA_CALL vec4 safeNormalize(vec4 v) noexcept
{
	float lengthTmp = sfz::length(v);
	if (lengthTmp == 0.0f || lengthTmp == -0.0f) return vec4(0.0f);
	return v / lengthTmp;
}

// Vector functions: cross()
// ------------------------------------------------------------------------------------------------

template<typename T>
constexpr Vector<T,3> cross(Vector<T,3> lhs, Vector<T,3> rhs) noexcept
{
	return Vector<T,3>(lhs.y * rhs.z - lhs.z * rhs.y,
	                   lhs.z * rhs.x - lhs.x * rhs.z,
	                   lhs.x * rhs.y - lhs.y * rhs.x);
}

// Vector functions: elementSum()
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr T elementSum(Vector<T,N> v) noexcept
{
	T result = T(0);
	for (uint32_t i = 0; i < N; ++i) {
		result += v[i];
	}
	return result;
}

template<typename T>
constexpr T elementSum(Vector<T,2> v) noexcept
{
	return v.x + v.y;
}

template<typename T>
constexpr T elementSum(Vector<T,3> v) noexcept
{
	return v.x + v.y + v.z;
}

template<typename T>
constexpr T elementSum(Vector<T,4> v) noexcept
{
	return v.x + v.y + v.z + v.w;
}

// Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr Vector<T,N>& operator+= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] += right.elements[i];
	}
	return left;
}

template<typename T>
constexpr Vector<T,2>& operator+= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x += right.x;
	left.y += right.y;
	return left;
}

template<typename T>
constexpr Vector<T,3>& operator+= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;
	return left;
}

template<typename T>
constexpr Vector<T,4>& operator+= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;
	left.w += right.w;
	return left;
}

template<typename T, uint32_t N>
constexpr Vector<T,N>& operator-= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] -= right.elements[i];
	}
	return left;
}

template<typename T>
constexpr Vector<T,2>& operator-= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;
	return left;
}

template<typename T>
constexpr Vector<T,3>& operator-= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;
	return left;
}

template<typename T>
constexpr Vector<T,4>& operator-= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;
	left.w -= right.w;
	return left;
}

template<typename T, uint32_t N>
constexpr Vector<T,N>& operator*= (Vector<T,N>& left, T right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] *= right;
	}
	return left;
}

template<typename T>
constexpr Vector<T,2>& operator*= (Vector<T,2>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;
	return left;
}

template<typename T>
constexpr Vector<T,3>& operator*= (Vector<T,3>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;
	left.z *= right;
	return left;
}

template<typename T>
constexpr Vector<T,4>& operator*= (Vector<T,4>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;
	left.z *= right;
	left.w *= right;
	return left;
}

template<typename T, uint32_t N>
constexpr Vector<T,N>& operator*= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] *= right.elements[i];
	}
	return left;
}

template<typename T>
constexpr Vector<T,2>& operator*= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x *= right.x;
	left.y *= right.y;
	return left;
}

template<typename T>
constexpr Vector<T,3>& operator*= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x *= right.x;
	left.y *= right.y;
	left.z *= right.z;
	return left;
}

template<typename T>
constexpr Vector<T,4>& operator*= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	left.x *= right.x;
	left.y *= right.y;
	left.z *= right.z;
	left.w *= right.w;
	return left;
}

template<typename T, uint32_t N>
constexpr Vector<T,N>& operator/= (Vector<T,N>& left, T right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] /= right;
	}
	return left;
}

template<typename T>
constexpr Vector<T,2>& operator/= (Vector<T,2>& left, T right) noexcept
{
	left.x /= right;
	left.y /= right;
	return left;
}

template<typename T>
constexpr Vector<T,3>& operator/= (Vector<T,3>& left, T right) noexcept
{
	left.x /= right;
	left.y /= right;
	left.z /= right;
	return left;
}

template<typename T>
constexpr Vector<T,4>& operator/= (Vector<T,4>& left, T right) noexcept
{
	left.x /= right;
	left.y /= right;
	left.z /= right;
	left.w /= right;
	return left;
}

template<typename T, uint32_t N>
constexpr Vector<T,N>& operator/= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] /= right.elements[i];
	}
	return left;
}

template<typename T>
constexpr Vector<T,2>& operator/= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x /= right.x;
	left.y /= right.y;
	return left;
}

template<typename T>
constexpr Vector<T,3>& operator/= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x /= right.x;
	left.y /= right.y;
	left.z /= right.z;
	return left;
}

template<typename T>
constexpr Vector<T,4>& operator/= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	left.x /= right.x;
	left.y /= right.y;
	left.z /= right.z;
	left.w /= right.w;
	return left;
}

// Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr Vector<T,N> operator+ (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp = left;
	return (temp += right);
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator- (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp = left;
	return (temp -= right);
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator- (const Vector<T,N>& vector) noexcept
{
	Vector<T,N> temp = vector;
	return (temp *= T(-1));
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator* (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T, N> temp = left;
	return (temp *= right);
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator* (const Vector<T,N>& left, T right) noexcept
{
	Vector<T,N> temp = left;
	return (temp *= right);
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator* (T left, const Vector<T,N>& right) noexcept
{
	return right * left;
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator/ (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp = left;
	return (temp /= right);
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator/ (const Vector<T,N>& left, T right) noexcept
{
	Vector<T,N> temp = left;
	return (temp /= right);
}

template<typename T, uint32_t N>
constexpr Vector<T,N> operator/ (T left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp(left);
	return (temp /= right);
}

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr bool operator== (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		if (left.elements[i] != right.elements[i]) return false;
	}
	return true;
}

template<typename T>
constexpr bool operator== (const Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	return left.x == right.x
	    && left.y == right.y;
}

template<typename T>
constexpr bool operator== (const Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	return left.x == right.x
	    && left.y == right.y
	    && left.z == right.z;
}

template<typename T>
constexpr bool operator== (const Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	return left.x == right.x
	    && left.y == right.y
	    && left.z == right.z
	    && left.w == right.w;
}

template<typename T, uint32_t N>
constexpr bool operator!= (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	return !(left == right);
}

} // namespace sfz

// Vector overloads of sfzMin() and sfzMax()
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMin(sfz::Vector<T,N> lhs, sfz::Vector<T,N> rhs) noexcept
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) {
		tmp[i] = sfzMin(lhs[i], rhs[i]);
	}
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMin(T lhs, sfz::Vector<T,N> rhs) noexcept
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) {
		tmp[i] = sfzMin(lhs, rhs[i]);
	}
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMin(sfz::Vector<T,N> lhs, T rhs) noexcept
{
	return sfzMin(rhs, lhs);
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMax(sfz::Vector<T,N> lhs, sfz::Vector<T,N> rhs) noexcept
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) {
		tmp[i] = sfzMax(lhs[i], rhs[i]);
	}
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMax(T lhs, sfz::Vector<T,N> rhs) noexcept
{
	sfz::Vector<T,N> tmp;
	for (uint32_t i = 0; i < N; i++) {
		tmp[i] = sfzMax(lhs, rhs[i]);
	}
	return tmp;
}

template<typename T, uint32_t N>
constexpr sfz::Vector<T,N> sfzMax(sfz::Vector<T,N> lhs, T rhs) noexcept
{
	return sfzMax(rhs, lhs);
}

// x86 SIMD implementations
#if !defined(__EMSCRIPTEN__) && !defined(SFZ_IOS)

template<>
inline sfz::vec4 sfzMin<float,4>(sfz::vec4 lhs, sfz::vec4 rhs) noexcept
{
	const __m128 lhsReg = _mm_load_ps(lhs.data());
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 minReg = _mm_min_ps(lhsReg, rhsReg);
	sfz::vec4 tmp;
	_mm_store_ps(tmp.data(), minReg);
	return tmp;
}

template<>
inline sfz::vec4_s32 sfzMin<int32_t,4>(sfz::vec4_s32 lhs, sfz::vec4_s32 rhs) noexcept
{
	const __m128i lhsReg = _mm_load_si128((const __m128i*)lhs.data());
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i minReg = _mm_min_epi32(lhsReg, rhsReg);
	sfz::vec4_s32 tmp;
	_mm_store_si128((__m128i*)tmp.data(), minReg);
	return tmp;
}

template<>
inline sfz::vec4 sfzMin<float,4>(float lhs, sfz::vec4 rhs) noexcept
{
	const __m128 lhsReg = _mm_set1_ps(lhs);
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 minReg = _mm_min_ps(lhsReg, rhsReg);
	sfz::vec4 tmp;
	_mm_store_ps(tmp.data(), minReg);
	return tmp;
}

template<>
inline sfz::vec4_s32 sfzMin<int32_t,4>(int32_t lhs, sfz::vec4_s32 rhs) noexcept
{
	const __m128i lhsReg = _mm_set1_epi32(lhs);
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i minReg = _mm_min_epi32(lhsReg, rhsReg);
	sfz::vec4_s32 tmp;
	_mm_store_si128((__m128i*)tmp.data(), minReg);
	return tmp;
}

template<>
inline sfz::vec4 sfzMax<float,4>(sfz::vec4 lhs, sfz::vec4 rhs) noexcept
{
	const __m128 lhsReg = _mm_load_ps(lhs.data());
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 minReg = _mm_max_ps(lhsReg, rhsReg);
	sfz::vec4 tmp;
	_mm_store_ps(tmp.data(), minReg);
	return tmp;
}

template<>
inline sfz::vec4_s32 sfzMax<int32_t,4>(sfz::vec4_s32 lhs, sfz::vec4_s32 rhs) noexcept
{
	const __m128i lhsReg = _mm_load_si128((const __m128i*)lhs.data());
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i maxReg = _mm_max_epi32(lhsReg, rhsReg);
	sfz::vec4_s32 tmp;
	_mm_store_si128((__m128i*)tmp.data(), maxReg);
	return tmp;
}

template<>
inline sfz::vec4 sfzMax<float,4>(float lhs, sfz::vec4 rhs) noexcept
{
	const __m128 lhsReg = _mm_set1_ps(lhs);
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 minReg = _mm_max_ps(lhsReg, rhsReg);
	sfz::vec4 tmp;
	_mm_store_ps(tmp.data(), minReg);
	return tmp;
}

template<>
inline sfz::vec4_s32 sfzMax<int32_t,4>(int32_t lhs, sfz::vec4_s32 rhs) noexcept
{
	const __m128i lhsReg = _mm_set1_epi32(lhs);
	const __m128i rhsReg = _mm_load_si128((const __m128i*)rhs.data());
	const __m128i maxReg = _mm_max_epi32(lhsReg, rhsReg);
	sfz::vec4_s32 tmp;
	_mm_store_si128((__m128i*)tmp.data(), maxReg);
	return tmp;
}

#endif
