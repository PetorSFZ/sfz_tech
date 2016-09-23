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

template<typename T>
SFZ_CUDA_CALLABLE bool approxEqual(T lhs, T rhs, T epsilon) noexcept
{
	static_assert(std::is_floating_point<T>::value, "Must be floating point type.");
	return lhs <= rhs + epsilon && lhs >= rhs - epsilon;
}

template<typename T>
SFZ_CUDA_CALLABLE bool approxEqual(T lhs, T rhs) noexcept
{
	return approxEqual<T>(lhs, rhs, defaultEpsilon<T>());
}

template<typename T, size_t N>
SFZ_CUDA_CALLABLE bool approxEqual(const Vector<T,N>& lhs, const Vector<T,N>& rhs, T epsilon) noexcept
{
	for (size_t i = 0; i < N; i++) {
		if(!approxEqual<T>(lhs[i], rhs[i], epsilon)) return false;
	}
	return true;
}

template<typename T, size_t N>
SFZ_CUDA_CALLABLE bool approxEqual(const Vector<T,N>& lhs, const Vector<T,N>& rhs) noexcept
{
	return approxEqual<T,N>(lhs, rhs, defaultEpsilon<T>());
}

template<typename T, size_t M, size_t N>
SFZ_CUDA_CALLABLE bool approxEqual(const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs, T epsilon) noexcept
{
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < N; j++) {
			if (!approxEqual<T>(lhs.at(i,j), rhs.at(i,j), epsilon)) return false;
		}
	}
	return true;
}

template<typename T, size_t M, size_t N>
SFZ_CUDA_CALLABLE bool approxEqual(const Matrix<T,M,N>& lhs, const Matrix<T,M,N>& rhs) noexcept
{
	return approxEqual<T,M,N>(lhs, rhs, defaultEpsilon<T>());
}

template<typename ArgT, typename FloatT>
SFZ_CUDA_CALLABLE ArgT lerp(ArgT v0, ArgT v1, FloatT t) noexcept
{
	return (FloatT(1)-t)*v0 + t*v1;
}

template<typename T>
SFZ_CUDA_CALLABLE T clamp(T value, T minValue, T maxValue)
{
	using std::min;
	using std::max;
	return max(minValue, min(value, maxValue));
}

} // namespace sfz
