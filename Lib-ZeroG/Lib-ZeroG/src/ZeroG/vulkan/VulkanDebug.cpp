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

#include <algorithm>

#include "ZeroG/util/CpuAllocation.hpp"
#include "ZeroG/util/Logging.hpp"
#include "ZeroG/util/Strings.hpp"
#include "ZeroG/util/Vector.hpp"
#include "ZeroG/vulkan/VulkanCommon.hpp"

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

static bool physicalDeviceQueueSupportsPresent(
	VkPhysicalDevice physicalDevice, uint32_t queueFamily, VkSurfaceKHR surface) noexcept
{
	VkBool32 supportsPresent = VK_FALSE;
	CHECK_VK vkGetPhysicalDeviceSurfaceSupportKHR(
		physicalDevice, queueFamily, surface, &supportsPresent);
	return supportsPresent;
}

static bool physicalDeviceSupportsPresent(
	VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept
{
	// Get number of queue families for device
	uint32_t numQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);

	// Go through each queue family in device and return true if any with present support exists
	for (uint32_t queueFamily = 0; queueFamily < numQueueFamilies; queueFamily++) {
		bool supportsPresent =
			physicalDeviceQueueSupportsPresent(physicalDevice, queueFamily, surface);
		if (supportsPresent) {
			return true;
		}
	}

	// Device does not support present
	return false;
}

static const char* vendorIDToString(uint32_t vendorID) noexcept
{
	switch (vendorID) {
	case 0x1002: return "AMD";
	case 0x1010: return "ImgTec";
	case 0x10DE: return "NVIDIA";
	case 0x13B5: return "ARM";
	case 0x5143: return "Qualcomm";
	case 0x8086: return "INTEL";
	default: return "UNKNOWN";
	}
}

static const char* deviceTypeToString(VkPhysicalDeviceType physicalDeviceType) noexcept
{
	switch (physicalDeviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "OTHER";
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "INTEGRATED_GPU";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "DISCRETE_GPU";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "VIRTUAL_GPU";
	case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
	}
	sfz_assert(false);
	return "";
}

static uint64_t deviceNumBytesDeviceMemory(VkPhysicalDevice physicalDevice) noexcept
{
	// Get memory properties
	VkPhysicalDeviceMemoryProperties memProperties = {};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// Iterate through all memory heaps and find the largest amount of device local memory
	uint64_t maxNumDeviceBytes = 0;
	for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
		VkMemoryHeap& memHeap = memProperties.memoryHeaps[i];
		if ((memHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
			maxNumDeviceBytes = std::max(maxNumDeviceBytes, memHeap.size);
		}
	}

	return maxNumDeviceBytes;
}

static bool flagsContainBit(VkQueueFlags flags, VkQueueFlagBits bit)
{
	return (uint32_t(flags) & uint32_t(bit)) == uint32_t(bit);
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
	sfz_assert(success);
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
	Vector<VkExtensionProperties> extensionProperties;
	bool success = extensionProperties.create(numExtensions, "logInstanceExtensionsProperties");
	sfz_assert(success);
	vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensionProperties.data());

	// Allocate memory for string
	constexpr uint32_t TMP_STR_SIZE = 32768;
	Vector<char> tmpStrVec;
	tmpStrVec.create(TMP_STR_SIZE, "logInstanceLayersTmpString");
	char* tmpStr = tmpStrVec.data();
	tmpStr[0] = '\0';
	uint32_t tmpStrBytesLeft = tmpStrVec.capacity();

	// Build extensions information strings
	printfAppend(tmpStr, tmpStrBytesLeft, "Available Vulkan instance extensions:\n");
	for (uint32_t i = 0; i < numExtensions; i++) {
		printfAppend(tmpStr, tmpStrBytesLeft, "- %s (v%u)\n",
			extensionProperties[i].extensionName,
			extensionProperties[i].specVersion);
	}

	// Log extensions informations
	ZG_INFO("%s", tmpStrVec.data());
}

void vulkanLogAvailablePhysicalDevices(VkInstance instance, VkSurfaceKHR surface) noexcept
{
	constexpr uint32_t MAX_NUM_PHYSICAL_DEVICES = 32;

	// Check how many physical devices there is
	uint32_t numPhysicalDevices = 0;
	CHECK_VK vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, nullptr);
	sfz_assert(numPhysicalDevices <= MAX_NUM_PHYSICAL_DEVICES);
	numPhysicalDevices = std::min(numPhysicalDevices, MAX_NUM_PHYSICAL_DEVICES);

	// Retrieve physical devices
	VkPhysicalDevice physicalDevices[MAX_NUM_PHYSICAL_DEVICES] = {};
	CHECK_VK vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDevices);

	// Allocate memory for string
	constexpr uint32_t TMP_STR_SIZE = 32768;
	Vector<char> tmpStrVec;
	tmpStrVec.create(TMP_STR_SIZE, "logPhysicalDevicesTmpString");
	char* tmpStr = tmpStrVec.data();
	tmpStr[0] = '\0';
	uint32_t tmpStrBytesLeft = tmpStrVec.capacity();

	// Build string containing information about all physical devices
	printfAppend(tmpStr, tmpStrBytesLeft, "Vulkan physical devices:\n");
	for (uint32_t i = 0; i < numPhysicalDevices; i++) {

		// Get properties
		VkPhysicalDevice& physicalDevice = physicalDevices[i];
		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		// Check if device supports present if a surface is specified
		const char* supportsPresentStr = "";
		if (surface != nullptr) {
			if (physicalDeviceSupportsPresent(physicalDevice, surface)) {
				supportsPresentStr = " -- Present support";
			}
		}

		printfAppend(tmpStr, tmpStrBytesLeft,
			"- %u -- %s -- %s -- %s -- Device Local Memory: %.2f GiB -- API %u.%u.%u%s\n",
			i,
			properties.deviceName,
			vendorIDToString(properties.vendorID),
			deviceTypeToString(properties.deviceType),
			float(deviceNumBytesDeviceMemory(physicalDevice)) / (1024.0f * 1024.0f * 1024.0f),
			VK_VERSION_MAJOR(properties.apiVersion),
			VK_VERSION_MINOR(properties.apiVersion),
			VK_VERSION_PATCH(properties.apiVersion),
			supportsPresentStr);
	}

	// Log physical devices
	ZG_INFO("%s", tmpStrVec.data());
}

void vulkanLogDeviceExtensions(
	uint32_t index, VkPhysicalDevice device, const VkPhysicalDeviceProperties& properties) noexcept
{
	constexpr uint32_t MAX_NUM_EXTENSIONS = 128;

	// Get number of device extensions
	uint32_t numExtensions = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr);
	sfz_assert(numExtensions < MAX_NUM_EXTENSIONS);
	numExtensions = std::min(numExtensions, MAX_NUM_EXTENSIONS);

	// Get device extensions
	VkExtensionProperties extensions[MAX_NUM_EXTENSIONS] = {};
	vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, extensions);

	// Allocate memory for string
	constexpr uint32_t TMP_STR_SIZE = 32768;
	Vector<char> tmpStrVec;
	tmpStrVec.create(TMP_STR_SIZE, "logDeviceExtensionsTmpStr");
	char* tmpStr = tmpStrVec.data();
	tmpStr[0] = '\0';
	uint32_t tmpStrBytesLeft = tmpStrVec.capacity();

	// Build string containing information about all device extensions
	printfAppend(tmpStr, tmpStrBytesLeft,
		"Device %u -- %s extensions:\n", index, properties.deviceName);
	for (uint32_t i = 0; i < numExtensions; i++) {
		printfAppend(tmpStr, tmpStrBytesLeft, "- %s (v%u)\n",
			extensions[i].extensionName,
			extensions[i].specVersion);
	}

	// Log device extensions
	ZG_INFO("%s", tmpStrVec.data());
}

void vulkanLogQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept
{
	constexpr uint32_t MAX_NUM_QUEUE_FAMILIES = 32;

	// Check how many queue families there are
	uint32_t numQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, nullptr);
	sfz_assert(numQueueFamilies < MAX_NUM_QUEUE_FAMILIES);
	numQueueFamilies = std::min(numQueueFamilies, MAX_NUM_QUEUE_FAMILIES);

	// Get queue families
	VkQueueFamilyProperties queueFamilyProperties[MAX_NUM_QUEUE_FAMILIES] = {};
	vkGetPhysicalDeviceQueueFamilyProperties(
		device, &numQueueFamilies, queueFamilyProperties);

	// Allocate memory for string
	constexpr uint32_t TMP_STR_SIZE = 32768;
	Vector<char> tmpStrVec;
	tmpStrVec.create(TMP_STR_SIZE, "logDeviceExtensionsTmpStr");
	char* tmpStr = tmpStrVec.data();
	tmpStr[0] = '\0';
	uint32_t tmpStrBytesLeft = tmpStrVec.capacity();

	// Create string with information about queue families
	printfAppend(tmpStr, tmpStrBytesLeft, "Queue families:\n");
	for (uint32_t i = 0; i < numQueueFamilies; i++) {
		const VkQueueFamilyProperties& properties = queueFamilyProperties[i];

		// Check which bits the family contains
		bool graphicsBit = flagsContainBit(properties.queueFlags, VK_QUEUE_GRAPHICS_BIT);
		bool computeBit = flagsContainBit(properties.queueFlags, VK_QUEUE_COMPUTE_BIT);
		bool transferBit = flagsContainBit(properties.queueFlags, VK_QUEUE_TRANSFER_BIT);
		bool sparseBindingBit = flagsContainBit(properties.queueFlags, VK_QUEUE_SPARSE_BINDING_BIT);
		bool protectedBit = flagsContainBit(properties.queueFlags, VK_QUEUE_PROTECTED_BIT);

		// Check for present support
		bool presentSupport = false;
		if (surface != nullptr) {
			if (physicalDeviceQueueSupportsPresent(device, i, surface)) {
				presentSupport = true;
			}
		}

		// Start of string
		printfAppend(tmpStr, tmpStrBytesLeft, "- Family %u -- Flags: ", i);

		// Flags
		if (presentSupport) printfAppend(tmpStr, tmpStrBytesLeft, "PRESENT, ");
		if (graphicsBit) printfAppend(tmpStr, tmpStrBytesLeft, "GRAPHICS, ");
		if (computeBit) printfAppend(tmpStr, tmpStrBytesLeft, "COMPUTE, ");
		if (transferBit) printfAppend(tmpStr, tmpStrBytesLeft, "TRANSFER, ");
		if (sparseBindingBit) printfAppend(tmpStr, tmpStrBytesLeft, "SPARSE BINDING, ");
		if (protectedBit) printfAppend(tmpStr, tmpStrBytesLeft, "PROTECTED, ");

		// Remove last two chars if (i.e. last ", ")
		tmpStr -= 2;
		*tmpStr = '\0';
		tmpStrBytesLeft += 2;

		printfAppend(tmpStr, tmpStrBytesLeft, " -- Count: %u\n", properties.queueCount);
	}

	// Log queue families extensions
	ZG_INFO("%s", tmpStrVec.data());
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
	ZgLogLevel level = ZG_LOG_LEVEL_NOISE; // NOISE by default
	if (performance) level = ZG_LOG_LEVEL_INFO;
	if (warning) level = ZG_LOG_LEVEL_WARNING;
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
