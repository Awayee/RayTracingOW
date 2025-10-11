#pragma once
#include "Core/Defines.h"
#include "RHI/RHICommon.h"
#include "Core/TUniquePtr.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <functional>

class VulkanPSO;

class VulkanRHI {
public:
	VulkanRHI(uint32 InWindowWidth, uint32 InWindowHeight);
	~VulkanRHI();
	RHITextureHandle CreateTexture(uint32 width, uint32 height, uint16 layer, uint16 mip, ERHIFormat format);
	void UpdateTextureData(RHITextureHandle Handle, const void* Data, uint64 ByteSize);
	void DestroyTexture(RHITextureHandle Handle);
	bool DrawTexture(RHITextureHandle TextureHandle);

private:
	enum : uint32{
		FRAME_IN_FLIGHT_MAX = 2,
		WAIT_FENCE_MAX = 10000,
	};
	GLFWwindow* Window;
	uint32 WindowWidth;
	uint32 WindowHeight;
	bool bEnableDebug;

	VkInstance Instance;
	VkDebugUtilsMessengerEXT DebugUtilsMessenger;
	VkPhysicalDevice PhysicalDevice;
	VkPhysicalDeviceProperties PhysicalDeviceProperties;
	VkDevice Device;
	VkSurfaceKHR Surface;
	VkSwapchainKHR Swapchain;

	struct VulkanSwapchainImage {
		VkImage Image;
		VkImageView View;
		VkFramebuffer Framebuffer;
	};
	std::vector<VulkanSwapchainImage> SwapchainImages;

	VkFormat DepthFormat;
	VkFormat SwapchainFormat;

	struct VulkanQueue {
		VkQueue Queue;
		uint32 FamilyIndex;
		uint32 QueueIndex;
	};
	VulkanQueue GraphicsQueue;
	VulkanQueue ComputeQueue;
	VulkanQueue TransferQueue;
	const VulkanQueue* PresentQueue;
	VkRenderPass DefaultRenderPass;
	VkCommandPool CommandPool;
	VkDescriptorPool DescriptorPool;
	VkDescriptorSetLayout DescriptorSetLayout;
	VkSampler DefaultSampler;
	TUniquePtr<VulkanPSO> DefaultPSO;

	struct FrameSyncResource {
		VkSemaphore ImageAvailableSmp {VK_NULL_HANDLE};
		VkSemaphore RenderFinishedSmp{ VK_NULL_HANDLE };
		VkFence Fence{ VK_NULL_HANDLE };
		VkCommandBuffer Cmd{ VK_NULL_HANDLE };
		VkDescriptorSet DescriptorSet{ VK_NULL_HANDLE };
	};
	std::array<FrameSyncResource, FRAME_IN_FLIGHT_MAX> FrameResources;
	std::array<std::vector<std::function<void()>>, FRAME_IN_FLIGHT_MAX> FrameBeginRenderCallbacks;
	uint32 FrameIndex;

	void CreateGLFWWindow();
	static void OnWindowResize(GLFWwindow* Window, int Width, int Height);

	void CreateInstance();
	void PickGPU();
	void CreateDevice();
	void CreateSwapchain();
	void CreateSyncResources();
	void CreateRenderPass();
	void CreateCommandPool();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDefaultSampler();
	void DestroySwapchain();
	void RecreateSwapchain();
	VkCommandBuffer AllocateCommandBuffer();
	void SubmitAndFreeCommandBufferWaitIdle(VkCommandBuffer Cmd, VkQueue Queue);
	void SubmitAndFreeCommandBuffer(VkCommandBuffer Cmd, VkQueue Queue, VkSemaphore WaitSmp, VkSemaphore SignalSmp, VkFence Fence);
};
