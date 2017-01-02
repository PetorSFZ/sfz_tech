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

// Quaternion: Constructors & destructors
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL Quaternion::Quaternion(float x, float y, float z, float w) noexcept
:
	x(x),
	y(y),
	z(z),
	w(w)
{ }

SFZ_CUDA_CALL Quaternion::Quaternion(vec3 v, float w) noexcept
:
	x(v.x), y(v.y), z(v.z), w(w)
{ }

SFZ_CUDA_CALL Quaternion Quaternion::identity() noexcept
{
	return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}

SFZ_CUDA_CALL Quaternion Quaternion::rotation(vec3 axis, float angleDeg) noexcept
{
	using std::cos;
	using std::sin;
	const float DEG_ANGLE_TO_RAD_HALF_ANGLE = (3.14159265358979323846f / 180.0f) / 2.0f;

	const float halfAngleRad = angleDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE;
	const vec3 normalizedAxis = normalize(axis);
	
	return Quaternion(sin(halfAngleRad) * normalizedAxis, cos(halfAngleRad));
}

SFZ_CUDA_CALL Quaternion Quaternion::fromEuler(float xDeg, float yDeg, float zDeg) noexcept
{
	using std::cos;
	using std::sin;
	const float DEG_ANGLE_TO_RAD_HALF_ANGLE = (3.14159265358979323846f / 180.0f) / 2.0f;

	float cosZ = cos(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	float sinZ = sin(zDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	float cosY = cos(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	float sinY = sin(yDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	float cosX = cos(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);
	float sinX = sin(xDeg * DEG_ANGLE_TO_RAD_HALF_ANGLE);

	Quaternion tmp;
	tmp.x = cosZ * sinX * cosY - sinZ * cosX * sinY;
	tmp.y = cosZ * cosX * sinY + sinZ * sinX * cosY;
	tmp.z = sinZ * cosX * cosY - cosZ * sinX * sinY;
	tmp.w = cosZ * cosX * cosY + sinZ * sinX * sinY;
	return tmp;
}

SFZ_CUDA_CALL Quaternion Quaternion::fromEuler(vec3 anglesDeg) noexcept
{
	return Quaternion::fromEuler(anglesDeg.x, anglesDeg.y, anglesDeg.z);
}

// Quaternion: Methods
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL vec3 Quaternion::rotationAxis() const noexcept
{
	return safeNormalize(this->v);
}

SFZ_CUDA_CALL float Quaternion::rotationAngleDeg() const noexcept
{
	using std::acos;
	const float RAD_ANGLE_TO_DEG_NON_HALF_ANGLE = (180.0f / 3.14159265358979323846f) * 2.0f;
	
	float halfAngleRad = acos(this->w);
	return halfAngleRad * RAD_ANGLE_TO_DEG_NON_HALF_ANGLE;
}

SFZ_CUDA_CALL vec3 Quaternion::toEuler() const noexcept
{
	using std::atan2;
	using std::asin;
	using std::max;
	using std::min;
	const float RAD_ANGLE_TO_DEG = 180.0f / 3.14159265358979323846f;

	vec3 tmp;
	tmp.x = atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y)) * RAD_ANGLE_TO_DEG;
	tmp.y = asin(min(max(2.0f * (w * y - z * x), -1.0f), 1.0f)) * RAD_ANGLE_TO_DEG;
	tmp.z = atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z)) * RAD_ANGLE_TO_DEG;
	return tmp;
}

// Quaternion: Matrix conversion methods
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL mat33 Quaternion::toMat33() const noexcept
{
	// Algorithm from Real-Time Rendering, page 76
	return mat33(1.0f - 2.0f*(y*y + z*z),  2.0f*(x*y - w*z),         2.0f*(x*z + w*y),
	             2.0f*(x*y + w*z),         1.0f - 2.0f*(x*x + z*z),  2.0f*(y*z - w*x),
	             2.0f*(x*z - w*y),         2.0f*(y*z + w*x),         1.0f - 2.0f*(x*x + y*y));
}

SFZ_CUDA_CALL mat34 Quaternion::toMat34() const noexcept
{
	// Algorithm from Real-Time Rendering, page 76
	return mat34(1.0f - 2.0f*(y*y + z*z),  2.0f*(x*y - w*z),         2.0f*(x*z + w*y),         0.0f,
	             2.0f*(x*y + w*z),         1.0f - 2.0f*(x*x + z*z),  2.0f*(y*z - w*x),         0.0f,
	             2.0f*(x*z - w*y),         2.0f*(y*z + w*x),         1.0f - 2.0f*(x*x + y*y),  0.0f);
}

SFZ_CUDA_CALL mat44 Quaternion::toMat44() const noexcept
{
	// Algorithm from Real-Time Rendering, page 76
	return mat44(1.0f - 2.0f*(y*y + z*z),  2.0f*(x*y - w*z),         2.0f*(x*z + w*y),         0.0f,
	             2.0f*(x*y + w*z),         1.0f - 2.0f*(x*x + z*z),  2.0f*(y*z - w*x),         0.0f,
	             2.0f*(x*z - w*y),         2.0f*(y*z + w*x),         1.0f - 2.0f*(x*x + y*y),  0.0f,
	             0.0f,                     0.0f,                     0.0f,                     1.0f);
}

SFZ_CUDA_CALL mat33 Quaternion::toMat33NonUnit() const noexcept
{
	// Algorithm from Real-Time Rendering, page 76
	float s = 2.0f / length(*this);
	return mat33(1.0f - s*(y*y + z*z),  s*(x*y - w*z),         s*(x*z + w*y),
	             s*(x*y + w*z),         1.0f - s*(x*x + z*z),  s*(y*z - w*x),
	             s*(x*z - w*y),         s*(y*z + w*x),         1.0f - s*(x*x + y*y));
}

SFZ_CUDA_CALL mat34 Quaternion::toMat34NonUnit() const noexcept
{
	// Algorithm from Real-Time Rendering, page 76
	float s = 2.0f / length(*this);
	return mat34(1.0f - s*(y*y + z*z),  s*(x*y - w*z),         s*(x*z + w*y),         0.0f,
	             s*(x*y + w*z),         1.0f - s*(x*x + z*z),  s*(y*z - w*x),         0.0f,
	             s*(x*z - w*y),         s*(y*z + w*x),         1.0f - s*(x*x + y*y),  0.0f);
}

SFZ_CUDA_CALL mat44 Quaternion::toMat44NonUnit() const noexcept
{
	// Algorithm from Real-Time Rendering, page 76
	float s = 2.0f / length(*this);
	return mat44(1.0f - s*(y*y + z*z),  s*(x*y - w*z),         s*(x*z + w*y),         0.0f,
	             s*(x*y + w*z),         1.0f - s*(x*x + z*z),  s*(y*z - w*x),         0.0f,
	             s*(x*z - w*y),         s*(y*z + w*x),         1.0f - s*(x*x + y*y),  0.0f,
	             0.0f,                  0.0f,                  0.0f,                  1.0f);
}

// Quaternion functions
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL float length(const Quaternion& q) noexcept
{
	return length(q.vector);
}

SFZ_CUDA_CALL Quaternion normalize(const Quaternion& q) noexcept
{
	Quaternion tmp;
	tmp.vector = normalize(q.vector);
	return tmp;
}

SFZ_CUDA_CALL Quaternion conjugate(const Quaternion& q) noexcept
{
	return Quaternion(-q.v, q.w);
}

SFZ_CUDA_CALL Quaternion inverse(const Quaternion& q) noexcept
{
	return (1.0f / dot(q.vector, q.vector)) * conjugate(q);
}

SFZ_CUDA_CALL vec3 rotate(Quaternion q, vec3 v) noexcept
{
	return rotate(q, v, conjugate(q));
}

SFZ_CUDA_CALL vec3 rotate(Quaternion q, vec3 v, Quaternion qInv) noexcept
{
	Quaternion tmp;
	tmp.v = v;
	tmp.w = 0.0f;
	tmp = q * tmp * qInv;
	return tmp.v;
}

// Quaternion:Operators (comparison)
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL bool operator== (const Quaternion& lhs, const Quaternion& rhs) noexcept
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

SFZ_CUDA_CALL bool operator!= (const Quaternion& lhs, const Quaternion& rhs) noexcept
{
	return !(lhs == rhs);
}

// Quaternion:Operators (arithmetic & assignment)
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL Quaternion& operator+= (Quaternion& left, const Quaternion& right) noexcept
{
	left.vector += right.vector;
	return left;
}

SFZ_CUDA_CALL Quaternion& operator-= (Quaternion& left, const Quaternion& right) noexcept
{
	left.vector -= right.vector;
	return left;
}

SFZ_CUDA_CALL Quaternion& operator*= (Quaternion& left, const Quaternion& right) noexcept
{
	Quaternion tmp;
	tmp.v = cross(left.v, right.v) + right.w * left.v + left.w * right.v;
	tmp.w = left.w * right.w - dot(left.v, right.v);
	left = tmp;
	return left;
}

SFZ_CUDA_CALL Quaternion& operator*= (Quaternion& q, float scalar) noexcept
{
	q.vector *= scalar;
	return q;
}

// Quaternion:Operators (arithmetic)
// ------------------------------------------------------------------------------------------------

SFZ_CUDA_CALL Quaternion operator+ (const Quaternion& left, const Quaternion& right) noexcept
{
	Quaternion tmp = left;
	return tmp += right;
}

SFZ_CUDA_CALL Quaternion operator- (const Quaternion& left, const Quaternion& right) noexcept
{
	Quaternion tmp = left;
	return tmp -= right;
}

SFZ_CUDA_CALL Quaternion operator* (const Quaternion& left, const Quaternion& right) noexcept
{
	Quaternion tmp = left;
	return tmp *= right;
}

SFZ_CUDA_CALL Quaternion operator* (const Quaternion& q, float scalar) noexcept
{
	Quaternion tmp = q;
	return tmp *= scalar;
}

SFZ_CUDA_CALL Quaternion operator* (float scalar, const Quaternion& q) noexcept
{
	Quaternion tmp = q;
	return tmp *= scalar;
}

} // namespace sfz
