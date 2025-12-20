#include "RHI/Common/VulkanRHI.h"
#include "RHI/Common/VulkanResources.h"
#include "Core/Log.h"
#include "Math/MathBase.h"
#include <algorithm>
#include <set>
#include <cstring>

static constexpr uint32 MIN_API_VERSION{ VK_API_VERSION_1_2 };
static const char* VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

inline void GetInstanceExtensions(std::vector<const char*>&extensions, bool enableDebug) {
	uint32 glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	extensions.resize(glfwExtensionCount);
	memcpy(extensions.data(), glfwExtensions, glfwExtensionCount * sizeof(const char*));
	if (enableDebug) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
}

inline bool CheckInstanceExtensionsSupported(const std::vector<const char*>&extensions) {
	uint32 extensionCount{ 0 };
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr), "");
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data()), "");
	for (auto extension : extensions) {
		bool supported = false;
		for (auto& extensionProperty : extensionProperties) {
			if (strcmp(extensionProperty.extensionName, extension) == 0) {
				supported = true;
				break;
			}
		}
		if (!supported) {
			return false;
		}
	}
	return true;
}

inline bool CheckLayerSupported(const std::vector<const char*>&layers) {
	uint32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	for (auto& name : layers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(name, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			LOG_WARNING("layers is not supported! %s", name);
			return false;
		}
	}
	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
	void* data) {
	if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT == severity) {
		LOG_WARNING(pCallbackData->pMessage);
	}
	else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT == severity) {
		LOG_ERROR(pCallbackData->pMessage);
	}
	else {
		LOG_DEBUG(pCallbackData->pMessage);
	}
	return VK_FALSE;
}

inline void SetupDebugInfo(VkDebugUtilsMessengerCreateInfoEXT & debugInfo) {
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = DebugCallback;
}

inline VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice) {
	// find depth format
	const std::vector<VkFormat> candidates{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT };
	VkImageTiling tiling{ VK_IMAGE_TILING_OPTIMAL };
	VkFormatFeatureFlags features{ VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT };
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	LOG_ERROR("findSupportedFormat failed");
	return VK_FORMAT_UNDEFINED;
}

inline std::vector<const char*> GetDeviceExtensions() {
	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	return extensions;
}

inline VkExtent2D GetSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32 WindowWidth, uint32 WindowHeight) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	return {
		Math::Clamp<uint32>(WindowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		Math::Clamp<uint32>(WindowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
}

inline VkSurfaceFormatKHR ChooseSurfaceFormat(const VkSurfaceFormatKHR* data, uint32 count) {
	static VkSurfaceFormatKHR s_PreferredFormat{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	ASSERT(0 != count, "No surface format available!");
	if (1 == count && data[0].format == VK_FORMAT_UNDEFINED) {
		return  s_PreferredFormat;
	}
	for (uint32 i = 0; i < count; ++i) {
		if (data[i].format == s_PreferredFormat.format && data[i].colorSpace == s_PreferredFormat.colorSpace) {
			return s_PreferredFormat;
		}
	}
	return data[0];
}

inline VkPresentModeKHR ChoosePresentMode(const VkPresentModeKHR* data, uint32 count, bool enableVSync) {
	const VkPresentModeKHR preferred = enableVSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
	for (uint32 i = 0; i < count; ++i) {
		if (data[i] == preferred) {
			return preferred;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkFormat ConvertToVkFormat(ERHIFormat InFormat) {
	switch(InFormat) {
	case ERHIFormat::R8G8B8A8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
	default: return VK_FORMAT_UNDEFINED;
	}
}

void TransitionImageLayout(VkCommandBuffer Cmd, VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = OldLayout;
	barrier.newLayout = NewLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = Image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	if (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags srcStage{};
	VkPipelineStageFlags dstStage{};
	if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	vkCmdPipelineBarrier(Cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VulkanRHI::VulkanRHI(uint32 InWindowWidth, uint32 InWindowHeight):
WindowWidth(InWindowWidth),
WindowHeight(InWindowHeight),
ContentScaleX(1.0f),
ContentScaleY(1.0f),
#if defined(_DEBUG)
bEnableDebug(true),
#else
bEnableDebug(false),
#endif
FrameIndex(0) {
	CreateGLFWWindow();
	CreateInstance();
	PickGPU();
	CreateDevice();
	CreateSwapchain();
	CreateRenderPass();
	CreateCommandPool();
	CreateDescriptorPool();
	CreateDescriptorSetLayout();
	CreateDefaultSampler();
	CreateSyncResources();

	DefaultPSO.Reset(new VulkanPSO(Device, "TextureMap.hlsl", {DescriptorSetLayout}, DefaultRenderPass));
}

VulkanRHI::~VulkanRHI() {
	DestroySwapchain();
	for(FrameSyncResource& FrameResource: FrameResources) {
		vkDestroySemaphore(Device, FrameResource.ImageAvailableSmp, nullptr);
		vkDestroySemaphore(Device, FrameResource.RenderFinishedSmp, nullptr);
		vkDestroyFence(Device, FrameResource.Fence, nullptr);

	}
	vkDestroyCommandPool(Device, CommandPool, nullptr);
	vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(Device, DescriptorPool, nullptr);
	vkDestroySampler(Device, DefaultSampler, nullptr);
	vkDestroyRenderPass(Device, DefaultRenderPass, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyDevice(Device, nullptr);

	glfwDestroyWindow(Window);
	glfwTerminate();
}

RHITextureHandle VulkanRHI::CreateTexture(uint32 width, uint32 height, uint16 layer, uint16 mip, ERHIFormat format) {
	return new VulkanTexture(Device, PhysicalDevice, width, height, layer, mip, ConvertToVkFormat(format));
}

void VulkanRHI::UpdateTextureData(RHITextureHandle Handle, const void* Data, uint64 ByteSize) {
	VulkanTexture* Texture = (VulkanTexture*)Handle;
	VulkanBuffer StagingBuffer{Device, PhysicalDevice, ByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
	void* Mapped = StagingBuffer.MapMemory(0, ByteSize);
	memcpy(Mapped, Data, ByteSize);
	StagingBuffer.UnmapMemory();

	VkCommandBuffer Cmd = AllocateCommandBuffer();
	TransitionImageLayout(Cmd, Texture->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { Texture->GetWidth(), Texture->GetHeight(), 1};
	vkCmdCopyBufferToImage(Cmd, StagingBuffer.GetBuffer(), Texture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	TransitionImageLayout(Cmd, Texture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	SubmitAndFreeCommandBufferWaitIdle(Cmd, GraphicsQueue.Queue);
}

void VulkanRHI::DestroyTexture(RHITextureHandle Handle) {
	vkQueueWaitIdle(GraphicsQueue.Queue);
	VulkanTexture* Texture = (VulkanTexture*)Handle;
	delete Texture;
}

bool VulkanRHI::DrawTexture(RHITextureHandle TextureHandle) {
	if (glfwWindowShouldClose(Window)) {
		return false;
	}

	VulkanTexture* Texture = (VulkanTexture*)TextureHandle;
	FrameSyncResource& FrameResource = FrameResources[FrameIndex];
	std::vector<std::function<void()>>& BeginRenderCallbacks = FrameBeginRenderCallbacks[FrameIndex];
	FrameIndex = (FrameIndex + 1) % FRAME_IN_FLIGHT_MAX;

	vkWaitForFences(Device, 1, &FrameResource.Fence, VK_TRUE, UINT64_MAX);
	vkResetFences(Device, 1, &FrameResource.Fence);
	for (auto& Callback : BeginRenderCallbacks) {
		Callback();
	}
	BeginRenderCallbacks.clear();


	uint32 ImageIndex;
	VkResult ImageResult = vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX, FrameResource.ImageAvailableSmp, VK_NULL_HANDLE, &ImageIndex);
	if(VK_SUCCESS != ImageResult) {
		return true;
	}

	// Allocate command buffer
	VkCommandBuffer Cmd = AllocateCommandBuffer();
	// Allocate descriptor set
	VkDescriptorSet Ds;
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &DescriptorSetLayout;
	VK_ASSERT(vkAllocateDescriptorSets(Device, &allocInfo, &Ds), "Failed to allocate descriptor sets");

	// Write descriptor set
	VkWriteDescriptorSet WriteSampler{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	WriteSampler.dstSet = Ds;
	WriteSampler.dstBinding = 0;
	WriteSampler.dstArrayElement = 0;
	WriteSampler.descriptorCount = 1;
	WriteSampler.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	VkDescriptorImageInfo SamplerInfo;
	SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	SamplerInfo.imageView = VK_NULL_HANDLE;
	SamplerInfo.sampler = DefaultSampler;
	WriteSampler.pImageInfo = &SamplerInfo;

	VkWriteDescriptorSet WriteImage{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	WriteImage.dstSet = Ds;
	WriteImage.dstBinding = 1;
	WriteImage.dstArrayElement = 0;
	WriteImage.descriptorCount = 1;
	WriteImage.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	VkDescriptorImageInfo ImageInfo;
	ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ImageInfo.imageView = Texture->GetDefaultView();
	ImageInfo.sampler = VK_NULL_HANDLE;
	WriteImage.pImageInfo = &ImageInfo;

	VkWriteDescriptorSet Writes[] = {WriteSampler, WriteImage};
	vkUpdateDescriptorSets(Device, 2, Writes, 0, nullptr);

	VkExtent2D ScaledSize = GetWindowSizeWithScale();
	// begin render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = DefaultRenderPass;
	renderPassInfo.framebuffer = SwapchainImages[ImageIndex].Framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = ScaledSize;
	VkClearValue clearValues[] = { {}, {} };
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(Cmd, & renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind pso and draw
	vkCmdBindPipeline(Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, DefaultPSO->GetPipeline());
	vkCmdBindDescriptorSets(Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, DefaultPSO->GetPipelineLayout(), 0, 1, &Ds, 0, nullptr);
	// flip y
	VkViewport Viewport{0.0f, (float)ScaledSize.height, (float)ScaledSize.width, -(float)ScaledSize.height, 0.0f, 1.0f};
	vkCmdSetViewport(Cmd, 0, 1, &Viewport);
	VkRect2D Rect2D{{0, 0}, ScaledSize};
	vkCmdSetScissor(Cmd, 0, 1, &Rect2D);
	vkCmdDraw(Cmd, 6, 1, 0, 0);
	vkCmdEndRenderPass(Cmd);

	// Submit
	vkEndCommandBuffer(Cmd);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &Cmd;

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &FrameResource.ImageAvailableSmp;
	const VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	submitInfo.pWaitDstStageMask = &WaitStage;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &FrameResource.RenderFinishedSmp;
	vkQueueSubmit(GraphicsQueue.Queue, 1, &submitInfo, FrameResource.Fence);
	// Free cmd and descriptor set in next frame
	BeginRenderCallbacks.push_back([this, Cmd, Ds]() {
		vkFreeCommandBuffers(Device, CommandPool, 1, &Cmd);
		vkFreeDescriptorSets(Device, DescriptorPool, 1, &Ds);
	});

	// Present
	VkPresentInfoKHR PresentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr };
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = &FrameResource.RenderFinishedSmp;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &Swapchain;
	PresentInfo.pImageIndices = &ImageIndex;
	PresentInfo.pResults = nullptr;
	vkQueuePresentKHR(PresentQueue->Queue, &PresentInfo);

	glfwPollEvents();
	return true;
}

void VulkanRHI::CreateGLFWWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
	Window = glfwCreateWindow((int)WindowWidth, (int)WindowHeight, PROJECT_NAME, nullptr, nullptr);

	// Update window size
	int NewWidth, NewHeight;
	glfwGetWindowSize(Window, &NewWidth, &NewHeight);
	WindowWidth = (uint32)NewWidth;
	WindowHeight = (uint32)NewHeight;
	glfwGetWindowContentScale(Window, &ContentScaleX, &ContentScaleY);

	glfwMakeContextCurrent(Window);
	glfwSetWindowUserPointer(Window, (void*)this);
	glfwSetWindowSizeCallback(Window, OnWindowResize);
	glfwSetWindowContentScaleCallback(Window, OnWindowContentResize);
}

void VulkanRHI::OnWindowResize(GLFWwindow* Window, int Width, int Height) {
	VulkanRHI* RHI = (VulkanRHI*)glfwGetWindowUserPointer(Window);
	RHI->WindowWidth = (uint32)Width;
	RHI->WindowHeight = (uint32)Height;
	RHI->RecreateSwapchain();
}

void VulkanRHI::OnWindowContentResize(GLFWwindow *Window, float ScaleX, float ScaleY) {
	VulkanRHI* RHI = (VulkanRHI*)glfwGetWindowUserPointer(Window);
	RHI->ContentScaleX = ScaleX;
	RHI->ContentScaleY = ScaleY;
	RHI->RecreateSwapchain();
}

void VulkanRHI::CreateInstance()
{
    Instance = VK_NULL_HANDLE;
	uint32 supportedVersion;
	vkEnumerateInstanceVersion(&supportedVersion);
	LOG_INFO("Supported Vulkan API version: %i.%i.%i", VK_API_VERSION_MAJOR(supportedVersion), VK_API_VERSION_MINOR(supportedVersion), VK_API_VERSION_PATCH(supportedVersion));
	if (supportedVersion < MIN_API_VERSION) {
		LOG_ERROR("Supported Vulkan API Version < MIN_API_VERSION");
		return;
	}

	// Check extensions
	std::vector<const char*> extensions;
	GetInstanceExtensions(extensions, true);
	if (!CheckInstanceExtensionsSupported(extensions)) {
		LOG_ERROR("Instance extension is not supported!");
		return;
	}

	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = PROJECT_NAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = PROJECT_NAME;
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	// create info
	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0 };
	createInfo.pApplicationInfo = &appInfo; // the appInfo is stored here
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.pNext = nullptr;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	const std::vector<const char*> layers{ VALIDATION_LAYER_NAME };


	if (bEnableDebug) {
		if (CheckLayerSupported(layers)) {
			createInfo.enabledLayerCount = layers.size();
			createInfo.ppEnabledLayerNames = layers.data();
			SetupDebugInfo(debugCreateInfo);
			debugCreateInfo.pUserData = (void*)this;
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			LOG_WARNING("[VulkanContext::CreateInstance] Validation layer is not supported!");
		}
	}
	VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &Instance), "vkCreateInstance");
	LOG_INFO("Instance Created!");

	if(bEnableDebug) {
		VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo{};
		SetupDebugInfo(DebugCreateInfo);
		DebugCreateInfo.pUserData = (void*)this;
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT_Ptr = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
		VK_ASSERT(vkCreateDebugUtilsMessengerEXT_Ptr(Instance, &DebugCreateInfo, nullptr, &DebugUtilsMessenger), "");
		LOG_INFO("Debug utils messenger created!");
	}
}

void VulkanRHI::PickGPU() {
	PhysicalDevice = VK_NULL_HANDLE;
	uint32 physicalDeviceCount;
	if (VK_SUCCESS != vkEnumeratePhysicalDevices(Instance, &physicalDeviceCount, nullptr)) {
		return;
	}
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	if (VK_SUCCESS != vkEnumeratePhysicalDevices(Instance, &physicalDeviceCount, physicalDevices.data())) {
		return;
	}

	struct PhysicalDeviceInfo {
		uint32 Order;
		VkPhysicalDevice PhysicalDevice;
		VkPhysicalDeviceProperties Properties;
	};
	std::vector<PhysicalDeviceInfo> deviceInfos;
	for (uint32 i = 0; i < physicalDeviceCount; ++i) {
		// TODO device extensions check
		// TODO device features check
		PhysicalDeviceInfo& info = deviceInfos.emplace_back();
		info.Order = i;
		info.PhysicalDevice = physicalDevices[i];
		vkGetPhysicalDeviceProperties(info.PhysicalDevice, &info.Properties);
		LOG_INFO("----Available GPU: %s", info.Properties.deviceName);
	}

	std::sort(deviceInfos.begin(), deviceInfos.end(), [](const PhysicalDeviceInfo& l, const PhysicalDeviceInfo& r)->bool {
		if (l.Properties.deviceType == r.Properties.deviceType) {
			return l.Order < r.Order;
		}
		// discrete gpu
		return l.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
			r.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
		});

	for (const PhysicalDeviceInfo& info : deviceInfos) {
		if (info.Properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
			PhysicalDevice = info.PhysicalDevice;
			LOG_INFO("Picked GPU: %s", info.Properties.deviceName);
			return;
		}
	}
	ASSERT(PhysicalDevice != VK_NULL_HANDLE, "Failed to pick GPU!");
}

void VulkanRHI::CreateDevice() {
	Device = VK_NULL_HANDLE;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
	DepthFormat = FindDepthFormat(PhysicalDevice);

	// Get queue families
		// Get queue
	uint32 queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	// get queue indices
	// // the present queue will be created after swapchain
	uint32 graphicsQueueFamilyIdx{ INVALID_INDEX_U32 }, computeQueueFamilyIdx{ INVALID_INDEX_U32 }, transferQueueFamilyIdx{ INVALID_INDEX_U32 };
	for (uint32 i = 0; i < queueFamilyCount; ++i) {
		const VkQueueFamilyProperties& prop = queueFamilyProperties[i];
		if ((prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) && INVALID_INDEX_U32 == graphicsQueueFamilyIdx) {
			graphicsQueueFamilyIdx = i;
		}
		if ((prop.queueFlags & VK_QUEUE_COMPUTE_BIT) && INVALID_INDEX_U32 == computeQueueFamilyIdx && graphicsQueueFamilyIdx != i) {
			computeQueueFamilyIdx = i;
		}
		if ((prop.queueFlags & VK_QUEUE_TRANSFER_BIT) && INVALID_INDEX_U32 == transferQueueFamilyIdx && graphicsQueueFamilyIdx != i && computeQueueFamilyIdx != i) {
			transferQueueFamilyIdx = i;
		}
	}
	// check if index not found, using shared queues
	ASSERT(INVALID_INDEX_U32 != graphicsQueueFamilyIdx, "Could not find graphics queue!");
	if (INVALID_INDEX_U32 == computeQueueFamilyIdx) {
		ASSERT(queueFamilyProperties[graphicsQueueFamilyIdx].queueFlags & VK_QUEUE_COMPUTE_BIT, "Could not find compute queue!");
		computeQueueFamilyIdx = graphicsQueueFamilyIdx;
	}
	if (INVALID_INDEX_U32 == transferQueueFamilyIdx) {
		ASSERT(queueFamilyProperties[computeQueueFamilyIdx].queueFlags & VK_QUEUE_TRANSFER_BIT, "Could not find transfer queue!");
		transferQueueFamilyIdx = computeQueueFamilyIdx;
	}

	std::set<uint32> queueFamilyIndices{ graphicsQueueFamilyIdx, computeQueueFamilyIdx, transferQueueFamilyIdx };
	uint32 queueCount = (uint32)queueFamilyIndices.size();
	static const float priority = 1.0f;
	constexpr uint32 queueCreateCount = 1u;// Always create ONE queue.
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueCount);

	queueCount = 0;
	for (uint32 queueFamilyIndex : queueFamilyIndices) {
		VkDeviceQueueCreateInfo& info = queueCreateInfos[queueCount++];
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.queueFamilyIndex = queueFamilyIndex;
		info.queueCount = queueCreateCount;
		info.pQueuePriorities = &priority;
	}

	// fill device extensions
	std::vector<const char*> extensions = GetDeviceExtensions();
	// setup features
	VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	VkPhysicalDeviceVulkan11Features features11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.shaderFloat16 = VK_TRUE;
	features12.shaderInt8 = VK_TRUE;
	features2.pNext = &features11;
	features11.pNext = &features12;
	VkPhysicalDeviceFeatures& features = features2.features;
	features.samplerAnisotropy = VK_TRUE;
	features.fragmentStoresAndAtomics = VK_TRUE;
	features.independentBlend = VK_TRUE;
	// create device
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pEnabledFeatures = &features;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = queueCount;
	deviceCreateInfo.enabledExtensionCount = extensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
	deviceCreateInfo.enabledLayerCount = 0;
	VK_ASSERT(vkCreateDevice(PhysicalDevice, &deviceCreateInfo, nullptr, &Device), "vkCreateDevice");
	LOG_DEBUG("[VulkanDevice::CreateDevice] Initialized GPU: %s, %u", PhysicalDeviceProperties.deviceName, PhysicalDeviceProperties.deviceID);

	// get queues
	constexpr uint32 fixedQueueIndex = 0;// Always get the first queue.
	GraphicsQueue.FamilyIndex = graphicsQueueFamilyIdx;
	GraphicsQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(Device, graphicsQueueFamilyIdx, fixedQueueIndex, &GraphicsQueue.Queue);
	ComputeQueue.FamilyIndex = computeQueueFamilyIdx;
	ComputeQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(Device, computeQueueFamilyIdx, fixedQueueIndex, &ComputeQueue.Queue);
	TransferQueue.FamilyIndex = transferQueueFamilyIdx;
	TransferQueue.QueueIndex = fixedQueueIndex;
	vkGetDeviceQueue(Device, transferQueueFamilyIdx, fixedQueueIndex, &TransferQueue.Queue);
}

void VulkanRHI::CreateSwapchain() {
	// Create surface
	VK_ASSERT(glfwCreateWindowSurface(Instance, Window, nullptr, &Surface), "vk create window surface");
	
	// Find present queue
	auto FindPresentQueue=[this]() -> const VulkanQueue*{
		VkBool32 isSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, GraphicsQueue.FamilyIndex, Surface, &isSupport);
		if (isSupport) {
			return &GraphicsQueue;
		}
		vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, ComputeQueue.FamilyIndex, Surface, &isSupport);
		if (isSupport) {
			return &ComputeQueue;
		}
		return nullptr;
	};
	PresentQueue = FindPresentQueue();
	ASSERT(PresentQueue, "[VulkanViewport]Could not find a present queue!");

	// Create swapchain
	// Window Size may be 0 if window is minimized
	if (WindowWidth == 0 || WindowHeight == 0) {
		return;
	}
	
	VkExtent2D ScaledSize = GetWindowSizeWithScale();
	//  get capabilities
	VkSurfaceCapabilitiesKHR capabilities{};
	VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	VkSurfaceFormatKHR surfaceFormat{};
	VkPresentModeKHR presentMode{ VK_PRESENT_MODE_FIFO_KHR };
	uint32 imageCount;
	VkExtent2D swapchainExtent = GetSwapchainExtent(capabilities, ScaledSize.width, ScaledSize.height);
	imageCount = capabilities.minImageCount + 1;
	if (0 != capabilities.maxImageCount && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	// get formats
	uint32 formatCount;
	VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &formatCount, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
	if (0 != formatCount) {
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &formatCount, formats.data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
		surfaceFormat = ChooseSurfaceFormat(formats.data(), formatCount);
	}

	// get present mode
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &presentModeCount, presentModes.data());
		presentMode = ChoosePresentMode(presentModes.data(), presentModeCount, true);
	}
	SwapchainFormat = surfaceFormat.format;

	VkSwapchainCreateInfoKHR swapchainInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0 };
	swapchainInfo.surface = Surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = surfaceFormat.format;
	swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainInfo.imageExtent = swapchainExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount = 0;
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
	vkCreateSwapchainKHR(Device, &swapchainInfo, nullptr, &Swapchain);

	// Get swap chain images
	uint32 swapchainImageCount;
	vkGetSwapchainImagesKHR(Device, Swapchain, &swapchainImageCount, nullptr);
	std::vector<VkImage> swapchainImages(swapchainImageCount);
	vkGetSwapchainImagesKHR(Device, Swapchain, &swapchainImageCount, swapchainImages.data());
	LOG_DEBUG("Image count of swapchain is %u.", swapchainImageCount);

	// Create swapchain images, views and framebuffers
	SwapchainImages.resize(swapchainImageCount);
	for (uint32 i = 0; i < swapchainImageCount; ++i) {
		SwapchainImages[i].Image = swapchainImages[i];
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0 };
		viewInfo.image = swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = surfaceFormat.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		vkCreateImageView(Device, &viewInfo, nullptr, &SwapchainImages[i].View);
	}
}

void VulkanRHI::CreateSyncResources() {
	// Create semaphores for per frame
	VkSemaphoreCreateInfo SmpInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
	VkFenceCreateInfo FenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
	for(FrameSyncResource& FrameResource: FrameResources) {
		vkCreateSemaphore(Device, &SmpInfo, nullptr, &FrameResource.ImageAvailableSmp);
		vkCreateSemaphore(Device, &SmpInfo, nullptr, &FrameResource.RenderFinishedSmp);
		vkCreateFence(Device, &FenceInfo, nullptr, &FrameResource.Fence);
	}
}

void VulkanRHI::CreateRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = SwapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = nullptr;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	VK_ASSERT(vkCreateRenderPass(Device, &renderPassInfo, nullptr, &DefaultRenderPass), "Failed to create render pass!");

	VkExtent2D ScaledSize = GetWindowSizeWithScale();
	// Create framebuffers
	for(uint32 i=0; i<SwapchainImages.size(); ++i) {
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = DefaultRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &SwapchainImages[i].View;
		framebufferInfo.width = ScaledSize.width;
		framebufferInfo.height = ScaledSize.height;
		framebufferInfo.layers = 1;
		VK_CHECK(vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &SwapchainImages[i].Framebuffer));
	}
}

void VulkanRHI::CreateCommandPool() {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = GraphicsQueue.FamilyIndex;
	poolInfo.flags = 0; // Optional
	VK_CHECK(vkCreateCommandPool(Device, &poolInfo, nullptr, &CommandPool));
}

void VulkanRHI::CreateDescriptorPool() {
	constexpr uint32 POOL_SIZE=4;
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{VK_DESCRIPTOR_TYPE_SAMPLER, POOL_SIZE},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, POOL_SIZE}
	};
	VkDescriptorPoolCreateInfo PoolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr};
	PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	PoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	PoolInfo.pPoolSizes = poolSizes.data();
	PoolInfo.maxSets = POOL_SIZE * 2;
	VK_CHECK(vkCreateDescriptorPool(Device, &PoolInfo, nullptr, &DescriptorPool));
}

void VulkanRHI::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding SamplerLayoutBinding;
	SamplerLayoutBinding.binding = 0;
	SamplerLayoutBinding.descriptorCount = 1;
	SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	SamplerLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;

	VkDescriptorSetLayoutBinding ImageLayoutBinding;
	ImageLayoutBinding.binding = 1;
	ImageLayoutBinding.descriptorCount = 1;
	ImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	ImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	ImageLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { SamplerLayoutBinding, ImageLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &DescriptorSetLayout));
}

void VulkanRHI::CreateDefaultSampler() {
	// Point sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 0;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	VK_CHECK(vkCreateSampler(Device, &samplerInfo, nullptr, &DefaultSampler));
}

void VulkanRHI::DestroySwapchain() {
	for (auto& vulkanImage : SwapchainImages) {
		vkDestroyImageView(Device, vulkanImage.View, nullptr);
		vkDestroyFramebuffer(Device, vulkanImage.Framebuffer, nullptr);
	}
	SwapchainImages.clear();
	vkDestroySwapchainKHR(Device, Swapchain, nullptr);
	Swapchain = VK_NULL_HANDLE;
	vkDestroyRenderPass(Device, DefaultRenderPass, nullptr);
	DefaultRenderPass = VK_NULL_HANDLE;
	DefaultPSO.Reset();
}

void VulkanRHI::RecreateSwapchain() {
	vkDeviceWaitIdle(Device);
	DestroySwapchain();
	CreateSwapchain();
	CreateRenderPass();
	DefaultPSO.Reset(new VulkanPSO(Device, "TextureMap.hlsl", {DescriptorSetLayout}, DefaultRenderPass));
}

VkExtent2D VulkanRHI::GetWindowSizeWithScale() {
    return VkExtent2D{
		(uint32)((float)WindowWidth * ContentScaleX),
		(uint32)((float)WindowHeight * ContentScaleY)
	};
}

VkCommandBuffer VulkanRHI::AllocateCommandBuffer() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer Cmd;
	VK_CHECK(vkAllocateCommandBuffers(Device, &allocInfo, &Cmd));

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(Cmd, &beginInfo);
	return Cmd;
}

void VulkanRHI::SubmitAndFreeCommandBufferWaitIdle(VkCommandBuffer Cmd, VkQueue Queue) {
	vkEndCommandBuffer(Cmd);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &Cmd;
	vkQueueSubmit(Queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(Queue);
	vkFreeCommandBuffers(Device, CommandPool, 1, &Cmd);
}

void VulkanRHI::SubmitAndFreeCommandBuffer(VkCommandBuffer Cmd, VkQueue Queue, VkSemaphore WaitSmp, VkSemaphore SignalSmp, VkFence Fence) {
	vkEndCommandBuffer(Cmd);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &Cmd;
	if(WaitSmp) {
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &WaitSmp;
		const VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		submitInfo.pWaitDstStageMask = &WaitStage;
	}
	else {
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
	}
	if(SignalSmp) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &SignalSmp;
	}
	else {
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
	}

	if(Fence) {
		vkResetFences(Device, 1, &Fence);
	}
	vkQueueSubmit(Queue, 1, &submitInfo, Fence);
	vkFreeCommandBuffers(Device, CommandPool, 1, &Cmd);
}
