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

// Vector hash function
// ------------------------------------------------------------------------------------------------

template<typename T, size_t N>
size_t hash(const Vector<T,N>& vector) noexcept
{
	std::hash<T> hasher;
	size_t hash = 0;
	for (uint32_t i = 0; i < N; ++i) {
		// hash_combine algorithm from boost
		hash ^= hasher(vector[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
	return hash;
}

// Matrix hash function
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
size_t hash(const Matrix<T,M,N>& matrix) noexcept
{
	std::hash<T> hasher;
	size_t hash = 0;
	for (uint32_t i = 0; i < M; i++) {
		for (uint32_t j = 0; j < N; j++) {
			// hash_combine algorithm from boost
			hash ^= hasher(matrix.elements[j][i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}
	}
	return hash;
}

} // namespace sfz

namespace std {

// Vector hash struct
// ------------------------------------------------------------------------------------------------

template<typename T, size_t N>
size_t hash<sfz::Vector<T,N>>::operator() (const sfz::Vector<T,N>& vector) const noexcept
{
	return sfz::hash(vector);
}

// Matrix hash struct
// ------------------------------------------------------------------------------------------------

template<typename T, size_t M, size_t N>
size_t hash<sfz::Matrix<T,M,N>>::operator() (const sfz::Matrix<T,M,N>& matrix) const noexcept
{
	return sfz::hash(matrix);
}

} // namespace std
