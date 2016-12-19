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
SFZ_CUDA_CALL Vector<T,N>::Vector(const T* arrayPtr) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		elements[i] = arrayPtr[i];
	}
}

template<typename T, uint32_t N>
template<typename T2>
SFZ_CUDA_CALL Vector<T,N>::Vector(const Vector<T2,N>& other) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		elements[i] = static_cast<T>(other[i]);
	}
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL T& Vector<T,N>::operator[] (uint32_t index) noexcept
{
	return elements[index];
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL T Vector<T,N>::operator[] (uint32_t index) const noexcept
{
	return elements[index];
}

// Vector struct declaration: Vector<T,2>
// ------------------------------------------------------------------------------------------------

template<typename T>
SFZ_CUDA_CALL Vector<T,2>::Vector(const T* arrayPtr) noexcept
:
	x(arrayPtr[0]),
	y(arrayPtr[1])
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,2>::Vector(T value) noexcept
:
	x(value),
	y(value)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,2>::Vector(T x, T y) noexcept
:
	x(x),
	y(y)
{ }

template<typename T>
template<typename T2>
SFZ_CUDA_CALL Vector<T,2>::Vector(const Vector<T2,2>& other) noexcept
:
	x(static_cast<T>(other.x)),
	y(static_cast<T>(other.y))
{ }

template<typename T>
SFZ_CUDA_CALL T& Vector<T,2>::operator[] (uint32_t index) noexcept
{
	return data()[index];
}

template<typename T>
SFZ_CUDA_CALL T Vector<T,2>::operator[] (uint32_t index) const noexcept
{
	return data()[index];
}

// Vector struct declaration: Vector<T,3>
// ------------------------------------------------------------------------------------------------

template<typename T>
SFZ_CUDA_CALL Vector<T,3>::Vector(const T* arrayPtr) noexcept
:
	x(arrayPtr[0]),
	y(arrayPtr[1]),
	z(arrayPtr[2])
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,3>::Vector(T value) noexcept
:
	x(value),
	y(value),
	z(value)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,3>::Vector(T x, T y, T z) noexcept
:
	x(x),
	y(y),
	z(z)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,3>::Vector(Vector<T,2> xy, T z) noexcept
:
	x(xy.x),
	y(xy.y),
	z(z)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,3>::Vector(T x, Vector<T,2> yz) noexcept
:
	x(x),
	y(yz.x),
	z(yz.y)
{ }

template<typename T>
template<typename T2>
SFZ_CUDA_CALL Vector<T,3>::Vector(const Vector<T2,3>& other) noexcept
:
	x(static_cast<T>(other.x)),
	y(static_cast<T>(other.y)),
	z(static_cast<T>(other.z))
{ }

template<typename T>
SFZ_CUDA_CALL T& Vector<T,3>::operator[] (uint32_t index) noexcept
{
	return data()[index];
}

template<typename T>
SFZ_CUDA_CALL T Vector<T,3>::operator[] (uint32_t index) const noexcept
{
	return data()[index];
}

// Vector struct declaration: Vector<T,4>
// ------------------------------------------------------------------------------------------------

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(const T* arrayPtr) noexcept
:
	x(arrayPtr[0]),
	y(arrayPtr[1]),
	z(arrayPtr[2]),
	w(arrayPtr[3])
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(T value) noexcept
:
	x(value),
	y(value),
	z(value),
	w(value)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(T x, T y, T z, T w) noexcept
:
	x(x),
	y(y),
	z(z),
	w(w)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(Vector<T,3> xyz, T w) noexcept
:
	x(xyz.x),
	y(xyz.y),
	z(xyz.z),
	w(w)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(T x, Vector<T,3> yzw) noexcept
:
	x(x),
	y(yzw.x),
	z(yzw.y),
	w(yzw.z)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(Vector<T,2> xy, Vector<T,2> zw) noexcept
:
	x(xy.x),
	y(xy.y),
	z(zw.x),
	w(zw.y)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(Vector<T,2> xy, T z, T w) noexcept
:
	x(xy.x),
	y(xy.y),
	z(z),
	w(w)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(T x, Vector<T,2> yz, T w) noexcept
:
	x(x),
	y(yz.x),
	z(yz.y),
	w(w)
{ }

template<typename T>
SFZ_CUDA_CALL Vector<T,4>::Vector(T x, T y, Vector<T,2> zw) noexcept
:
	x(x),
	y(y),
	z(zw.x),
	w(zw.y)
{ }

template<typename T>
template<typename T2>
SFZ_CUDA_CALL Vector<T,4>::Vector(const Vector<T2,4>& other) noexcept
:
	x(static_cast<T>(other.x)),
	y(static_cast<T>(other.y)),
	z(static_cast<T>(other.z)),
	w(static_cast<T>(other.w))
{ }

template<typename T>
SFZ_CUDA_CALL T& Vector<T,4>::operator[] (uint32_t index) noexcept
{
	return data()[index];
}

template<typename T>
SFZ_CUDA_CALL T Vector<T,4>::operator[] (uint32_t index) const noexcept
{
	return data()[index];
}

// Vector functions: dot()
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
SFZ_CUDA_CALL T dot(Vector<T,N> lhs, Vector<T,N> rhs) noexcept
{
	T product = T(0);
	for (uint32_t i = 0; i < N; i++) {
		product += lhs[i] * rhs[i];
	}
	return product;
}

template<typename T>
SFZ_CUDA_CALL T dot(Vector<T,2> lhs, Vector<T,2> rhs) noexcept
{
	return lhs.x * rhs.x
	     + lhs.y * rhs.y;
}

template<typename T>
SFZ_CUDA_CALL T dot(Vector<T,3> lhs, Vector<T,3> rhs) noexcept
{
	return lhs.x * rhs.x
	     + lhs.y * rhs.y
	     + lhs.z * rhs.z;
}

template<typename T>
SFZ_CUDA_CALL T dot(Vector<T,4> lhs, Vector<T,4> rhs) noexcept
{
	return lhs.x * rhs.x
	     + lhs.y * rhs.y
	     + lhs.z * rhs.z
	     + lhs.w * rhs.w;
}

template<>
SFZ_CUDA_CALL float dot(vec2 lhs, vec2 rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return fmaf(lhs.x, rhs.x,
	            lhs.y * rhs.y);
#else
	return dot<float,2>(lhs, rhs);
#endif
}

template<>
SFZ_CUDA_CALL float dot(vec3 lhs, vec3 rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return fmaf(lhs.x, rhs.x,
	       fmaf(lhs.y, rhs.y,
	            lhs.z * rhs.z));
#else
	vec4 tmpLhs;
	tmpLhs.xyz = lhs;
	vec4 tmpRhs;
	tmpRhs.xyz = rhs;

	const __m128 lhsReg = _mm_load_ps(tmpLhs.data());
	const __m128 rhsReg = _mm_load_ps(tmpRhs.data());
	const __m128 dotProd = _mm_dp_ps(lhsReg, rhsReg, 0x71); // 0111 0001 (3 elements, store in lowest)
	return _mm_cvtss_f32(dotProd);
#endif
}

template<>
SFZ_CUDA_CALL float dot(vec4 lhs, vec4 rhs) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return fmaf(lhs.x, rhs.x,
	       fmaf(lhs.y, rhs.y,
	       fmaf(lhs.z, rhs.z,
	            lhs.w * rhs.w)));
#else
	const __m128 lhsReg = _mm_load_ps(lhs.data());
	const __m128 rhsReg = _mm_load_ps(rhs.data());
	const __m128 dotProd = _mm_dp_ps(lhsReg, rhsReg, 0xF1); // 1111 0001 (4 elements, store in lowest)
	return _mm_cvtss_f32(dotProd);
#endif
}

// Vector functions: length()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float length(vec2 v) noexcept
{
	using std::sqrt;
	return sqrt(dot(v, v));
}

SFZ_CUDA_CALL float length(vec3 v) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return sqrtf(dot(v, v));
#else
	vec4 tmp;
	tmp.xyz = v;
	const __m128 reg = _mm_load_ps(tmp.data());
	const __m128 dotProd = _mm_dp_ps(reg, reg, 0x71); // 0111 0001 (3 elements, store in lowest)
	const __m128 len = _mm_sqrt_ss(dotProd); // sqrt() of lowest
	return _mm_cvtss_f32(len);
#endif
}

SFZ_CUDA_CALL float length(vec4 v) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	return sqrtf(dot(v, v));
#else
	const __m128 reg = _mm_load_ps(v.data());
	const __m128 dotProd = _mm_dp_ps(reg, reg, 0xF1); // 1111 0001 (4 elements, store in lowest)
	const __m128 len = _mm_sqrt_ss(dotProd); // sqrt() of lowest
	return _mm_cvtss_f32(len);
#endif
}

// Vector functions: normalize()
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec2 normalize(vec2 v) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	float inverseSqrt = rsqrtf(sfz::dot(v, v));
	return inverseSqrt * v;
#else
	return v / sfz::length(v);
#endif
}

SFZ_CUDA_CALL vec3 normalize(vec3 v) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	float inverseSqrt = rsqrtf(sfz::dot(v, v));
	return inverseSqrt * v;
#else
	vec4 tmp;
	tmp.xyz = v;
	const __m128 reg = _mm_load_ps(tmp.data());
	const __m128 dotProd = _mm_dp_ps(reg, reg, 0x77); // 0111 0111 (3 elements, store in 3 lowest)
	const __m128 inverseSqrt = _mm_rsqrt_ps(dotProd);
	const __m128 res = _mm_mul_ps(reg, inverseSqrt);
	_mm_store_ps(tmp.data(), res);
	return tmp.xyz;
#endif
}

SFZ_CUDA_CALL vec4 normalize(vec4 v) noexcept
{
#ifdef SFZ_CUDA_DEVICE_CODE
	float inverseSqrt = rsqrtf(sfz::dot(v, v));
	return inverseSqrt * v;
#else
	const __m128 reg = _mm_load_ps(v.data());
	const __m128 dotProd = _mm_dp_ps(reg, reg, 0xFF); // 1111 1111 (4 elements, store in all)
	const __m128 inverseSqrt = _mm_rsqrt_ps(dotProd);
	const __m128 res = _mm_mul_ps(reg, inverseSqrt);
	_mm_store_ps(v.data(), res);
	return v;
#endif
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
SFZ_CUDA_CALL Vector<T,3> cross(Vector<T,3> lhs, Vector<T,3> rhs) noexcept
{
	return Vector<T,3>(lhs.y * rhs.z - lhs.z * rhs.y,
	                   lhs.z * rhs.x - lhs.x * rhs.z,
	                   lhs.x * rhs.y - lhs.y * rhs.x);
}

// Vector functions: elementSum()
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
SFZ_CUDA_CALL T elementSum(Vector<T,N> v) noexcept
{
	T result = T(0);
	for (uint32_t i = 0; i < N; ++i) {
		result += v[i];
	}
	return result;
}

template<typename T>
SFZ_CUDA_CALL T elementSum(Vector<T,2> v) noexcept
{
	return v.x + v.y;
}

template<typename T>
SFZ_CUDA_CALL T elementSum(Vector<T,3> v) noexcept
{
	return v.x + v.y + v.z;
}

template<typename T>
SFZ_CUDA_CALL T elementSum(Vector<T,4> v) noexcept
{
	return v.x + v.y + v.z + v.w;
}

// Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator+= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] += right.elements[i];
	}
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,2>& operator+= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x += right.x;
	left.y += right.y;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3>& operator+= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,4>& operator+= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;
	left.w += right.w;
	return left;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator-= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] -= right.elements[i];
	}
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,2>& operator-= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3>& operator-= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,4>& operator-= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;
	left.w -= right.w;
	return left;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator*= (Vector<T,N>& left, T right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] *= right;
	}
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,2>& operator*= (Vector<T,2>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3>& operator*= (Vector<T,3>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;
	left.z *= right;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,4>& operator*= (Vector<T,4>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;
	left.z *= right;
	left.w *= right;
	return left;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator*= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] *= right.elements[i];
	}
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,2>& operator*= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x *= right.x;
	left.y *= right.y;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3>& operator*= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x *= right.x;
	left.y *= right.y;
	left.z *= right.z;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,4>& operator*= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	left.x *= right.x;
	left.y *= right.y;
	left.z *= right.z;
	left.w *= right.w;
	return left;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator/= (Vector<T,N>& left, T right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] /= right;
	}
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,2>& operator/= (Vector<T,2>& left, T right) noexcept
{
	left.x /= right;
	left.y /= right;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3>& operator/= (Vector<T,3>& left, T right) noexcept
{
	left.x /= right;
	left.y /= right;
	left.z /= right;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,4>& operator/= (Vector<T,4>& left, T right) noexcept
{
	left.x /= right;
	left.y /= right;
	left.z /= right;
	left.w /= right;
	return left;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N>& operator/= (Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		left.elements[i] /= right.elements[i];
	}
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,2>& operator/= (Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	left.x /= right.x;
	left.y /= right.y;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,3>& operator/= (Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	left.x /= right.x;
	left.y /= right.y;
	left.z /= right.z;
	return left;
}

template<typename T>
SFZ_CUDA_CALL Vector<T,4>& operator/= (Vector<T,4>& left, const Vector<T,4>& right) noexcept
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
SFZ_CUDA_CALL Vector<T,N> operator+ (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp = left;
	return (temp += right);
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator- (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp = left;
	return (temp -= right);
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator- (const Vector<T,N>& vector) noexcept
{
	Vector<T,N> temp = vector;
	return (temp *= T(-1));
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator* (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T, N> temp = left;
	return (temp *= right);
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator* (const Vector<T,N>& left, T right) noexcept
{
	Vector<T,N> temp = left;
	return (temp *= right);
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator* (T left, const Vector<T,N>& right) noexcept
{
	return right * left;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator/ (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp = left;
	return (temp /= right);
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator/ (const Vector<T,N>& left, T right) noexcept
{
	Vector<T,N> temp = left;
	return (temp /= right);
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL Vector<T,N> operator/ (T left, const Vector<T,N>& right) noexcept
{
	Vector<T,N> temp(left);
	return (temp /= right);
}

// Operators (comparison)
// ------------------------------------------------------------------------------------------------

template<typename T, uint32_t N>
SFZ_CUDA_CALL bool operator== (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	for (uint32_t i = 0; i < N; ++i) {
		if (left.elements[i] != right.elements[i]) return false;
	}
	return true;
}

template<typename T>
SFZ_CUDA_CALL bool operator== (const Vector<T,2>& left, const Vector<T,2>& right) noexcept
{
	return left.x == right.x
	    && left.y == right.y;
}

template<typename T>
SFZ_CUDA_CALL bool operator== (const Vector<T,3>& left, const Vector<T,3>& right) noexcept
{
	return left.x == right.x
	    && left.y == right.y
	    && left.z == right.z;
}

template<typename T>
SFZ_CUDA_CALL bool operator== (const Vector<T,4>& left, const Vector<T,4>& right) noexcept
{
	return left.x == right.x
	    && left.y == right.y
	    && left.z == right.z
	    && left.w == right.w;
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL bool operator!= (const Vector<T,N>& left, const Vector<T,N>& right) noexcept
{
	return !(left == right);
}

} // namespace sfz
