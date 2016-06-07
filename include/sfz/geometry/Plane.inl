namespace sfz {

// Constructors & destructors
// ------------------------------------------------------------------------------------------------

inline Plane::Plane(const vec3& normal, float d) noexcept
:
	mNormal(normal),
	mD{d}
{
	sfz_assert_debug(approxEqual<float>(length(normal), 1.0f, 0.025f));
}

inline Plane::Plane(const vec3& normal, const vec3& position) noexcept
:
	mNormal(normal),
	mD{dot(normal, position)}
{
	sfz_assert_debug(approxEqual<float>(length(normal), 1.0f, 0.025f));
}

// Public member functions
// ------------------------------------------------------------------------------------------------

inline float Plane::signedDistance(const vec3& point) const noexcept
{
	return dot(mNormal, point) - mD; // mNormal MUST be normalized.
}

inline vec3 Plane::closestPoint(const vec3& point) const noexcept
{
	return point - signedDistance(point)*mNormal;
}

inline size_t Plane::hash() const noexcept
{
	std::hash<float> fHasher;
	std::hash<vec3> vecHasher;
	size_t hash = 0;
	// hash_combine algorithm from boost
	hash ^= fHasher(mD) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	hash ^= vecHasher(mNormal) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}

} // namespace sfz

// Specializations of standard library for sfz::Plane
// ------------------------------------------------------------------------------------------------

namespace std {

inline size_t hash<sfz::Plane>::operator() (const sfz::Plane& plane) const noexcept
{
	return plane.hash();
}

} // namespace std
