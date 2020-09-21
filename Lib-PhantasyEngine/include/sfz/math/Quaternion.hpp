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

#include <skipifzero.hpp>
#include <skipifzero_math.hpp>

namespace sfz {

// Quaternion primitive
// ------------------------------------------------------------------------------------------------

template<typename T>
struct Quat final {

	// i*x + j*y + k*z + w
	// or
	// [v, w], v = [x, y, z] in the imaginary space, w is scalar real part
	// where
	// i² = j² = k² = -1
	// j*k = -k*j = i
	// k*i = -i*k = j
	// i*j = -j*i = k
	union {
		struct { T x, y, z, w; };
		struct { Vec<T,3> v; };
		Vec<T,4> vector;
	};

	constexpr Quat() noexcept = default;
	constexpr Quat(const Quat&) noexcept = default;
	constexpr Quat& operator= (const Quat&) noexcept = default;
	~Quat() noexcept = default;

	constexpr Quat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
	constexpr Quat(Vec<T,3> v, T w) : x(v.x), y(v.y), z(v.z), w(w) { }

	// Creates an identity quaternion representing a non-rotation, i.e. [0, 0, 0, 1]
	static constexpr Quat identity() { return Quat(T(0), T(0), T(0), T(1)); }

	// Creates a unit quaternion representing a (right-handed) rotation around the specified axis.
	// The given axis will be automatically normalized.
	static Quat rotationDeg(Vec<T,3> axis, T angleDeg) { return Quat::rotationRad(axis, angleDeg * DEG_TO_RAD); }
	static Quat rotationRad(Vec<T,3> axis, T angleRad)
	{
		const T halfAngleRad = angleRad * T(0.5);
		const Vec<T,3> normalizedAxis = normalize(axis);
		return Quat(sin(halfAngleRad) * normalizedAxis, cos(halfAngleRad));
	}

	// Constructs a Quaternion from Euler angles. The rotation around the z axis is performed first,
	// then y and last x axis.
	static Quat fromEuler(T xDeg, T yDeg, T zDeg)
	{
		const T DEG_ANGLE_TO_RAD_HALF_ANGLE = (T(3.14159265358979323846f) / T(180.0f)) / T(2.0f);

		T cosZ = cos(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T sinZ = sin(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T cosY = cos(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T sinY = sin(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T cosX = cos(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
		T sinX = sin(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);

		Quat tmp;
		tmp.x = cosZ * sinX * cosY - sinZ * cosX * sinY;
		tmp.y = cosZ * cosX * sinY + sinZ * sinX * cosY;
		tmp.z = sinZ * cosX * cosY - cosZ * sinX * sinY;
		tmp.w = cosZ * cosX * cosY + sinZ * sinX * sinY;
		return tmp;
	}
	static Quat fromEuler(Vec<T,3> anglesDeg) { return Quat::fromEuler(anglesDeg.x, anglesDeg.y, anglesDeg.z); }

	static Quat fromRotationMatrix(const Mat<T,3,3>& m)
	{
		// Algorithm from page 205 of Game Engine Architecture 2nd Edition
		const Vec<T,3>& e0 = m.row(0); const Vec<T,3>& e1 = m.row(1); const Vec<T,3>& e2 = m.row(2);
		T trace = e0[0] + e1[1] + e2[2];

		Quat tmp;

		// Check the diagonal
		if (trace > T(0)) {
			T s = std::sqrt(trace + T(1));
			tmp.w = s * T(0.5);

			T t = T(0.5) / s;
			tmp.x = (e2[1] - e1[2]) * t;
			tmp.y = (e0[2] - e2[0]) * t;
			tmp.z = (e1[0] - e0[1]) * t;
		}
		else {
			// Diagonal is negative
			int i = 0;
			if (e1[1] > e0[0]) i = 1;
			if (e2[2] > m.at(i, i)) i = 2;

			constexpr int NEXT[3] = { 1, 2, 0 };
			int j = NEXT[i];
			int k = NEXT[j];

			T s = sqrt((m.at(i, j) - (m.at(j, j) + m.at(k, k))) + T(1));
			tmp.vector[i] = s * T(0.5);

			T t = (s != T(0.0)) ? (T(0.5) / s) : s;

			tmp.vector[3] = (m.at(k, j) - m.at(j, k)) * t;
			tmp.vector[j] = (m.at(j, i) + m.at(i, j)) * t;
			tmp.vector[k] = (m.at(k, i) + m.at(i, k)) * t;
		}

		return tmp;
	}
	static Quat fromRotationMatrix(const Mat<T,3,4>& m) { return Quat::fromRotationMatrix(Mat<T,3,3>(m)); }

	// Returns the normalized axis which the quaternion rotates around, returns 0 vector for
	// identity Quaternion. Includes a safeNormalize() call, not necessarily super fast.
	Vec<T,3> rotationAxis() const { return normalizeSafe(this->v); }

	// Returns the angle (degrees) this Quaternion rotates around the rotationAxis().
	T rotationAngleDeg() const
	{
		const T RAD_ANGLE_TO_DEG_NON_HALF_ANGLE = (T(180) / T(3.14159265358979323846)) * T(2);
		T halfAngleRad = acos(this->w);
		return halfAngleRad * RAD_ANGLE_TO_DEG_NON_HALF_ANGLE;
	}

	// Returns a Euler angle (degrees) representation of this Quaternion. Assumes the Quaternion
	// is unit.
	Vec<T,3> toEuler() const
	{
		const T RAD_ANGLE_TO_DEG = T(180) / T(3.14159265358979323846);
		Vec<T,3> tmp;
		tmp.x = atan2(T(2) * (w * x + y * z), T(1) - T(2) * (x * x + y * y)) * RAD_ANGLE_TO_DEG;
		tmp.y = asin(sfz::min(sfz::max(2.0f * (w * y - z * x), -T(1)), T(1))) * RAD_ANGLE_TO_DEG;
		tmp.z = atan2(T(2) * (w * z + x * y), T(1) - T(2) * (y * y + z * z)) * RAD_ANGLE_TO_DEG;
		return tmp;
	}

	// Converts the given Quaternion into a Matrix. The normal variants assume that the Quaternion
	// is unit, while the "non-unit" variants make no such assumptions.
	constexpr Mat<T,3,3> toMat33() const
	{
		// Algorithm from Real-Time Rendering, page 76
		return Mat<T,3,3>(T(1) - T(2)*(y*y + z*z),  T(2)*(x*y - w*z),         T(2)*(x*z + w*y),
		                  T(2)*(x*y + w*z),         T(1) - T(2)*(x*x + z*z),  T(2)*(y*z - w*x),
		                  T(2)*(x*z - w*y),         T(2)*(y*z + w*x),         T(1) - T(2)*(x*x + y*y));
	}
	constexpr Mat<T,3,4> toMat34() const { return Mat<T,3,4>(toMat33()); }
	constexpr Mat<T,4,4> toMat44() const { return Mat<T,4,4>(toMat33()); }

	Mat<T,3,3> toMat33NonUnit() const
	{
		// Algorithm from Real-Time Rendering, page 76
		T s = T(2) / length(vector);
		return Mat<T,3,3>(T(1) - s*(y*y + z*z),  s*(x*y - w*z),         s*(x*z + w*y),
		                  s*(x*y + w*z),         T(1) - s*(x*x + z*z),  s*(y*z - w*x),
		                  s*(x*z - w*y),         s*(y*z + w*x),         T(1) - s*(x*x + y*y));
	}
	Mat<T,3,4> toMat34NonUnit() const { return Mat<T,3,4>(toMat33NonUnit()); }
	Mat<T,4,4> toMat44NonUnit() const { return Mat<T,4,4>(toMat33NonUnit()); }

	constexpr Quat& operator+= (Quat o) { this->vector += o.vector; return *this; }
	constexpr Quat& operator-= (Quat o) { this->vector -= o.vector; return *this; }
	constexpr Quat& operator*= (Quat o)
	{
		Quat tmp;
		tmp.v = cross(v, o.v) + o.w * v + w * o.v;
		tmp.w = w * o.w - dot(v, o.v);
		*this = tmp;
		return *this;
	}
	constexpr Quat& operator*= (T scalar) { vector *= scalar; return *this; }

	constexpr Quat operator+ (Quat o) const { return Quat(*this) += o; }
	constexpr Quat operator- (Quat o) const { return Quat(*this) -= o; }
	constexpr Quat operator* (Quat o) const { return Quat(*this) *= o; }
	constexpr Quat operator* (T scalar) const { return Quat(*this) *= scalar; }

	constexpr bool operator== (Quat o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	constexpr bool operator!= (Quat o) const { return !(*this == o); }
};

using quat = Quat<float>; static_assert(sizeof(quat) == sizeof(vec4), "");

template<typename T>
constexpr Quat<T> operator* (T scalar, Quat<T> q) { return q *= scalar; }

// Calculates the length (norm) of the Quaternion. A unit Quaternion has length 1. If the
// Quaternions are used for rotations they should always be unit.
template<typename T>
float length(Quat<T> q) { return length(q.vector); }

// Normalizes the Quaternion into a unit Quaternion by dividing each component by the length.
template<typename T>
Quat<T> normalize(Quat<T> q) { Quat<T> tmp; tmp.vector = normalize(q.vector); return tmp; }

// Calculates the conjugate quaternion, i.e. [-v, w]. If the quaternion is unit length this is
// the same as the inverse.
template<typename T>
constexpr Quat<T> conjugate(Quat<T> q) { return Quat<T>(-q.v, q.w); }

// Calculates the inverse for any Quaternion, i.e. (1 / length(q)²) * conjugate(q). For unit
// Quaternions (which should be the most common case) the conjugate() function should be used
// instead as it is way faster.
template<typename T>
constexpr Quat<T> inverse(Quat<T> q) { return (T(1) / dot(q.vector, q.vector)) * conjugate(q); }

// Rotates a vector with the specified Quaternion, using q * v * qInv. Either the inverse can
// be specified manually, or it can be calculated automatically from the given Quaternion. When
// it is calculated automatically it is assumed that the Quaternion is unit, so the inverse is
// the conjugate.
template<typename T>
constexpr Vec<T,3> rotate(Quat<T> q, Vec<T,3> v, Quat<T> qInv)
{
	Quat<T> tmp(v, T(0));
	tmp = q * tmp * qInv;
	return tmp.v;
}

template<typename T>
constexpr Vec<T,3> rotate(Quat<T> q, Vec<T,3> v) { return rotate(q, v, conjugate(q)); }

template<typename T>
Quat<T> lerp(Quat<T> q0, Quat<T> q1, T t)
{
	Quat<T> tmp;
	tmp.vector = lerp(q0.vector, q1.vector, t);
	tmp = normalize(tmp);
	return tmp;
}

} // namespace sfz
