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

#include "ZeroG/vulkan/VulkanDebug.hpp"

#include "ZeroG/util/CpuAllocation.hpp"
#include "ZeroG/util/Logging.hpp"
#include "ZeroG/util/Strings.hpp"
#include "ZeroG/util/Vector.hpp"

namespace zg {

// Statics
// ------------------------------------------------------------------------------------------------

static bool flagsContainBit(VkDebugReportFlagsEXT flags, VkDebugReportFlagBitsEXT bit) noexcept
{
	return (uint32_t(flags) & uint32_t(bit)) == uint32_t(bit);
};

static const char* debugReportObjectTypeToString(VkDebugReportObjectTypeEXT type) noexcept
{
	switch (type) {
	case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT: return "UNKNOWN";
	case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT: return "INSTANCE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT: return "PHYSICAL_DEVICE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT: return "DEVICE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT: return "QUEUE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT: return "SEMAPHORE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT: return "COMMAND_BUFFER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT: return "FENCE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT: return "DEVICE_MEMORY";
	case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: return "BUFFER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: return "IMAGE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT: return "EVENT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT: return "QUERY_POOL";
	case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT: return "BUFFER_VIEW";
	case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT: return "IMAGE_VIEW";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT: return "SHADER_MODULE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT: return "PIPELINE_CACHE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT: return "PIPELINE_LAYOUT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT: return "RENDER_PASS";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT: return "PIPELINE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT: return "DESCRIPTOR_SET_LAYOUT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT: return "SAMPLER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT: return "DESCRIPTOR_POOL";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT: return "DESCRIPTOR_SET";
	case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT: return "FRAMEBUFFER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT: return "COMMAND_POOL";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT: return "SURFACE_KHR";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT: return "SWAPCHAIN_KHR";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT: return "DEBUG_REPORT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT: return "DISPLAY_KHR_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT: return "DISPLAY_MODE_KHR_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT: return "OBJECT_TABLE_NVX_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT: return "INDIRECT_COMMANDS_LAYOUT_NVX_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT: return "VALIDATION_CACHE_EXT_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT: return "RANGE_SIZE_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT: return "DESCRIPTOR_UPDATE_TEMPLATE_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT: return "SAMPLER_YCBCR_CONVERSION_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT: return "OBJECT_TYPE_MAX_ENUM_EXT";
	}
	return "INVALID OBJECT TYPE";
}

// Debug information loggers
// ------------------------------------------------------------------------------------------------

void vulkanLogAvailableInstanceLayers() noexcept
{
	// Retrieve number of instance layers
	uint32_t numLayers = 0;
	vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

	// Retrieve instance layers
	ZgAllocator allocator = getAllocator();
	Vector<VkLayerProperties> layerProperties;
	bool success = layerProperties.create(numLayers, "logInstanceLayersProperties");
	ZG_ASSERT(success);
	vkEnumerateInstanceLayerProperties(&numLayers, layerProperties.data());

	// Allocate memory for string
	constexpr uint32_t TMP_STR_SIZE = 32768;
	char* const tmpStrStart = reinterpret_cast<char*>(
		allocator.allocate(allocator.userPtr, TMP_STR_SIZE, "logInstanceLayersTmpString"));
	char* tmpStr = tmpStrStart;
	tmpStr[0] = '\0';
	uint32_t tmpStrBytesLeft = TMP_STR_SIZE;

	// Build layer information in temp string
	printfAppend(tmpStr, tmpStrBytesLeft, "Available Vulkan instance layers:\n");
	for (uint32_t i = 0; i < numLayers; i++) {
		printfAppend(tmpStr, tmpStrBytesLeft, "- %s  --  %s (v%u)\n",
			layerProperties[i].layerName,
			layerProperties[i].description,
			layerProperties[i].implementationVersion);
	}

	// Log layer information from temp string
	ZG_INFO("%s", tmpStrStart);

	// Deallocate memory
	allocator.deallocate(allocator.userPtr, tmpStrStart);
}

void vulkanLogAvailableInstanceExtensions() noexcept
{
	// Retrieve number of instance extensions
	uint32_t numExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

	// Retrieve layer properties
	ZgAllocator allocator = getAllocator();
	Vector<VkExtensionProperties> extensionProperties;
	bool success = extensionProperties.create(numExtensions, "logInstanceExtensionsProperties");
	ZG_ASSERT(success);
	vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensionProperties.data());

	// Allocate memory for string
	constexpr uint32_t TMP_STR_SIZE = 32768;
	char* const tmpStrStart = reinterpret_cast<char*>(
		allocator.allocate(allocator.userPtr, TMP_STR_SIZE, "logInstanceLayersTmpString"));
	char* tmpStr = tmpStrStart;
	tmpStr[0] = '\0';
	uint32_t tmpStrBytesLeft = TMP_STR_SIZE;

	// Build extensions information strings
	printfAppend(tmpStr, tmpStrBytesLeft, "Available Vulkan instance extensions:\n");
	for (uint32_t i = 0; i < numExtensions; i++) {
		printfAppend(tmpStr, tmpStrBytesLeft, "- %s (v%u)\n",
			extensionProperties[i].extensionName,
			extensionProperties[i].specVersion);
	}

	// Log extensions informations
	ZG_INFO("%s", tmpStrStart);

	// Deallocate memory
	allocator.deallocate(allocator.userPtr, tmpStrStart);
}

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
	void* pUserData)
{
	(void)object;
	(void)location;
	(void)pUserData;

	// Check the if the flags contains the following flag bits
	bool information = flagsContainBit(flags, VK_DEBUG_REPORT_INFORMATION_BIT_EXT);
	bool warning = flagsContainBit(flags, VK_DEBUG_REPORT_WARNING_BIT_EXT);
	bool performance = flagsContainBit(flags, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
	bool error = flagsContainBit(flags, VK_DEBUG_REPORT_ERROR_BIT_EXT);
	bool debug = flagsContainBit(flags, VK_DEBUG_REPORT_DEBUG_BIT_EXT);

	// Determine ZeroG log level
	ZgLogLevel level = ZG_LOG_LEVEL_INFO; // INFO by default
	if (warning || performance) level = ZG_LOG_LEVEL_WARNING;
	if (error) level = ZG_LOG_LEVEL_ERROR;

	// Convert debug report flags to strings
	char tmpStrStart[256] = {};
	char* tmpStr = tmpStrStart;
	uint32_t tmpStrBytesLeft = 256;
	if (information) printfAppend(tmpStr, tmpStrBytesLeft, "%s", "Information, ");
	if (warning) printfAppend(tmpStr, tmpStrBytesLeft, "%s", "Warning, ");
	if (performance) printfAppend(tmpStr, tmpStrBytesLeft, "%s", "Performance, ");
	if (error) printfAppend(tmpStr, tmpStrBytesLeft, "%s", "Error, ");
	if (debug) printfAppend(tmpStr, tmpStrBytesLeft, "%s", "Debug, ");

	// Remove last two chars if (i.e. last ", ") if  we added anything
	uint32_t size = 256 - tmpStrBytesLeft;
	if (size > 2) tmpStrStart[size - 2] = '\0';

	// Log message
	ZG_LOG(level,
		"=== VK_EXT_debug_report ===\nFlags: %s\nObjectType: %s\nLayer: %s\nMessageCode: %i\nMessage: %s",
		tmpStrStart,
		debugReportObjectTypeToString(objectType),
		pLayerPrefix,
		messageCode,
		pMessage);

	return VK_FALSE; // Whether the call that triggered the callback should be aborted or not
}

VkDebugReportCallbackEXT vulkanDebugCallback = {};

} // namespace zg
