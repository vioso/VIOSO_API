#pragma once
#include "vulkan/vulkan.h"
#include <array>
#include <vector>
#include <memory>

namespace VK {

	/////////////////////////////////////////////////////////////////
	// structure wrappers
	#define VK_DEFCLASS( name, type ) class name : public Vk##name { public: name() noexcept: Vk##name( { VK_STRUCTURE_TYPE_##type, nullptr } ) {}; name( Vk##name const& other ) noexcept : Vk##name( other ) {}; Vk##name* operator&() { return this; } }

	VK_DEFCLASS( PhysicalDeviceProperties2, PHYSICAL_DEVICE_PROPERTIES_2 );
	VK_DEFCLASS( PhysicalDeviceMemoryProperties2, PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 );
	VK_DEFCLASS( SurfaceCapabilities2KHR, SURFACE_CAPABILITIES_2_KHR );
	VK_DEFCLASS( MemoryRequirements2KHR, MEMORY_REQUIREMENTS_2_KHR );
	VK_DEFCLASS( MemoryRequirements2, MEMORY_REQUIREMENTS_2 );
//	VK_DEFCLASS( MemoryRequirements, MEMORY_REQUIREMENTS );
	VK_DEFCLASS( SurfaceFormat2KHR, SURFACE_FORMAT_2_KHR );
	VK_DEFCLASS( DisplayProperties2KHR, DISPLAY_PROPERTIES_2_KHR );
	VK_DEFCLASS( DisplayPlaneProperties2KHR, DISPLAY_PLANE_PROPERTIES_2_KHR );
	VK_DEFCLASS( SubmitInfo, SUBMIT_INFO );
	VK_DEFCLASS( SubmitInfo2KHR, SUBMIT_INFO_2_KHR );
	VK_DEFCLASS( PresentInfoKHR, PRESENT_INFO_KHR );
	VK_DEFCLASS( BufferCreateInfo, BUFFER_CREATE_INFO );
	VK_DEFCLASS( MemoryAllocateInfo, MEMORY_ALLOCATE_INFO );

	class FenceCreateInfo : public VkFenceCreateInfo
	{
	public:
		FenceCreateInfo( VkFenceCreateFlags flags = 0 ) noexcept : VkFenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, flags } {}
		FenceCreateInfo( VkFenceCreateInfo const& other ) noexcept : VkFenceCreateInfo(other) {}
		VkFenceCreateInfo* operator&() { return this; }
	};

	class ImageCreateInfo : public VkImageCreateInfo {
	public:
		ImageCreateInfo(
			uint32_t width = 0, uint32_t height = 0,
			VkFormat format = VK_FORMAT_B8G8R8A8_UNORM,
			uint32_t mipLevels = 1,
			VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE ) noexcept
			: VkImageCreateInfo( {
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, 0, VK_IMAGE_TYPE_2D,
			format,
			{ width, height, 1 },
			mipLevels,
			1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_LINEAR,
			usage,
			sharingMode,
			0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED
								 } ) {};
		ImageCreateInfo( VkImageCreateInfo const& other ) noexcept : VkImageCreateInfo( other ) {};
		VkImageCreateInfo* operator&() { return this; }
	};

	class ImageMemoryRequirementsInfo2 : public VkImageMemoryRequirementsInfo2
	{
	public:
		ImageMemoryRequirementsInfo2( VkImage const& img = 0 ) noexcept : VkImageMemoryRequirementsInfo2( { VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR, nullptr, img } ) {}
		ImageMemoryRequirementsInfo2( VkImageMemoryRequirementsInfo2 const& other ) noexcept : VkImageMemoryRequirementsInfo2( other ) {}
		VkImageMemoryRequirementsInfo2* operator&() { return this; }
	};

	class BufferMemoryRequirementsInfo2 : public VkBufferMemoryRequirementsInfo2
	{
	public:
		BufferMemoryRequirementsInfo2( VkBuffer const& buf = 0 ) noexcept : VkBufferMemoryRequirementsInfo2( { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR, nullptr, buf } ) {}
		BufferMemoryRequirementsInfo2( VkBufferMemoryRequirementsInfo2 const& other ) noexcept : VkBufferMemoryRequirementsInfo2( other ) {}
		VkBufferMemoryRequirementsInfo2* operator&() { return this; }
	};

	class MappedMemoryRange : public VkMappedMemoryRange
	{
	public:
		MappedMemoryRange( VkDeviceMemory const& mem = 0, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE ) noexcept : VkMappedMemoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, mem, offset, size } {}
		MappedMemoryRange( VkMappedMemoryRange const& other ) noexcept : VkMappedMemoryRange( other ) {}
		VkMappedMemoryRange* operator&() { return this; }
	};

	class CommandBufferBeginInfo : public VkCommandBufferBeginInfo
	{
	public:
		CommandBufferBeginInfo( VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VkCommandBufferInheritanceInfo const* pii = nullptr ) noexcept : VkCommandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, flags, pii } {}
		CommandBufferBeginInfo( VkCommandBufferBeginInfo const& other ) noexcept : VkCommandBufferBeginInfo( other ) {}
		VkCommandBufferBeginInfo* operator&() { return this; }
	};

	class RenderPassBeginInfo : public VkRenderPassBeginInfo
	{
	public:
		template< size_t cvs_ >
		RenderPassBeginInfo( 
			VkRenderPass const& renderPass,
			VkFramebuffer const& frameBuffer,
			VkRect2D const& renderArea,
			VkClearValue const (& clearValues)[cvs_]
		) noexcept : VkRenderPassBeginInfo{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr,
			renderPass, frameBuffer, renderArea,
			uint32_t( cvs_ ), clearValues
		} {}
		RenderPassBeginInfo( VkRenderPassBeginInfo const& other ) noexcept : VkRenderPassBeginInfo( other ) {}
		VkRenderPassBeginInfo* operator&() { return this; }
	};

	class ShaderModuleCreateInfo : public VkShaderModuleCreateInfo
	{
	public:
		ShaderModuleCreateInfo( const uint32_t* code = nullptr, size_t size = 0, VkShaderModuleCreateFlags flags = 0 ) noexcept : VkShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, flags, size, code } {}
		ShaderModuleCreateInfo( VkShaderModuleCreateInfo const& other ) noexcept : VkShaderModuleCreateInfo( other ) {}
		VkShaderModuleCreateInfo* operator&() { return this; }
	};

	class PipelineShaderStageCreateInfo : public VkPipelineShaderStageCreateInfo
	{
	public:
		PipelineShaderStageCreateInfo( VkShaderModule mod = 0, VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT, char const* pName = "main" ) noexcept : VkPipelineShaderStageCreateInfo({VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, stage, mod, pName, nullptr}) {}
		PipelineShaderStageCreateInfo( VkPipelineShaderStageCreateInfo const& other ) noexcept : VkPipelineShaderStageCreateInfo( other ) {}
		VkPipelineShaderStageCreateInfo* operator&() { return this; }
	};

	class PipelineVertexInputStateCreateInfo : public VkPipelineVertexInputStateCreateInfo
	{
	public:
		template< size_t bds_, size_t lys_ >
		PipelineVertexInputStateCreateInfo( VkVertexInputBindingDescription const( &bindings )[bds_] = {}, VkVertexInputAttributeDescription const( &layouts )[lys_] = {} ) noexcept :
			VkPipelineVertexInputStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0,
			(uint32_t)bds_, bindings,
			(uint32_t)lys_, layouts
		} {}
		PipelineVertexInputStateCreateInfo( VkPipelineVertexInputStateCreateInfo const& other ) noexcept : VkPipelineVertexInputStateCreateInfo( other ) {}
		VkPipelineVertexInputStateCreateInfo* operator&() { return this; }
	};

	class PipelineInputAssemblyStateCreateInfo : public VkPipelineInputAssemblyStateCreateInfo
	{
	public:
		PipelineInputAssemblyStateCreateInfo( VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkBool32 primitiveRestartEnable = VK_FALSE ) : VkPipelineInputAssemblyStateCreateInfo( { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0, topology, primitiveRestartEnable } ) {}
		PipelineInputAssemblyStateCreateInfo( VkPipelineInputAssemblyStateCreateInfo const& other ) noexcept : VkPipelineInputAssemblyStateCreateInfo( other ) {}
		VkPipelineInputAssemblyStateCreateInfo* operator&() { return this; }
	};

	class PipelineViewportStateCreateInfo : public VkPipelineViewportStateCreateInfo
	{
	public:
		template< size_t vps_, size_t scs_ >
		PipelineViewportStateCreateInfo( VkViewport const (& viewports)[vps_], VkRect2D const (& scissors)[scs_] ) noexcept : VkPipelineViewportStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0,
			(uint32_t)vps_, viewports,
			(uint32_t)scs_, scissors
		} {}
		PipelineViewportStateCreateInfo( VkPipelineViewportStateCreateInfo const& other ) noexcept : VkPipelineViewportStateCreateInfo( other ) {}
		VkPipelineViewportStateCreateInfo* operator&() { return this; }
	};

	class PipelineRasterizationStateCreateInfo : public VkPipelineRasterizationStateCreateInfo
	{
	public:
		PipelineRasterizationStateCreateInfo( VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL ) noexcept : VkPipelineRasterizationStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0,
			FALSE, FALSE, polygonMode, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE,
			FALSE, 0.0f, 0.0f, 0.0f, 1.0f
		} {}
		PipelineRasterizationStateCreateInfo( VkPipelineRasterizationStateCreateInfo const& other ) noexcept : VkPipelineRasterizationStateCreateInfo( other ) {}
		VkPipelineRasterizationStateCreateInfo* operator&() { return this; }
	};

	class PipelineMultisampleStateCreateInfo : public VkPipelineMultisampleStateCreateInfo
	{
	public:
		PipelineMultisampleStateCreateInfo() noexcept : VkPipelineMultisampleStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0,
			VK_SAMPLE_COUNT_1_BIT, VK_FALSE,
			1.0f, nullptr, VK_FALSE, VK_FALSE
		} {}
		PipelineMultisampleStateCreateInfo( VkPipelineMultisampleStateCreateInfo const& other ) noexcept : VkPipelineMultisampleStateCreateInfo( other ) {}
		VkPipelineMultisampleStateCreateInfo* operator&() { return this; }
	};

	class StencilOpState : public VkStencilOpState
	{
	public:
		StencilOpState() noexcept : VkStencilOpState{
			VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_NEVER,
			0, 0, 0
		} {}
		StencilOpState( VkStencilOpState const& other ) : VkStencilOpState( other ) {}
		VkStencilOpState* operator&() { return this; }
	};

	class PipelineDepthStencilStateCreateInfo : public VkPipelineDepthStencilStateCreateInfo
	{
	public:
		PipelineDepthStencilStateCreateInfo() noexcept : VkPipelineDepthStencilStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0,
			VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS,
			VK_FALSE, VK_FALSE,
			StencilOpState(), StencilOpState(), 0.0f, 1.0f
		} {}
		PipelineDepthStencilStateCreateInfo( VkPipelineDepthStencilStateCreateInfo const& other ) noexcept : VkPipelineDepthStencilStateCreateInfo( other ) {}
		VkPipelineDepthStencilStateCreateInfo* operator&() { return this; }
	};

	class PipelineColorBlendAttachmentState : public VkPipelineColorBlendAttachmentState
	{
	public:
		static const VkPipelineColorBlendAttachmentState _enabled;
		static const VkPipelineColorBlendAttachmentState _disabled;
		PipelineColorBlendAttachmentState( bool blend = false ) noexcept : VkPipelineColorBlendAttachmentState{ blend ? _enabled : _disabled } {};
		PipelineColorBlendAttachmentState( VkPipelineColorBlendAttachmentState  const& other ) noexcept : VkPipelineColorBlendAttachmentState( other ) {}
		VkPipelineColorBlendAttachmentState* operator&() { return this; }
	};

	class PipelineColorBlendStateCreateInfo : public VkPipelineColorBlendStateCreateInfo
	{
	public:
		template< size_t ats_ >
		PipelineColorBlendStateCreateInfo( VkPipelineColorBlendAttachmentState const (& attachments)[ats_] ) noexcept : VkPipelineColorBlendStateCreateInfo( {
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0,
			VK_FALSE, VK_LOGIC_OP_COPY,
			uint32_t( ats_ ), attachments,
			0.0f, 0.0f, 0.0f, 0.0f } ) {}
		PipelineColorBlendStateCreateInfo( VkPipelineColorBlendStateCreateInfo const& other ) noexcept : VkPipelineColorBlendStateCreateInfo( other ) {}
		VkPipelineColorBlendStateCreateInfo* operator&() { return this; }
	};

	template< size_t dss_ = 0 >
	class PipelineDynamicStateCreateInfo : public VkPipelineDynamicStateCreateInfo
	{
	public:
		PipelineDynamicStateCreateInfo( VkDynamicState const ( &dynamicStates )[dss_] = {} ) noexcept :
			VkPipelineDynamicStateCreateInfo({
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0,
			uint32_t( dss_ ), dynamicStates } ) {}
		PipelineDynamicStateCreateInfo( VkPipelineDynamicStateCreateInfo const& other ) noexcept : VkPipelineDynamicStateCreateInfo( other ) {}
		VkPipelineDynamicStateCreateInfo* operator&() { return this; }
	};

	class AttachmentDescription : public VkAttachmentDescription
	{
	public:
		AttachmentDescription( VkFormat format = VK_FORMAT_A8B8G8R8_UNORM_PACK32, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT ) noexcept : VkAttachmentDescription( {
			0, format, samples,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
																																																													  } ) {}
		AttachmentDescription( VkAttachmentDescription const& other ) noexcept : VkAttachmentDescription( other ) {}
		VkAttachmentDescription* operator&() { return this; }
	};

	class AttachmentDescription2 : public VkAttachmentDescription2
	{
	public:
		AttachmentDescription2( VkFormat format = VK_FORMAT_A8B8G8R8_UNORM_PACK32, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT ) noexcept : VkAttachmentDescription2( {
			VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2, nullptr, 0,
			format, samples,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
																																																													  } ) {}
		AttachmentDescription2( VkAttachmentDescription2 const& other ) noexcept : VkAttachmentDescription2( other ) {}
		VkAttachmentDescription2* operator&() { return this; }
	};

	class AttachmentReference : public VkAttachmentReference
	{
	public:
		AttachmentReference( uint32_t attachment = 0, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ) noexcept : VkAttachmentReference{ attachment , layout } {}
		AttachmentReference( VkAttachmentReference const& other ) noexcept : VkAttachmentReference( other ) {}
		VkAttachmentReference* operator&() { return this; }
	};

	class AttachmentReference2 : public VkAttachmentReference2
	{
	public:
		AttachmentReference2( uint32_t attachment = 0, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ) noexcept : VkAttachmentReference2( { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,nullptr, attachment , layout, 0 } ) {}
		AttachmentReference2( VkAttachmentReference2 const& other ) noexcept : VkAttachmentReference2( other ) {}
		VkAttachmentReference2* operator&() { return this; }
	};

	template< size_t ias_ = 0, size_t cas_ = 0>
	class SubpassDescription : public VkSubpassDescription
	{
	public:
		SubpassDescription(
			VkAttachmentReference const( &inputAttachments )[ias_] = {},
			VkAttachmentReference const( &colorAttachments )[cas_] = {},
			VkAttachmentReference* pDepthStencil = nullptr
		) noexcept :
			VkSubpassDescription{
			0, VK_PIPELINE_BIND_POINT_GRAPHICS,
			uint32_t( ias_ ), inputAttachments,
			uint32_t( cas_ ), colorAttachments,
			nullptr, pDepthStencil
		} {}
		SubpassDescription( VkSubpassDescription const& other ) noexcept : VkSubpassDescription( other ) {}
		VkSubpassDescription* operator&() { return this; }
	};

	class SubpassDescription2 : public VkSubpassDescription2
	{
	public:
		template< size_t ias_ = 0, size_t cas_ = 0>
		SubpassDescription2(
			VkAttachmentReference2 const(& inputAttachments)[ias_] = nullptr,
			VkAttachmentReference2 const(& colorAttachments)[cas_] = nullptr,
			VkAttachmentReference2* pDepthStencil = nullptr,
			VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			uint32_t viewMask = 0
			) noexcept :
			VkSubpassDescription2( {
			VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2, nullptr, 0,
			bindPoint, viewMask,
			uint32_t( ias_ ), inputAttachments,
			uint32_t( cas_ ), colorAttachments,
			nullptr, pDepthStencil, 0, nullptr
			} ) {}
		SubpassDescription2( VkSubpassDescription2 const& other ) noexcept : VkSubpassDescription2( other ) {}
		VkSubpassDescription2* operator&() { return this; }
	};

	class SubpassDependency : public VkSubpassDependency
	{
	public:
		SubpassDependency( uint32_t src = 0, uint32_t dst = 0 ) noexcept : VkSubpassDependency( {
			src, dst } ) {}
		SubpassDependency( VkSubpassDependency const& other ) noexcept : VkSubpassDependency( other ) {}
		VkSubpassDependency* operator&() { return this; }
	};

	class SubpassDependency2 : public VkSubpassDependency2
	{
	public:
		SubpassDependency2( uint32_t src = 0, uint32_t dst = 0 ) noexcept : VkSubpassDependency2( {
			VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2, nullptr,
			src, dst
		} ) {}
		SubpassDependency2( VkSubpassDependency2 const& other ) noexcept : VkSubpassDependency2( other ) {}
		VkSubpassDependency2* operator&() { return this; }
	};

	template< size_t ats_, size_t sps_, size_t dps_ >
	class RenderPassCreateInfo : public VkRenderPassCreateInfo
	{
	public:
		RenderPassCreateInfo( VkAttachmentDescription const( &attachments )[ats_], VkSubpassDescription const ( &subpasses )[sps_], VkSubpassDependency const( &dependencies )[dps_] = {} ) noexcept :
			VkRenderPassCreateInfo{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,nullptr,0,
			uint32_t( ats_ ), attachments,
			uint32_t( sps_ ), subpasses,
			uint32_t( dps_ ), dependencies
		} {}
		RenderPassCreateInfo( VkRenderPassCreateInfo const& other ) noexcept : VkRenderPassCreateInfo( other ) {}
		VkRenderPassCreateInfo* operator&() { return this; }
	};

	class RenderPassCreateInfo2 : public VkRenderPassCreateInfo2
	{
	public:
		template< size_t ats_, size_t sps_, size_t dps_ >
		RenderPassCreateInfo2( VkAttachmentDescription2 const(& attachments)[ats_], VkSubpassDescription2 const (& subpasses)[sps_], VkSubpassDependency2 const(& dependencies)[dps_] = {} ) noexcept :
			VkRenderPassCreateInfo2( {
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,nullptr,0,
			uint32_t( ats_ ), attachments,
			uint32_t( sps_ ), subpasses,
			uint32_t( dps_ ), dependencies,
			0, 0
		} ) {}
		RenderPassCreateInfo2( VkRenderPassCreateInfo2 const& other ) noexcept : VkRenderPassCreateInfo2( other ) {}
		VkRenderPassCreateInfo2* operator&() { return this; }
	};

	template< size_t sls_ = 0, size_t pcs_ = 0 >
	class PipelineLayoutCreateInfo : public VkPipelineLayoutCreateInfo
	{
	public:
		PipelineLayoutCreateInfo(
			VkDescriptorSetLayout const ( &setLayouts )[sls_] = {},
			VkPushConstantRange const( &pushConstantRanges )[pcs_] = {} 
		) noexcept : VkPipelineLayoutCreateInfo( {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0,
			uint32_t( sls_ ), setLayouts,
			uint32_t( pcs_ ), pushConstantRanges,
		} ) {}
		PipelineLayoutCreateInfo( VkPipelineLayoutCreateInfo const& other ) noexcept : VkPipelineLayoutCreateInfo( other ) {}
		VkPipelineLayoutCreateInfo* operator&() { return this; }
	};

	class GraphicsPipelineCreateInfo : public VkGraphicsPipelineCreateInfo
	{
	public:
		GraphicsPipelineCreateInfo(
			VkRenderPass renderpass,
			uint32_t subpass = 0,
			VkPipelineLayout layout = 0,
			uint32_t nStages = 0,
			VkPipelineShaderStageCreateInfo const* stages = nullptr,
			VkPipelineVertexInputStateCreateInfo const* pVertexInputState = nullptr,
			VkPipelineInputAssemblyStateCreateInfo const* pInputAssemblyState = nullptr,
			VkPipelineTessellationStateCreateInfo const* pTessellationState = nullptr,
			VkPipelineViewportStateCreateInfo const* pViewportState = nullptr,
			VkPipelineRasterizationStateCreateInfo const* pRasterizationState = nullptr,
			VkPipelineMultisampleStateCreateInfo const* pMultisampleState = nullptr,
			VkPipelineDepthStencilStateCreateInfo const* pDepthStencilState = nullptr,
			VkPipelineColorBlendStateCreateInfo const* pColorBlendState = nullptr,
			VkPipelineDynamicStateCreateInfo const* pDynamicState = nullptr
		) noexcept :
			VkGraphicsPipelineCreateInfo( {
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0,
			nStages, stages,
			pVertexInputState, pInputAssemblyState, pTessellationState, pViewportState,
			pRasterizationState, pMultisampleState, pDepthStencilState,	pColorBlendState, pDynamicState,
			layout, renderpass, subpass, 0, 0
		} ) {}
		template< size_t sts_ = 0 >
		GraphicsPipelineCreateInfo(
			VkRenderPass renderpass,
			uint32_t subpass = 0,
			VkPipelineLayout layout = 0,
			VkPipelineShaderStageCreateInfo const( &stages )[sts_] = nullptr,
			VkPipelineVertexInputStateCreateInfo const* pVertexInputState = nullptr,
			VkPipelineInputAssemblyStateCreateInfo const* pInputAssemblyState = nullptr,
			VkPipelineTessellationStateCreateInfo const* pTessellationState = nullptr,
			VkPipelineViewportStateCreateInfo const* pViewportState = nullptr,
			VkPipelineRasterizationStateCreateInfo const* pRasterizationState = nullptr,
			VkPipelineMultisampleStateCreateInfo const* pMultisampleState = nullptr,
			VkPipelineDepthStencilStateCreateInfo const* pDepthStencilState = nullptr,
			VkPipelineColorBlendStateCreateInfo const* pColorBlendState = nullptr,
			VkPipelineDynamicStateCreateInfo const* pDynamicState = nullptr
		) noexcept :
			VkGraphicsPipelineCreateInfo( {
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0,
			uint32_t( sts_ ), stages,
			pVertexInputState, pInputAssemblyState, pTessellationState, pViewportState,
			pRasterizationState, pMultisampleState, pDepthStencilState,	pColorBlendState, pDynamicState,
			layout, renderpass, subpass, 0, 0
		} ) {}
		GraphicsPipelineCreateInfo( VkGraphicsPipelineCreateInfo const& other ) noexcept : VkGraphicsPipelineCreateInfo( other ) {}
		VkGraphicsPipelineCreateInfo* operator&() { return this; }
	};

	class ImageViewCreateInfo : public VkImageViewCreateInfo
	{
	public:
		ImageViewCreateInfo(
			VkImage const& image = 0,
			VkImageSubresourceRange sr = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 },
			VkFormat format = VK_FORMAT_B8G8R8A8_UNORM,
			VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D,
			VkImageViewCreateFlags flags = 0
		) noexcept : VkImageViewCreateInfo( {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr,
			flags, image, type, format,
			VkComponentMapping{},
			sr
		} ) {}
		ImageViewCreateInfo( VkImageViewCreateInfo const& other ) noexcept : VkImageViewCreateInfo( other ) {}
		VkImageViewCreateInfo* operator&() { return this; }
	};

	class SemaphoreCreateInfo : public VkSemaphoreCreateInfo
	{
	public:
		SemaphoreCreateInfo( VkSemaphoreCreateFlags flags = 0 ) noexcept : VkSemaphoreCreateInfo( { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, flags } ) {}
		SemaphoreCreateInfo( VkSemaphoreCreateInfo const& other ) noexcept : VkSemaphoreCreateInfo( other ) {}
		VkSemaphoreCreateInfo* operator&() { return this; }
	};

	template< size_t ats_ >
	class FramebufferCreateInfo : public VkFramebufferCreateInfo
	{
	public:
		FramebufferCreateInfo( VkRenderPass const& renderPass, VkImageView const(& attachments)[ats_], uint32_t width, uint32_t height, uint32_t layers = 1 ) noexcept :
			VkFramebufferCreateInfo( {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0,
			renderPass, uint32_t( ats_ ), attachments,
			width, height, layers
		} ) {}
		FramebufferCreateInfo( VkFramebufferCreateInfo const& other ) noexcept : VkFramebufferCreateInfo( other ) {}
		VkFramebufferCreateInfo* operator&() { return this; }
	};

	class DescriptorSetLayoutBinding : public VkDescriptorSetLayoutBinding
	{
	public:
		DescriptorSetLayoutBinding(
			uint32_t binding = 0,
			VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
		) noexcept : VkDescriptorSetLayoutBinding{
			binding, descriptorType, 1, stageFlags, nullptr
		} {}
		DescriptorSetLayoutBinding( VkDescriptorSetLayoutBinding const& other )  noexcept : VkDescriptorSetLayoutBinding( other ) {}
		VkDescriptorSetLayoutBinding* operator&() { return this; }
	};

	class DescriptorSetLayoutCreateInfo : public VkDescriptorSetLayoutCreateInfo
	{
	public:
		template< size_t bds_ >
		DescriptorSetLayoutCreateInfo(
			VkDescriptorSetLayoutBinding const (& bindings)[bds_]
		) noexcept : VkDescriptorSetLayoutCreateInfo{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0,
			uint32_t( bds_ ), bindings
		} {}
		DescriptorSetLayoutCreateInfo( VkDescriptorSetLayoutCreateInfo const& other )  noexcept : VkDescriptorSetLayoutCreateInfo( other ) {}
		VkDescriptorSetLayoutCreateInfo* operator&() { return this; }
	};

};