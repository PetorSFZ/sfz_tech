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

// Common specialized operations
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,2,2> inverse(const Matrix<T,2,2>& m) noexcept
{
	const T det = determinant(m);
	if (det == 0) return Matrix<T,2,2>(T(0), T(0), T(0), T(0));

	Matrix<T,2,2> tmp(m.at(1, 1), -m.at(0, 1),
	                  -m.at(1,0), m.at(0,0));

	return (T(1) / det) * tmp;
}

template<typename T>
Matrix<T,3,3> inverse(const Matrix<T,3,3>& m) noexcept
{
	const T det = determinant(m);
	if (det == 0) return Matrix<T,3,3>(T(0), T(0), T(0),
	                                   T(0), T(0), T(0),
	                                   T(0), T(0), T(0));

	const T a = m.at(0,0);
	const T b = m.at(0,1); 
	const T c = m.at(0,2);
	const T d = m.at(1,0);
	const T e = m.at(1,1); 
	const T f = m.at(1,2);
	const T g = m.at(2,0);
	const T h = m.at(2,1); 
	const T i = m.at(2,2);

	
	const T A = (e*i - f*h);
	const T B = -(d*i - f*g);
	const T C = (d*h - e*g);
	const T D = -(b*i - c*h);
	const T E = (a*i - c*g);
	const T F = -(a*h - b*g);
	const T G = (b*f - c*e);
	const T H = -(a*f - c*d);
	const T I = (a*e - b*d);

	Matrix<T,3,3> tmp(A, D, G,
	                  B, E, H,
	                  C, F, I);
	return (T(1) / det) * tmp;
}

template<typename T>
Matrix<T,4,4> inverse(const Matrix<T,4,4>& m) noexcept
{
	const T det = determinant(m);
	if (det == 0) return Matrix<T,4,4>(T(0), T(0), T(0), T(0),
	                                   T(0), T(0), T(0), T(0),
	                                   T(0), T(0), T(0), T(0),
	                                   T(0), T(0), T(0), T(0));

	const T m00 = m.at(0, 0), m01 = m.at(0, 1), m02 = m.at(0, 2), m03 = m.at(0, 3),
	        m10 = m.at(1, 0), m11 = m.at(1, 1), m12 = m.at(1, 2), m13 = m.at(1, 3),
	        m20 = m.at(2, 0), m21 = m.at(2, 1), m22 = m.at(2, 2), m23 = m.at(2, 3),
	        m30 = m.at(3, 0), m31 = m.at(3, 1), m32 = m.at(3, 2), m33 = m.at(3, 3);

	const T b00 = m11*m22*m33 + m12*m23*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 - m13*m22*m31;
	const T b01 = m01*m23*m32 + m02*m21*m33 + m03*m22*m31 - m01*m22*m33 - m02*m23*m31 - m03*m21*m32;
	const T b02 = m01*m12*m33 + m02*m13*m31 + m03*m11*m32 - m01*m13*m32 - m02*m11*m33 - m03*m12*m31;
	const T b03 = m01*m13*m22 + m02*m11*m23 + m03*m12*m21 - m01*m12*m23 - m02*m13*m21 - m03*m11*m22;
	const T b10 = m10*m23*m32 + m12*m20*m33 + m13*m22*m30 - m10*m22*m33 - m12*m23*m30 - m13*m20*m32;
	const T b11 = m00*m22*m33 + m02*m23*m30 + m03*m20*m32 - m00*m23*m32 - m02*m20*m33 - m03*m22*m30;
	const T b12 = m00*m13*m32 + m02*m10*m33 + m03*m12*m30 - m00*m12*m33 - m02*m13*m30 - m03*m10*m32;
	const T b13 = m00*m12*m23 + m02*m13*m20 + m03*m10*m22 - m00*m13*m22 - m02*m10*m23 - m03*m12*m20;
	const T b20 = m10*m21*m33 + m11*m23*m30 + m13*m20*m31 - m01*m23*m31 - m11*m20*m33 - m13*m21*m30;
	const T b21 = m00*m23*m31 + m01*m20*m33 + m03*m21*m30 - m00*m21*m33 - m01*m23*m30 - m03*m20*m31;
	const T b22 = m00*m11*m33 + m01*m13*m30 + m03*m10*m31 - m00*m13*m31 - m01*m10*m33 - m03*m11*m30;
	const T b23 = m00*m13*m21 + m01*m10*m23 + m03*m11*m20 - m00*m11*m23 - m01*m13*m20 - m03*m10*m21;
	const T b30 = m10*m22*m31 + m11*m20*m32 + m12*m21*m30 - m10*m21*m32 - m11*m22*m30 - m12*m20*m31;
	const T b31 = m00*m21*m32 + m01*m22*m30 + m02*m20*m31 - m00*m22*m31 - m01*m20*m32 - m02*m21*m30;
	const T b32 = m00*m12*m31 + m01*m10*m32 + m02*m11*m30 - m00*m11*m32 - m01*m12*m30 - m02*m10*m31;
	const T b33 = m00*m11*m22 + m01*m12*m20 + m02*m10*m21 - m00*m12*m21 - m01*m10*m22 - m02*m11*m20;

	Matrix<T,4,4> tmp(b00, b01, b02, b03,
	                  b10, b11, b12, b13,
	                  b20, b21, b22, b23,
	                  b30, b31, b32, b33);
	return (T(1) / det) * tmp;
}

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
