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

// Rotation matrices
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,3,3> xRotationMatrix3(T angleRads) noexcept
{
	T cosA = std::cos(angleRads);
	T sinA = std::sin(angleRads);
	return Matrix<T,3,3>{{1, 0, 0},
	                     {0, cosA, -sinA},
	                     {0, sinA, cosA}};
}

template<typename T>
Matrix<T,4,4> xRotationMatrix4(T angleRads) noexcept
{
	T cosA = std::cos(angleRads);
	T sinA = std::sin(angleRads);
	return Matrix<T,4,4>{{1, 0, 0, 0},
	                     {0, cosA, -sinA, 0},
	                     {0, sinA, cosA, 0},
	                     {0, 0, 0, 1}};
}

template<typename T>
Matrix<T,3,3> yRotationMatrix3(T angleRads) noexcept
{
	T cosA = std::cos(angleRads);
	T sinA = std::sin(angleRads);
	return Matrix<T,3,3>{{cosA, 0, sinA},
	                     {0, 1, 0},
	                     {-sinA, 0, cosA}};
}

template<typename T>
Matrix<T,4,4> yRotationMatrix4(T angleRads) noexcept
{
	T cosA = std::cos(angleRads);
	T sinA = std::sin(angleRads);
	return Matrix<T,4,4>{{cosA, 0, sinA, 0},
	                     {0, 1, 0, 0},
	                     {-sinA, 0, cosA, 0},
	                     {0, 0, 0, 1}};
}

template<typename T>
Matrix<T,3,3> zRotationMatrix3(T angleRads) noexcept
{
	T cosA = std::cos(angleRads);
	T sinA = std::sin(angleRads);
	return Matrix<T,3,3>{{cosA, -sinA, 0},
	                     {sinA, cosA, 0},
	                     {0, 0, 1}};
}

template<typename T>
Matrix<T,4,4> zRotationMatrix4(T angleRads) noexcept
{
	T cosA = std::cos(angleRads);
	T sinA = std::sin(angleRads);
	return Matrix<T,4,4>{{cosA, -sinA, 0, 0},
	                     {sinA, cosA, 0, 0},
	                     {0, 0, 1, 0},
	                     {0, 0, 0, 1}};
}

template<typename T>
Matrix<T,3,3> rotationMatrix3(const sfz::Vector<T,3>& axis, T angleRads) noexcept
{
	using std::cos;
	using std::sin;
	sfz::Vector<T,3> r = normalize(axis);
	T x = r[0];
	T y = r[1];
	T z = r[2];
	T c = cos(angleRads);
	T s = sin(angleRads);
	T cm1 = static_cast<T>(1) - c;
	// Matrix by Goldman, page 71 of Real-Time Rendering.
	return Matrix<T,3,3>{{c + cm1*x*x, cm1*x*y - z*s, cm1*x*z + y*s},
	                     {cm1*x*y + z*s, c + cm1*y*y, cm1*y*z - x*s},
	                     {cm1*x*z - y*s, cm1*y*z + x*s, c + cm1*z*z}};
}

template<typename T>
Matrix<T,4,4> rotationMatrix4(const sfz::Vector<T,3>& axis, T angleRads) noexcept
{
	using std::cos;
	using std::sin;
	sfz::Vector<T,3> r = normalize(axis);
	T x = r[0];
	T y = r[1];
	T z = r[2];
	T c = cos(angleRads);
	T s = sin(angleRads);
	T cm1 = static_cast<T>(1) - c;
	// Matrix by Goldman, page 71 of Real-Time Rendering.
	return Matrix<T,4,4>{{c + cm1*x*x, cm1*x*y - z*s, cm1*x*z + y*s, 0},
	                     {cm1*x*y + z*s, c + cm1*y*y, cm1*y*z - x*s, 0},
	                     {cm1*x*z - y*s, cm1*y*z + x*s, c + cm1*z*z, 0},
	                     {0, 0, 0, 1}};
}

// Transformation matrices
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,4,4> translationMatrix(T deltaX, T deltaY, T deltaZ) noexcept
{
	return Matrix<T,4,4>{{1, 0, 0, deltaX},
	                     {0, 1, 0, deltaY},
	                     {0, 0, 1, deltaZ},
	                     {0, 0, 0, 1}};
}

template<typename T>
Matrix<T,4,4> translationMatrix(const Vector<T,3>& delta) noexcept
{
	return translationMatrix(delta[0], delta[1], delta[2]);
}

} // namespace sfz
