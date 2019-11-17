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

#include <vulkan/vulkan.h>

namespace zg {

// Debug information loggers
// ------------------------------------------------------------------------------------------------

void vulkanLogAvailableInstanceLayers() noexcept;

void vulkanLogAvailableInstanceExtensions() noexcept;

void vulkanLogAvailablePhysicalDevices(VkInstance instance, VkSurfaceKHR surface) noexcept;

void vulkanLogDeviceExtensions(
	uint32_t index, VkPhysicalDevice device, const VkPhysicalDeviceProperties& properties) noexcept;

void vulkanLogQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept;

// Vulkan debug report callback
// ------------------------------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData);

extern VkDebugReportCallbackEXT vulkanDebugCallback;

} // namespace zg
