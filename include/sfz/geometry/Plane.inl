namespace sfz {

// Constructors & destructors
// ------------------------------------------------------------------------------------------------

inline Plane::Plane(const vec3& normal, float d) noexcept
:
	mNormal(normal),
	mD{d}
{
	sfz_assert_debug(approxEqual(length(normal), 1.0f, 0.025f));
}

inline Plane::Plane(const vec3& normal, const vec3& position) noexcept
:
	mNormal(normal),
	mD{dot(normal, position)}
{
	sfz_assert_debug(approxEqual(length(normal), 1.0f, 0.025f));
}

// Public member functions
// ------------------------------------------------------------------------------------------------

inline float Plane::signedDistance(const vec3& point) const noexcept
{
	return dot(mNormal, point) - mD; // mNormal MUST be normalized.
}

} // namespace sfz
