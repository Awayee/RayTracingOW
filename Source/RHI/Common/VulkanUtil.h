#pragma once
#include "Core/Log.h"
#include <vulkan/vulkan.h>

#define VK_ASSERT(x, s) do{\
	VkResult result = (x);\
	ASSERT(VK_SUCCESS == result, s);} while(false)

#define VK_CHECK(x) do{\
	VkResult result = (x);\
	CHECK(VK_SUCCESS == result);} while(false)