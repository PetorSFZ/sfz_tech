#pragma once

#include <skipifzero.hpp>

#include "sfz/rendering/Image.hpp"

namespace sfz {

// Specular BRDF LUT
// ------------------------------------------------------------------------------------------------

Image genSpecularBrdfLut(Allocator* allocator);

} // namespace sfz
