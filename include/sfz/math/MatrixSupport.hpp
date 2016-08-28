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

#include <cmath>

#include "sfz/Assert.hpp"
#include "sfz/math/Matrix.hpp"

namespace sfz {

// Resizing helpers
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,3,3> toMat3(const Matrix<T,4,4>& m) noexcept;

template<typename T>
Matrix<T,4,4> toMat4(const Matrix<T,3,3>& m) noexcept;

// Transforming 3D vector helpers
// ------------------------------------------------------------------------------------------------

template<typename T>
Vector<T,3> transformPoint(const Matrix<T,4,4>& m, const Vector<T,3>& p) noexcept;

template<typename T>
Vector<T,3> transformDir(const Matrix<T,4,4>& m, const Vector<T,3>& d) noexcept;

// Common specialized operations
// ------------------------------------------------------------------------------------------------

template<typename T>
T determinant(const Matrix<T,2,2>& m) noexcept;

template<typename T>
T determinant(const Matrix<T,3,3>& m) noexcept;

template<typename T>
T determinant(const Matrix<T,4,4>& m) noexcept;

template<typename T>
Matrix<T,2,2> inverse(const Matrix<T,2,2>& m) noexcept;

template<typename T>
Matrix<T,3,3> inverse(const Matrix<T,3,3>& m) noexcept;

template<typename T>
Matrix<T,4,4> inverse(const Matrix<T,4,4>& m) noexcept;

// Rotation matrices
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,3,3> xRotationMatrix3(T angleRads) noexcept;

template<typename T>
Matrix<T,4,4> xRotationMatrix4(T angleRads) noexcept;

template<typename T>
Matrix<T,3,3> yRotationMatrix3(T angleRads) noexcept;

template<typename T>
Matrix<T,4,4> yRotationMatrix4(T angleRads) noexcept;

template<typename T>
Matrix<T,3,3> zRotationMatrix3(T angleRads) noexcept;

template<typename T>
Matrix<T,4,4> zRotationMatrix4(T angleRads) noexcept;

template<typename T>
Matrix<T,3,3> rotationMatrix3(const sfz::Vector<T,3>& axis, T angleRads) noexcept;

template<typename T>
Matrix<T,4,4> rotationMatrix4(const sfz::Vector<T,3>& axis, T angleRads) noexcept;

// Transformation matrices
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,3,3> identityMatrix3() noexcept;

template<typename T>
Matrix<T,4,4> identityMatrix4() noexcept;

template<typename T>
Matrix<T,3,3> scalingMatrix3(T scaleFactor) noexcept;

template<typename T>
Matrix<T,4,4> scalingMatrix4(T scaleFactor) noexcept;

template<typename T>
Matrix<T,3,3> scalingMatrix3(T scaleX, T scaleY, T scaleZ) noexcept;

template<typename T>
Matrix<T,4,4> scalingMatrix4(T scaleX, T scaleY, T scaleZ) noexcept;

template<typename T>
Matrix<T,4,4> translationMatrix(T deltaX, T deltaY, T deltaZ) noexcept;

template<typename T>
Matrix<T,4,4> translationMatrix(const Vector<T,3>& delta) noexcept;

// View matrices
// ------------------------------------------------------------------------------------------------

template<typename T>
Matrix<T,4,4> lookAt(const Vector<T,3>& cameraPosition, const Vector<T,3> cameraTarget,
                     const Vector<T,3> upVector) noexcept;

// Transform helper functions
// ------------------------------------------------------------------------------------------------

template<typename T>
Vector<T,3> translation(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void translation(Matrix<T,4,4>& transform, const Vector<T,3>& translation) noexcept;

template<typename T>
Vector<T,3> scaling(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void scaling(Matrix<T,4,4>& transform, const Vector<T,3>& scaling) noexcept;

template<typename T>
Vector<T,3> forward(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void forward(Matrix<T,4,4>& transform, const Vector<T,3>& forward) noexcept;

template<typename T>
Vector<T,3> backward(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void backward(Matrix<T,4,4>& transform, const Vector<T,3>& backward) noexcept;

template<typename T>
Vector<T,3> up(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void up(Matrix<T,4,4>& transform, const Vector<T,3>& up) noexcept;

template<typename T>
Vector<T,3> down(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void down(Matrix<T,4,4>& transform, const Vector<T,3>& down) noexcept;

template<typename T>
Vector<T,3> right(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void right(Matrix<T,4,4>& transform, const Vector<T,3>& right) noexcept;

template<typename T>
Vector<T,3> left(const Matrix<T,4,4>& transform) noexcept;

template<typename T>
void left(Matrix<T,4,4>& transform, const Vector<T,3>& left) noexcept;

} // namespace sfz

#include "sfz/math/MatrixSupport.inl"
