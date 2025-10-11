#include "RHI/Common/VulkanResources.h"
#include "RHI/ShaderCompiler.h"

uint32 FindMemoryType(VkPhysicalDevice PhysicalDevice, uint32_t typrFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typrFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
			return i;
		}
	}
	return INVALID_INDEX_U32;
}

inline std::string GetCompiledSPVFileName(const char* FileName, const char* EntryName) {
	std::string Output{ FileName };
	if (size_t ExtIdx = Output.rfind(".hlsl"); ExtIdx != std::string::npos) {
		Output.erase(ExtIdx);
	}
	Output.append(EntryName).append(".spv");
	return Output;
}


class VulkanShader {
public:
	VulkanShader(VkDevice InDevice, const char* FileName, const char* EntryName, const char* SM) : Device(InDevice) {
		const std::string CompiledFileName = GetCompiledSPVFileName(FileName, EntryName);
		const std::string CompiledFilePath = std::string{ SHADER_PATH }.append(CompiledFileName);

		std::vector<char> Bytes;
		if (!ShaderCompiler::ReadCompiledShaderFile(CompiledFilePath.c_str(), EntryName, SM, Bytes)) {
			const std::string ShaderFullPath = std::string{ SHADER_PATH }.append(FileName);
			const std::vector<ShaderCompiler::Macro> Defines{{"RHI_VULKAN", "1"}};
			ShaderCompiler::CompileShaderToSPV(ShaderFullPath.c_str(), EntryName, SM, Defines, Bytes);
			ShaderCompiler::SaveCompiledShaderFile(CompiledFilePath.c_str(), Bytes);
		}
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = Bytes.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(Bytes.data());
		VK_ASSERT(vkCreateShaderModule(Device, &createInfo, nullptr, &ShaderModule), "Failed to create shader module");
	}
	~VulkanShader() {
		vkDestroyShaderModule(Device, ShaderModule, nullptr);
	}
	VkShaderModule GetShaderModule() const { return ShaderModule; }
private:
	VkDevice Device;
	VkShaderModule ShaderModule;
};

VulkanBuffer::VulkanBuffer(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags MemoryPropertyFlags) : Device(InDevice){
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = Size;
	bufferInfo.usage = Usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(Device, &bufferInfo, nullptr, &Buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(Device, Buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(InPhysicalDevice, memRequirements.memoryTypeBits, MemoryPropertyFlags);

	VK_CHECK(vkAllocateMemory(Device, &allocInfo, nullptr, &Memory));
	vkBindBufferMemory(Device, Buffer, Memory, 0);
}

VulkanBuffer::~VulkanBuffer() {
	vkDestroyBuffer(Device, Buffer, nullptr);
	vkFreeMemory(Device, Memory, nullptr);
}

void* VulkanBuffer::MapMemory(VkDeviceSize Offset, VkDeviceSize Size) {
	void* Mapped;
	vkMapMemory(Device, Memory, Offset, Size, 0, &Mapped);
	return Mapped;
}

void VulkanBuffer::UnmapMemory() {
	vkUnmapMemory(Device, Memory);
}

VulkanTexture::VulkanTexture(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, uint32 width, uint32 height, uint16 layer, uint16 mip, VkFormat format):
Device(InDevice), Width(width), Height(height), Layer(layer), Mip(mip), Format(format) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = Width;
	imageInfo.extent.height = Height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = Mip;
	imageInfo.arrayLayers = Layer;
	imageInfo.format = Format;
	imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateImage(Device, &imageInfo, nullptr, &Image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(Device, Image, &memRequirements);
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(InPhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(Device, &allocInfo, nullptr, &ImageMemory));

	vkBindImageMemory(Device, Image, ImageMemory, 0);

	// image view
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	VK_CHECK(vkCreateImageView(Device, &viewInfo, nullptr, &DefaultView));
}

VulkanTexture::~VulkanTexture() {
	vkDestroyImageView(Device, DefaultView, nullptr);
	vkDestroyImage(Device, Image, nullptr);
	vkFreeMemory(Device, ImageMemory, nullptr);
}

VulkanPSO::VulkanPSO(VkDevice InDevice, const char* ShaderFileName, const std::vector<VkDescriptorSetLayout>& DescriptorSetLayouts, VkRenderPass RenderPass): Device(InDevice) {
	// pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = DescriptorSetLayouts.size(); // 
	pipelineLayoutInfo.pSetLayouts = DescriptorSetLayouts.data(); // 
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
	VK_CHECK(vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &PipelineLayout));


	VkGraphicsPipelineCreateInfo PipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	VulkanShader VS{Device, ShaderFileName, "MainVS", "vs_6_0"};
	VulkanShader PS{Device, ShaderFileName, "MainPS", "ps_6_0"};

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	VkPipelineShaderStageCreateInfo& VSInfo = ShaderStages.emplace_back();
	VSInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VSInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VSInfo.module = VS.GetShaderModule();
	VSInfo.pName = "MainVS";
	VkPipelineShaderStageCreateInfo& PSInfo = ShaderStages.emplace_back();
	PSInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	PSInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	PSInfo.module = PS.GetShaderModule();
	PSInfo.pName = "MainPS";
	

	PipelineInfo.stageCount = static_cast<uint32_t> (ShaderStages.size());
	PipelineInfo.pStages = ShaderStages.data();

	VkPipelineVertexInputStateCreateInfo VertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0};
	VertexInputInfo.vertexBindingDescriptionCount = 0;
	VertexInputInfo.pVertexBindingDescriptions = nullptr;
	VertexInputInfo.vertexAttributeDescriptionCount = 0;
	VertexInputInfo.pVertexAttributeDescriptions = nullptr;
	PipelineInfo.pVertexInputState = &VertexInputInfo;

	VkPipelineInputAssemblyStateCreateInfo InputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0};
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssembly.primitiveRestartEnable = VK_FALSE;
	PipelineInfo.pInputAssemblyState = &InputAssembly;

	VkPipelineViewportStateCreateInfo ViewportInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO , nullptr, 0};
	ViewportInfo.viewportCount = 1;
	ViewportInfo.scissorCount = 1;
	PipelineInfo.pViewportState = &ViewportInfo;

	VkPipelineRasterizationStateCreateInfo Rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0};
	Rasterizer.depthClampEnable = VK_FALSE;
	Rasterizer.rasterizerDiscardEnable = VK_FALSE;
	Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	Rasterizer.lineWidth = 1.0f;
	Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	Rasterizer.depthBiasEnable = VK_FALSE;
	Rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	Rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
	Rasterizer.depthBiasClamp = 0.0f; // Optional
	PipelineInfo.pRasterizationState = &Rasterizer;

	VkPipelineMultisampleStateCreateInfo Multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0};
	Multisampling.sampleShadingEnable = VK_FALSE;
	Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	Multisampling.minSampleShading = 1.0f; // Optional
	Multisampling.pSampleMask = nullptr; // Optional
	Multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	Multisampling.alphaToOneEnable = VK_FALSE; // Optional
	PipelineInfo.pMultisampleState = &Multisampling;


	VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachment.blendEnable = VK_TRUE;
	ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendStateCreateInfo ColorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &ColorBlendAttachment;
	ColorBlending.blendConstants[0] = 0.0f; // Optional
	ColorBlending.blendConstants[1] = 0.0f; // Optional
	ColorBlending.blendConstants[2] = 0.0f; // Optional
	ColorBlending.blendConstants[3] = 0.0f; // Optional
	PipelineInfo.pColorBlendState = &ColorBlending;

	VkDynamicState DynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo DynamicStateInfo{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	DynamicStateInfo.dynamicStateCount = 2;
	DynamicStateInfo.pDynamicStates = DynamicStates;
	PipelineInfo.pDynamicState = &DynamicStateInfo;

	VkPipelineDepthStencilStateCreateInfo DepthStencilInfo{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	DepthStencilInfo.depthTestEnable = VK_TRUE;
	DepthStencilInfo.depthWriteEnable = VK_TRUE;
	DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	DepthStencilInfo.minDepthBounds = 0.0f;
	DepthStencilInfo.maxDepthBounds = 1.0f;
	DepthStencilInfo.stencilTestEnable = VK_FALSE;
	DepthStencilInfo.front = {};
	DepthStencilInfo.back = {};
	PipelineInfo.pDepthStencilState = &DepthStencilInfo;

	PipelineInfo.layout = PipelineLayout;
	PipelineInfo.renderPass = RenderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	PipelineInfo.basePipelineIndex = -1;
	VK_CHECK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline));
}

VulkanPSO::~VulkanPSO() {
	vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	vkDestroyPipeline(Device, Pipeline, nullptr);
}
