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

#include <sfz/geometry/Plane.hpp>
#include <sfz/geometry/ViewFrustum.hpp>
#include <sfz/math/Vector.hpp>

// Stupid hack for stupid near/far macros (windows.h)
#undef near
#undef far

namespace sfz {
	
// Forward declares geometry primitives
class AABB;
class OBB;
class Sphere;

class ViewFrustum final {
public:
	// Constructors & destructors
	// --------------------------------------------------------------------------------------------

	ViewFrustum() noexcept = default;
	ViewFrustum(const ViewFrustum&) noexcept = default;
	ViewFrustum& operator= (const ViewFrustum&) noexcept = default;

	ViewFrustum(vec3 position, vec3 direction, vec3 up, float verticalFovDeg, float aspect,
	            float near, float far) noexcept;
	
	// Public methods
	// --------------------------------------------------------------------------------------------

	bool isVisible(const AABB& aabb) const noexcept;
	bool isVisible(const OBB& obb) const noexcept;
	bool isVisible(const Sphere& sphere) const noexcept;
	bool isVisible(const ViewFrustum& viewFrustum) const noexcept;

	// Getters
	// --------------------------------------------------------------------------------------------

	inline vec3 pos() const noexcept { return mPos; }
	inline vec3 dir() const noexcept { return mDir; }
	inline vec3 up() const noexcept { return mUp; }
	inline float verticalFov() const noexcept { return mVerticalFovDeg; }
	inline float aspectRatio() const noexcept { return mAspectRatio; }
	inline float near() const noexcept { return mNear; }
	inline float far() const noexcept { return mFar; }
	inline const mat4& viewMatrix() const noexcept { return mViewMatrix; }
	inline const mat4& projMatrix() const noexcept { return mProjMatrix; }

	// Setters
	// --------------------------------------------------------------------------------------------

	void setPos(vec3 position) noexcept;
	void setVerticalFov(float verticalFovDeg) noexcept;
	void setAspectRatio(float aspect) noexcept;

	void setDir(vec3 direction, vec3 up) noexcept;
	void setClipDist(float near, float far) noexcept;
	void set(vec3 position, vec3 direction, vec3 up, float verticalFovDeg, float aspect, float near,
	         float far) noexcept;

private:
	// Private methods
	// --------------------------------------------------------------------------------------------

	void update() noexcept;
	void updateMatrices() noexcept;
	void updatePlanes() noexcept;

	// Private members
	// --------------------------------------------------------------------------------------------

	vec3 mPos, mDir, mUp;
	float mVerticalFovDeg, mAspectRatio, mNear, mFar;
	mat4 mViewMatrix, mProjMatrix;
	Plane mNearPlane, mFarPlane, mUpPlane, mDownPlane, mLeftPlane, mRightPlane;
};

} // namespace sfz
