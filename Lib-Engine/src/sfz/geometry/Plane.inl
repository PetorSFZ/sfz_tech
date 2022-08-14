namespace sfz {

// Constructors & destructors
// ------------------------------------------------------------------------------------------------

inline Plane::Plane(const f32x3& normal, f32 d) noexcept
:
	mNormal(normal),
	mD{d}
{
	sfz_assert(eqf(f32x3_length(normal), 1.0f, 0.025f));
}

inline Plane::Plane(const f32x3& normal, const f32x3& position) noexcept
:
	mNormal(normal),
	mD(f32x3_dot(normal, position))
{
	sfz_assert(eqf(f32x3_length(normal), 1.0f, 0.025f));
}

// Public member functions
// ------------------------------------------------------------------------------------------------

inline f32 Plane::signedDistance(const f32x3& point) const noexcept
{
	return f32x3_dot(mNormal, point) - mD; // mNormal MUST be normalized.
}

} // namespace sfz
