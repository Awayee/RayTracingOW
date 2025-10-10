#pragma once
#include <vector>
#include "Core/Defines.h"
#include "Renderer/Common/VulkanUtil.h"

class VulkanBuffer {
public:
	VulkanBuffer(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags MemoryPropertyFlags);
	~VulkanBuffer();
	VkBuffer GetBuffer()const {return Buffer;}
	void* MapMemory(VkDeviceSize Offset, VkDeviceSize Size);
	void UnmapMemory();
private:
	VkDevice Device;
	VkBuffer Buffer;
	VkDeviceMemory Memory;
};

class VulkanTexture {
public:
	VulkanTexture(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, uint32 width, uint32 height, uint16 layer, uint16 mip, VkFormat format);
	~VulkanTexture();
	VkImage GetImage() const {return Image;}
	VkImageView GetDefaultView() const {return DefaultView;}
	uint32 GetWidth() const {return Width;}
	uint32 GetHeight() const {return Height;}
private:
	VkDevice Device;
	VkImage Image;
	VkDeviceMemory ImageMemory;
	VkImageView DefaultView;
	uint32 Width;
	uint32 Height;
	uint32 Layer;
	uint32 Mip;
	VkFormat Format;
};

class VulkanPSO {
public:
	VulkanPSO(VkDevice InDevice, const char* ShaderFileName, const std::vector<VkDescriptorSetLayout>& DescriptorSetLayouts, VkRenderPass RenderPass);
	~VulkanPSO();
	VkPipeline GetPipeline() const{return Pipeline;}
	VkPipelineLayout GetPipelineLayout() const {return PipelineLayout;}
private:
	VkDevice Device;
	VkPipelineLayout PipelineLayout;
	VkPipeline Pipeline;
};