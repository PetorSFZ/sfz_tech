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

// Approximate equal functions
// ------------------------------------------------------------------------------------------------

template<typename T, typename EpsT>
SFZ_CUDA_CALL bool approxEqual(T lhs, T rhs, EpsT epsilon) noexcept
{
	return (lhs <= (rhs + epsilon)) && (lhs >= (rhs - epsilon));
}

template<typename T, uint32_t N>
SFZ_CUDA_CALL bool approxEqual(const Vector<T,N>& lhs, const Vector<T,N>& rhs, T epsilon) noexcept
{
	for (uint32_t i = 0; i < N; i++) {
		if (!approxEqual<T,T>(lhs[i], rhs[i], epsilon)) return false;
	}
	return true;
}

template<typename T, uint32_t M, uint32_t N>
SFZ_CUDA_CALL bool approxEqual(const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs, T epsilon) noexcept
{
	for (uint32_t i = 0; i < M; i++) {
		for (uint32_t j = 0; j < N; j++) {
			if (!approxEqual<T,T>(lhs.at(i, j), rhs.at(i, j), epsilon)) return false;
		}
	}
	return true;
}

// old
// ------------------------------------------------------------------------------------------------

template<typename ArgT, typename FloatT>
SFZ_CUDA_CALL ArgT lerp(ArgT v0, ArgT v1, FloatT t) noexcept
{
	return (FloatT(1)-t)*v0 + t*v1;
}

template<typename T>
SFZ_CUDA_CALL T clamp(T value, T minValue, T maxValue)
{
	using std::min;
	using std::max;
	return max(minValue, min(value, maxValue));
}

} // namespace sfz
