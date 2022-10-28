#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment( lib, "vulkan-1.lib" )
#pragma comment( lib, "VkLayer_utils.lib" )
#endif
#include "VK.h"


#define _USE_MATH_DEFINES
#include <corecrt_math_defines.h>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <any>
#include <type_traits>
#include <string>
using namespace std;

namespace VK
{

    bool test()
    {
        return true;
    }

    //--------------------------------------------------------------------------------------
    // static initialization
    //--------------------------------------------------------------------------------------
    const VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState::_disabled{
        VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    const VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState::_enabled{
        VK_TRUE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    //--------------------------------------------------------------------------------------
    // Image
    // 
    //--------------------------------------------------------------------------------------

    Image::~Image() {

    }

    //--------------------------------------------------------------------------------------
    // TextureImage
    //--------------------------------------------------------------------------------------

    TextureImage::TextureImage( GFX const& gfx, VkImageCreateInfo const& ci, VkMemoryAllocateFlags memFlags )
    : Image( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    {
        m_ci = ci;
        if( ci.extent.width * ci.extent.height * ci.extent.depth )
        {
            _rts( vkCreateImage( gfx.getDevice(), &ci, _defAlloc, m_image.set( gfx.getDevice() ) ), "Failed to create image." );

            VkMemoryRequirements mem_reqs;
            vkGetImageMemoryRequirements( gfx.getDevice(), m_image, &mem_reqs );

            VkMemoryAllocateInfo mem_alloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0 };
            mem_alloc.allocationSize = mem_reqs.size;
            mem_alloc.memoryTypeIndex = gfx.findMemTypeFromProps( mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            _rtc( mem_alloc.memoryTypeIndex < VK_MAX_MEMORY_TYPES, "required memory type for depth buffer not available." );

            _rts( vkAllocateMemory( gfx.getDevice(), &mem_alloc, _defAlloc, m_mem.set( gfx.getDevice() ) ), "failed to allocate image memory for depth buffer." );

            _rts( vkBindImageMemory( gfx.getDevice(), m_image, m_mem, 0 ), "failed to bind image memory to depth buffer." );

            ImageViewCreateInfo ivCI(
                m_image,
                { VkImageAspectFlags( ( ci.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT ), 0, ci.mipLevels, 0, 1 },
                ci.format );
            _rts( vkCreateImageView( gfx.getDevice(), &ivCI, _defAlloc, m_view.set( gfx.getDevice() ) ), "failed to create view for deth buffer." );
        }
    }

    //--------------------------------------------------------------------------------------
    // DepthBuffer
    //--------------------------------------------------------------------------------------

    DepthBuffer::DepthBuffer( GFX const& gfx, VkImageCreateInfo const& ci, VkMemoryAllocateFlags )
    : Image( VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
    {
        m_ci = ci;
        if( ci.extent.width * ci.extent.height * ci.extent.depth )
        {
            _rts( vkCreateImage( gfx.getDevice(), &ci, _defAlloc, m_image.set( gfx.getDevice() ) ), "Failed to create image." );

            VkMemoryRequirements mem_reqs;
            vkGetImageMemoryRequirements( gfx.getDevice(), m_image, &mem_reqs );

            VkMemoryAllocateInfo mem_alloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0 };
            mem_alloc.allocationSize = mem_reqs.size;
            mem_alloc.memoryTypeIndex = gfx.findMemTypeFromProps( mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            _rtc( mem_alloc.memoryTypeIndex < VK_MAX_MEMORY_TYPES, "required memory type for depth buffer not available." );

            _rts( vkAllocateMemory( gfx.getDevice(), &mem_alloc, _defAlloc, m_mem.set( gfx.getDevice() ) ), "failed to allocate image memory for depth buffer." );

            _rts( vkBindImageMemory( gfx.getDevice(), m_image, m_mem, 0 ), "failed to bind image memory to depth buffer." );

            ImageViewCreateInfo ivCI(
                m_image,
                { VkImageAspectFlags( ( ci.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT ), 0, ci.mipLevels, 0, 1 },
                ci.format );
            _rts( vkCreateImageView( gfx.getDevice(), &ivCI, _defAlloc, m_view.set( gfx.getDevice() ) ), "failed to create view for deth buffer." );
        }
    }

    //--------------------------------------------------------------------------------------
    // SwapChainImage
    //--------------------------------------------------------------------------------------
    SwapchainImage::SwapchainImage( GFX const& gfx, VkImage image, VkFormat format, uint32_t width, uint32_t height )
    : Image( VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
    , m_image(image)
    {
        m_ci = ImageCreateInfo( width, height, format, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT );
        ImageViewCreateInfo ivCI( image, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, format );

        _rts( vkCreateImageView( gfx.getDevice(), &ivCI, _defAlloc, m_view.set( gfx.getDevice() ) ), "could not create image view." );
    }

    SwapchainImage::~SwapchainImage()
    {
    }

    //--------------------------------------------------------------------------------------
   // RenderTarget
   //--------------------------------------------------------------------------------------
    const VkClearColorValue RenderTarget::s_black{};
    const VkClearColorValue RenderTarget::s_sky{ { 0.59f, 0.86f, 1.0f, 1.0f } };
    const VkClearDepthStencilValue RenderTarget::s_clearDepth{ 1.0f, 0 };

    RenderTarget::RenderTarget( DeviceH const& dev, std::vector<VkImage> const& images, VkExtent3D const& extent, VkFormat format, VkFormat depthFormat, uint32_t mipLevels )
    : m_currentBuffer( 0 )
    , m_extent( extent )
    , m_mipLevels( mipLevels )
    , m_format( format )
    , m_depthFormat( depthFormat )
    , m_dev( dev )
    {
        // create semaphores for acquireing
        for( auto& image : images )
        {
            _rts( vkCreateSemaphore( dev, &SemaphoreCreateInfo(), _defAlloc, m_imageAcquireSemas.emplace_back().set( dev ) ), "ERROR failed to crate image acquire semahore." );
        }
    }

    std::map<VkImage, FramebufferH> RenderTarget::createFramebufferMap( RenderPassH const& renderPass ) const
    {
        std::map<VkImage, FramebufferH> fbm;
        for( size_t i = 0; i != m_images.size(); i++ )
        {
            _rts( vkCreateFramebuffer(
                m_dev,
                &FramebufferCreateInfo(
                    renderPass,
                    { VkImageView(*m_images[i]), VkImageView(m_depthBuffers[i]) },
                    m_images[i]->getCI().extent.width, m_images[i]->getCI().extent.height, m_images[i]->getCI().extent.depth ),
                _defAlloc,
                fbm[*m_images[i]].set( m_dev )
            ), "failed to create framebuffer." );
        }
        return fbm;
    }

    void RenderTarget::resize( int width, int height, DeviceH& dev, SwapchainKHRH& sc )
    {
        // BIG TODO
    }

     //--------------------------------------------------------------------------------------
    // MappedConstantBufferBase
    //--------------------------------------------------------------------------------------
    GPUBuffer::GPUBuffer( GFX const& gfx, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags, VkSharingMode sharingMode )
    {
        BufferCreateInfo ci;
        ci.size = size;
        ci.sharingMode = sharingMode;
        ci.usage = usage;
        _rts( vkCreateBuffer( gfx.getDevice(), &ci, _defAlloc, m_hBuf.set( gfx.getDevice() ) ), "Failed to vkCreateBuffer.");

        VkMemoryRequirements mr;
        BufferMemoryRequirementsInfo2 mri( m_hBuf );
        vkGetBufferMemoryRequirements( gfx.getDevice(), m_hBuf, &mr );
   	    _rtc( size <= mr.size, "Constant buffer GPU memory size differs." );

        MemoryAllocateInfo mi; mi.allocationSize = mr.size;
        mi.memoryTypeIndex = -1;
        _rtc( -1 != ( mi.memoryTypeIndex = gfx.findMemTypeFromProps( mr.memoryTypeBits, memFlags ) ), "Could not find requested GPU heap for buffer." );

        _rts( vkAllocateMemory( gfx.getDevice(), &mi, _defAlloc, m_hMem.set( gfx.getDevice() ) ), "Could not allocate GPU memory for buffer." );

        _rts( vkBindBufferMemory( gfx.getDevice(), m_hBuf, m_hMem, 0 ), "Failed to bind GPU memory to buffer.");
    }

    void GPUBuffer::updateStaging( GFX const& gfx, void const* data, size_t size )
    {
        GPUBuffer stage( gfx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        stage.updateMapped( data, size );
        gfx.submitBufferCopy( stage, *this, size );
    }

    //--------------------------------------------------------------------------------------
    // BackBuffer
    //--------------------------------------------------------------------------------------
    BackBuffer::BackBuffer(GFX const& gfx, SwapchainKHRH const& sc, VkFormat format, VkExtent2D extent, VkFormat depthFormat)
    : RenderTarget(gfx.getDevice(), vectorize<VkImage, VkDevice, VkSwapchainKHR>(vkGetSwapchainImagesKHR, gfx.getDevice(), sc))
    , m_sc( sc )
    {
        m_extent = VkExtent3D{ extent.width, extent.height, 1 };
        m_format = format;
        m_depthFormat = depthFormat;

        auto images = vectorize<VkImage, VkDevice, VkSwapchainKHR>(vkGetSwapchainImagesKHR, gfx.getDevice(), sc);
        _rtc( !images.empty(), "could not get images from swapchain." );

        m_images.clear();

        for( auto& image : images ) {
            m_images.emplace_back( make_unique<SwapchainImage>( gfx, image, m_format, extent.width, extent.height ) );

            if( VK_FORMAT_UNDEFINED != depthFormat )
            {
                const VkImageCreateInfo iCI{
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    nullptr,
                    0,
                    VK_IMAGE_TYPE_2D,
                    depthFormat,
                    m_extent,
                    1,
                    1,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0,
                    nullptr,
                    VK_IMAGE_LAYOUT_UNDEFINED
                };

                m_depthBuffers.emplace_back( gfx, iCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            }
        }
    }

    Image const& BackBuffer::getNextBuffer()
    {
        VkResult res = vkAcquireNextImageKHR( m_dev, m_sc, UINT64_MAX, m_imageAcquireSemas.back(), 0, atomicRef(m_currentBuffer) ); // this will block, if all images are in-flight
        m_images[m_currentBuffer]->swapSema( m_imageAcquireSemas.back() );
        m_imageAcquireSemas.pop_back();

        if( VK_ERROR_OUT_OF_DATE_KHR == res )
            throw out_of_date( "Need resize while acquireing backbuffer image" );
        else if( VK_ERROR_DEVICE_LOST == res )
            throw device_lost( "Device lost while acquiring next backbuffer image." );
        else if( res != VK_SUCCESS )
            throw runtime_error( "Unknown error while acquiering backbuffer image" );
        return *m_images[m_currentBuffer];
    }

    //--------------------------------------------------------------------------------------
    // RenderTexture
    //--------------------------------------------------------------------------------------

    // NOTE we need to signal the semaphore

    //--------------------------------------------------------------------------------------
    // Renderer
    //--------------------------------------------------------------------------------------

    uint64_t Renderer::s_freeID = 0;

    #pragma warning( push )
    #pragma warning( disable : 4309 )
    #pragma warning( disable : 4838 )
    //const char* Renderer::s_passthroughVS_bytecode // in/out vec4 position and vec2 tc 
    #include "passthroughVS.h"
    //const char* Renderer::s_passthroughFS_bytecode; // in vec4 position and vec2 tc, out vec 
    #include "passthroughFS.h"
    #pragma warning( pop )

    // NOTE: filling the PipelineVertexInputStateCreateInfo need lists
    // somewhere in memory, it does not work filling with initializer list, as it gets
    // unallocated as soon as we leave the constructor, thus invalidates the pointer to
    // the lists
    const PipelineVertexInputStateCreateInfo Renderer::Vertex::_layout( 
        { VkVertexInputBindingDescription{0, sizeof( Vertex ), VK_VERTEX_INPUT_RATE_VERTEX} },
        { VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( Vertex, x )} } );

    const PipelineVertexInputStateCreateInfo Renderer::VertexWCol::_layout(
        { VkVertexInputBindingDescription{0, sizeof( VertexWCol ), VK_VERTEX_INPUT_RATE_VERTEX} },
        { 
            VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexWCol, x )},
            VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof( VertexWCol, r )},
        } );
    const PipelineVertexInputStateCreateInfo Renderer::VertexWTex::_layout(
        { VkVertexInputBindingDescription{0, sizeof( VertexWTex ), VK_VERTEX_INPUT_RATE_VERTEX} },
        {
            VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexWTex, x )},
            VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexWTex, u )},
        } );
    const PipelineVertexInputStateCreateInfo Renderer::VertexWColTex::_layout(
        { VkVertexInputBindingDescription{0, sizeof( VertexWColTex ), VK_VERTEX_INPUT_RATE_VERTEX} },
        {
            VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexWColTex, x )},
            VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof( VertexWCol, r )},
            VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof( VertexWColTex, u )},
        } );

    Renderer::Renderer( 
        GFX const& gfx,
        vector<VkDynamicState> const& dynamicStates,
        vector<shared_ptr<ShaderModule>>&& shaderStages,
        RenderTarget const& rt,
        shared_ptr <UniformBuffer>&& uniformBuffer,
        shared_ptr <VertexBuffer>&& vertexBuffer,
        vector<shared_ptr<Sampler>>&& samplers,
        uint32_t nFramesAhed)
    : m_id( ++s_freeID )
    , m_dev( gfx.getDevice() )
    , m_iFrame( 0 )
    , m_cbs( gfx.createCommandBuffers( nFramesAhed ) )
    , m_sss( std::move( shaderStages ))
    , m_sms( std::move( samplers ))
    , m_ub( std::move( uniformBuffer ))
    , m_vb( std::move( vertexBuffer ))
    {
        PipelineVertexInputStateCreateInfo pvisCI;
        if(m_vb)
            pvisCI = m_vb->getInputLayout();
        auto colAttRefs = rt.getColorAttachmentReferences();
		_rts( vkCreateRenderPass(
			gfx.getDevice(),
			&RenderPassCreateInfo(
                rt.getAttachmentDescriptions(),
                { SubpassDescription( {}, colAttRefs, rt.getDepthStencilAttachmentReference( uint32_t(colAttRefs.size()) ).get() ) } ),
			_defAlloc,
			m_renderPass.set( gfx.getDevice() )
		), "Failed to create renderPass." );

		//_rts( vkCreateDescriptorSetLayout(
		//	gfx.getDevice(),
		//	&DescriptorSetLayoutCreateInfo( { DescriptorSetLayoutBinding() } ),
		//	_defAlloc,
		//	m_descriptorSetLayout.set( gfx.getDevice() )
		//), "failed to vkCreateDescriptorSetLayout." );

		_rts( vkCreatePipelineLayout(
			gfx.getDevice(),
			&PipelineLayoutCreateInfo(), // TODO: create from uniformBuffer and Samplers
			_defAlloc,
            m_pipelineLayout.set( gfx.getDevice() )
        ), "failed to vkCreatePipelineLayout." );

        vector< VkPipelineShaderStageCreateInfo > pssCIs;
        for( auto& sm : m_sss )
            pssCIs.push_back( sm->getCI() );

        _rts( vkCreateGraphicsPipelines( 
            gfx.getDevice(), 0, 1,
            &GraphicsPipelineCreateInfo(
                m_renderPass, 0, m_pipelineLayout,
                uint32_t( pssCIs.size() ), pssCIs.data(),
                m_vb ? &m_vb->getInputLayout() : &PipelineVertexInputStateCreateInfo(), 
                &PipelineInputAssemblyStateCreateInfo(),
                nullptr,
                &rt.getPipelineViewportStateCreateInfo(),
                &PipelineRasterizationStateCreateInfo(),
                &PipelineMultisampleStateCreateInfo(),
                rt.getPipelineDepthStencilStateCreateInfo().get(),
                &PipelineColorBlendStateCreateInfo( { PipelineColorBlendAttachmentState( false ) } ),
                &PipelineDynamicStateCreateInfo(dynamicStates)
            ),
            _defAlloc, m_pipeline.set( gfx.getDevice() )
        ), "Failed to create pipeline for Cubes Renderer." );

        m_framebuffers = rt.createFramebufferMap( m_renderPass );

        const SemaphoreCreateInfo semaphoreCI;
        const FenceCreateInfo fenceCI( VK_FENCE_CREATE_SIGNALED_BIT);
        for( uint32_t i = 0; i != nFramesAhed; i++ )
        {
            _rts( vkCreateSemaphore( gfx.getDevice(), &semaphoreCI, _defAlloc, m_finishedSemas.emplace_back().set( gfx.getDevice() ) ), "ERROR failed to crate render finished semahore." );
            _rts( vkCreateFence( gfx.getDevice(), &fenceCI, _defAlloc, m_finishedFences.emplace_back().set( gfx.getDevice() ) ), "ERROR failed to crate render finished fence." );
        }
    }

   void Renderer::preRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
    {
        waitForFenceThrow( gfx.getDevice(), m_finishedFences[m_iFrame] ); // let the frame in-flight finish. We still might have m_finishedFences.size() - 1 frames in-flight
        const CommandBufferBeginInfo cmdBI( VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT );
        _rts( vkResetCommandBuffer( m_cbs[m_iFrame], 0 ), "vkResetCommandBuffer failed" );
        _rts( vkBeginCommandBuffer( m_cbs[m_iFrame], &cmdBI ), "beginCommandBuffer failed." );

        vkCmdBeginRenderPass(
            m_cbs[m_iFrame],
            &VK::RenderPassBeginInfo(
                m_renderPass,
                m_framebuffers[gfx.getRT().getCurrentBuffer()],
                { {0,0}, VK::to_2D( gfx.getRT().getExtent() ) },
                { VkClearValue{ VK::RenderTarget::s_sky }, VkClearValue{ .depthStencil{ VK::RenderTarget::s_clearDepth } } }
            ),
            VK_SUBPASS_CONTENTS_INLINE
        );
        vkCmdBindPipeline( m_cbs[m_iFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );
        auto vp = gfx.getRT().getPipelineViewportStateCreateInfo();
        vkCmdSetViewport( m_cbs[m_iFrame], 0, vp.viewportCount, vp.pViewports );
        vkCmdSetScissor( m_cbs[m_iFrame], 0, vp.scissorCount, vp.pScissors );
    }

    void Renderer::render( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
    {
    }

    void Renderer::postRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
    {
        vkCmdEndRenderPass( m_cbs[m_iFrame] );
        _rts( vkEndCommandBuffer( m_cbs[m_iFrame] ), "endCommandBuffer failed." );


        gfx.submit( { m_cbs[m_iFrame] }, 0, { gfx.getRT().getCurrentImageAquire() }, { m_finishedSemas[m_iFrame] } );

        // increment current cb
        m_iFrame++;
        if( m_iFrame == m_cbs.size() )
            m_iFrame = 0;
    }

    //--------------------------------------------------------------------------------------
    // GFXPipeline
    //--------------------------------------------------------------------------------------

    GFX::GFX( int32_t iGPU, char const* name, VkOffset2D const& windowCoord, bool validate )
    : m_validation(false)
    , m_frame(0)

    {
        mat4x4_identity( m_mView );
        mat4x4_perspective( m_mProjection, float(M_PI) / 2, 16.0f / 9, 0.25f, 1024.25f );

        // Look for validation layers
        if( validate ) {

            m_enabledLayers = filter( vectorize( vkEnumerateInstanceLayerProperties ), &VkLayerProperties::layerName, array{ "VK_LAYER_KHRONOS_validation" } );
            m_validation = !m_enabledLayers.empty();

            _rtc( m_validation, "vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
                    "Please look at the Getting Started guide for additional information.\n"
                    "GFXPipeline Failure" );
        }

        {
            m_enabledInstanceExtensions = filter( vectorize<VkExtensionProperties,const char*>( vkEnumerateInstanceExtensionProperties, (const char*)nullptr ), &VkExtensionProperties::extensionName, array {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
                VK_KHR_DISPLAY_EXTENSION_NAME,
                VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME,
                VK_KHR_GET_DISPLAY_PROPERTIES_2_EXTENSION_NAME,
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME
            } );

            _rtc( m_enabledInstanceExtensions.size() >= 5,
                    "enumerateInstanceExtensionProperties failed to find required extensions.\n\n"
                    "Please look at the Getting Started guide for additional information.\n"
                    "GFXPipeline Failure" );

            if( !validate ) // remove debug stuff
            {
                for( auto it = m_enabledInstanceExtensions.begin(); it != m_enabledInstanceExtensions.end(); )
                {
                    if( 0 == _stricmp( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, *it ) ||
                        0 == _stricmp( VK_EXT_DEBUG_REPORT_EXTENSION_NAME, *it ) )
                        it = m_enabledInstanceExtensions.erase( it );
                }
            }
        }

        // create instance
        {
            auto const app = VkApplicationInfo{
                VK_STRUCTURE_TYPE_APPLICATION_INFO,
                nullptr,
                name,
                0,
                "VIOSO_VK",
                VK_API_VERSION_1_0
            };
            auto const inst_info = VkInstanceCreateInfo{
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                nullptr,
                0,
                &app,
                (uint32_t)m_enabledLayers.size(),
                m_enabledLayers.data(),
                (uint32_t)m_enabledInstanceExtensions.size(),
                m_enabledInstanceExtensions.data()
            };
            auto result = vkCreateInstance( &inst_info, _defAlloc, m_inst.set() );

            if( result == VK_ERROR_INCOMPATIBLE_DRIVER ) {
                throw runtime_error(
                    "Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
                    "Please look at the Getting Started guide for additional information.\n"
                    "GFXPipeline Failure" );
            }
            else if( result == VK_ERROR_EXTENSION_NOT_PRESENT ) {
                throw runtime_error(
                    "Cannot find a specified extension library.\n"
                    "Make sure your layers path is set appropriately.\n"
                    "GFXPipeline Failure" );
            }
            else if( result != VK_SUCCESS ) {
                throw runtime_error(
                    "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                    "Please look at the Getting Started guide for additional information.\n"
                    "GFXPipeline Failure" );
            }
        }



        // determine GPU
        {
            auto gpus = vectorize<VkPhysicalDevice,VkInstance>( vkEnumeratePhysicalDevices, m_inst );
            _rtc( int(gpus.size()) > iGPU,
                    "Requested GPU not found?\n"
                    "GFXPipeline Failure" );
            decltype(gpus) matches;
            if( iGPU == -1 )
            { 
                // try to assign to display, if desktop-coordinate is set
                if( windowCoord.x != INT_MAX )
                {
                    for( int i = 0; i != gpus.size(); i++ )
                    {
                        auto& gpu = gpus[i];

                        //VkDisplayKHR disp;
                        //vkGetWinrtDisplayNV( gpu, 0, &disp );
                        //if( 0 != disp )
                        //    vkReleaseDisplayEXT( gpu, disp );
                        auto props1 = vectorize<VkDisplayPropertiesKHR, VkPhysicalDevice>( vkGetPhysicalDeviceDisplayPropertiesKHR, gpu );
                        auto props2 = vectorizeI<VkDisplayProperties2KHR, VkPhysicalDevice>( vkGetPhysicalDeviceDisplayProperties2KHR, gpu, DisplayProperties2KHR() );
                        auto props3 = vectorizeI<VkDisplayPlaneProperties2KHR, VkPhysicalDevice>( vkGetPhysicalDeviceDisplayPlaneProperties2KHR, gpu, DisplayPlaneProperties2KHR() );
                        if( props3.size() )
                        {
                            // it seems all of these do not work on PC, also VkDisplayPropertiesKHR does not contain desktop offset coordinates
                            // seems like we have to go NV/AMD/Intel API way to determine which GPU should be used
                            // 
                            // todo check position
                            // matches.push_back( gpu );
                        }
                    }
                }
                // hack, fill all GPUs to match, if display assignment failed
                if( matches.empty() )
                {
                    matches = gpus;
                }
                // filter

            }
            else
            {
                matches.push_back( gpus[iGPU] );
            }

            // try for dedicated GPU with biggest default heap
            {
                decltype( matches )::value_type bestMatch;
                size_t maxheapSize = 0;
                for( auto& match : matches )
                {
                    vkGetPhysicalDeviceProperties2( match, &m_props );
                    if( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == m_props.properties.deviceType )
                    {
                        vkGetPhysicalDeviceMemoryProperties2( match, &m_memProps );
                        if( maxheapSize < m_memProps.memoryProperties.memoryHeaps[0].size )
                        {
                            maxheapSize = (size_t)m_memProps.memoryProperties.memoryHeaps[0].size;
                            bestMatch = match;
                        }
                    }
                }
                if( 0 != maxheapSize )
                    m_gpu.reset( bestMatch );
            }

            if( !m_gpu )
            {
                // try for integrated GPU
                for( auto& match : matches )
                {
                    vkGetPhysicalDeviceProperties2( match, &m_props );
                    if( VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == m_props.properties.deviceType )
                    {
                        m_gpu.reset( match );
                        break;
                    }
                }
            }

            _rtc( (bool)m_gpu, "Could not find a dedicated or integrated GPU." );

            for( int i = 0; i != gpus.size(); i++ )
            {
                if( m_gpu == gpus[i] )
                {
                    iGPU = i;
                    break;
                }
            }

            vkGetPhysicalDeviceMemoryProperties2( m_gpu, &m_memProps );

            logStr( (string( "GPU " ) + to_string( iGPU ) + " \"" + m_props.properties.deviceName + "\" type: " + to_string( m_props.properties.deviceType ) + " RAM: " + to_string(m_memProps.memoryProperties.memoryHeaps[0].size) + "\n" ).c_str());
        }

    }

    GFX::~GFX()
    {
    }


    uint32_t GFX::findMemTypeFromProps( uint32_t typeBits, VkMemoryPropertyFlags requirementMask ) const
    {
        for( uint32_t i = 0; i != VK_MAX_MEMORY_TYPES; i++ )
        {
            if( typeBits & 1 )
            {
                if( m_memProps.memoryProperties.memoryTypes[i].propertyFlags & requirementMask )
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }
        return -1;
    }

    bool GFX::instanceCan( const char* extensionName ) const
    {
        return !filter( vectorize<VkExtensionProperties, char const*>( vkEnumerateInstanceExtensionProperties, nullptr ), &VkExtensionProperties::extensionName, to_array<char const*,1>( &extensionName ) ).empty();
    }

    bool GFX::gpuCan( const char* extensionName ) const
    {
        if( m_gpu )
        {
            return !filter( vectorize<VkExtensionProperties, VkPhysicalDevice, char const*>( vkEnumerateDeviceExtensionProperties, m_gpu, nullptr ), &VkExtensionProperties::extensionName, to_array<char const*, 1>( &extensionName ) ).empty();
        }
        return false;
    }

    CommandBufferH GFX::createCommandBuffer() const
    {
        return createCommandBuffers( 1 )[0];
    }

    vector<CommandBufferH> GFX::createCommandBuffers( uint32_t n ) const
    {
        const VkCommandBufferAllocateInfo cmdAI{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            nullptr,
            m_cp,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            n
        };
        vector<VkCommandBuffer> raw( n,0 );
        _rts( vkAllocateCommandBuffers( m_dev, &cmdAI, raw.data() ), "ERROR failed to crate command buffer." );

        vector<CommandBufferH> cbs( n, 0 );
        for( size_t i = 0; i != n; i++ )
            *cbs[i].set( { m_dev, m_cp } ) = raw[i];
        return cbs;
    }

    VkResult GFX::submitBufferCopy( GPUBuffer const& src, GPUBuffer const& dst, VkDeviceSize size ) const
    {
        CommandBufferH cb = createCommandBuffer();
        CommandBufferBeginInfo bi;
        vkBeginCommandBuffer( cb, &bi );
        VkBufferCopy bc{ 0,0,size };
        vkCmdCopyBuffer( cb, (BufferH)src, (BufferH)dst, 1, &bc );
        vkEndCommandBuffer( cb );
        FenceH f; vkCreateFence( m_dev, &FenceCreateInfo(), _defAlloc, f.set( m_dev ) );
        VkResult res = submit( { cb }, f );
        waitForFenceThrow( m_dev, f );
        return res;
    }

    VkResult GFX::submitImageCopy( Image const& src, Image const& dst, VkDeviceSize size ) const
    {
        CommandBufferH cb = createCommandBuffer();
        CommandBufferBeginInfo bi;
        vkBeginCommandBuffer( cb, &bi );
        // BIG TODO
        //VkImageCopy ic{ 0,0,size };
        //vkCmdCopyBuffer( cb, (BufferH)src, (BufferH)dst, 1, &bc );
        vkEndCommandBuffer( cb );
        return submit( { cb } );
    }

    VkResult GFX::submit( 
        vector<CommandBufferH> const& cbs,
        FenceH const& f,
        vector<SemaphoreH> const& wait,
        vector<SemaphoreH> const& signal ) const
    {
        vector<VkSemaphore> rawWaits( wait.begin(), wait.end() );
        vector<VkSemaphore> rawSignals( signal.begin(), signal.end() );
        vector<VkCommandBuffer> rawCBs( cbs.begin(), cbs.end() );
        SubmitInfo sI;
        vector<VkPipelineStageFlags> sf( wait.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT );
        if( !wait.empty() )
        {
            sI.waitSemaphoreCount = uint32_t( wait.size() );
            sI.pWaitSemaphores = rawWaits.data();
            sI.pWaitDstStageMask = sf.data();
        }
        if( !signal.empty() )
        {
            sI.signalSemaphoreCount = uint32_t( signal.size() );
            sI.pSignalSemaphores = rawSignals.data();
        }
        if( !cbs.empty() )
        {
            sI.commandBufferCount = uint32_t( cbs.size() );
            sI.pCommandBuffers = rawCBs.data();
        }
        return vkQueueSubmit( m_cq, 1, &sI, f );
    }

    ShaderModule GFX::createVSPassthrough() const
    {
        return ShaderModule( m_dev, (uint32_t const*)passthroughVS_bytecode, sizeof( passthroughVS_bytecode ) );
    }

    ShaderModule GFX::createFSPassthrough() const
    {
        return ShaderModule( m_dev, (uint32_t const*)passthroughFS_bytecode, sizeof( passthroughFS_bytecode ), VK_SHADER_STAGE_FRAGMENT_BIT );
    }

    void GFX::addRenderer( std::shared_ptr<Renderer> renderer )
    {
        m_renderers.push_back( renderer );
    }

    void GFX::preRender( mat4x4 const& world )
    {
        // acquire new buffer
        m_rt->getNextBuffer();
        uint32_t imageIndex = m_rt->getCurrentIndex();

        for( auto& renderer : m_renderers )
            renderer->preRender( *this, world, m_mView, m_mProjection );
    }

    void GFX::render( mat4x4 const& world )
    {
        for( auto& renderer : m_renderers )
            renderer->render( *this, world, m_mView, m_mProjection );
    }

    void GFX::postRender( mat4x4 const& world )
    {
        for( auto& renderer : m_renderers )
            renderer->postRender( *this, world, m_mView, m_mProjection );
        m_rt->finish();
        m_frame++;
    }

    //--------------------------------------------------------------------------------------
    // OutputWindow
    //--------------------------------------------------------------------------------------
    uint16_t s_wndclass = 0;

    LRESULT CALLBACK  OutputWindow::_WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        OutputWindow* that = (OutputWindow*)GetWindowLongPtr( hWnd, GWLP_USERDATA );
        if( that )
            return that->wndProc( reinterpret_cast<intptr_t>(hWnd), msg, wParam, lParam );
        return DefWindowProc( hWnd, msg, wParam, lParam );
    }

    LRESULT OutputWindow::wndProc( intptr_t xWnd, uint32_t msg, intptr_t wParam, intptr_t lParam )
    {
        HWND& hWnd = *(HWND*)&xWnd;
        PAINTSTRUCT ps;
        HDC hdc;

        switch( msg )
        {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        case WM_CHAR:
            if( 27 == wParam )
                PostQuitMessage( 0 );
            break;
        //case WM_SIZE: // note: an exception is raised anyway, we handle reconstruction of swapchain and backuffer there
        //    if( m_dev && m_sc && m_rt )
        //    {
        //        UINT width = LOWORD( lParam );
        //        UINT height = HIWORD( lParam );

        //        m_rt->resize( width, height, m_dev, m_sc ); // TODO
        //    }
        //    break;
        default:
            return DefWindowProc( hWnd, msg, wParam, lParam );
        }

        return 0;
    }

    OutputWindow::OutputWindow( HINSTANCE hInstance, const char* windowName, int x, int y, int width, int height, int nCmdShow, VkPresentModeKHR presentMode, uint32_t bufferCount, VkCompositeAlphaFlagBitsKHR alphaMode, bool withDepth, int iGPU, bool validate )
        : GFX( iGPU, windowName, { x, y }, validate )
        , m_hWnd( 0 )
        , m_preTransform( VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
        , m_colorSpace( VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        , m_alphaMode( alphaMode )
        , m_presentMode( presentMode )
      {
        _rtc( gpuCan( VK_KHR_SWAPCHAIN_EXTENSION_NAME ), "ERROR: GPU does not support swapchain." );
        m_enabledDeviceExtensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
        //m_enabledDeviceExtensions.push_back( "VK_KHR_portability_subset" ); // try for portability

        if( !s_wndclass )
        {
            // Register class
            WNDCLASSEXA wcex = {
                sizeof( WNDCLASSEX ),
                CS_HREDRAW | CS_VREDRAW,
                _WndProc,
                0,0,
                hInstance,
                LoadIconA( hInstance, "khronos_icon_132269.ico" ),
                LoadCursor( NULL, IDC_ARROW ),
                (HBRUSH)( COLOR_WINDOW + 1 ),
                NULL,
                "OutputWindowClass",
                LoadIconA( hInstance, "khronos_icon_132269.ico" )
            };
            s_wndclass = RegisterClassEx( &wcex );
            if( !s_wndclass )
                throw exception( "failed to register window class" );
        }

        // Create window
        DWORD dwExStyle = WS_EX_APPWINDOW;								// Window Extended Style
        DWORD dwStyle = WS_POPUP;										// Windows Style
        RECT rc = { x, y, x + width, y + height };
        if( 0 == width )
        {
            MONITORINFO mi = { 0 }; mi.cbSize = sizeof( mi );
            const POINT _p0 = { x,y };
            if( GetMonitorInfo( MonitorFromPoint( _p0, MONITOR_DEFAULTTONULL ), &mi ) )
            {
                rc = mi.rcMonitor;
                width = rc.right - rc.left;
                height = rc.bottom - rc.top;
            }
            AdjustWindowRectEx( &rc, dwStyle, FALSE, dwExStyle );
        }
        m_hWnd = reinterpret_cast<intptr_t>( CreateWindowExA(
            dwExStyle, "OutputWindowClass", windowName, dwStyle,
            rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            NULL, NULL, hInstance, NULL ) );
        _rtc( 0 != m_hWnd, "failed to create window" );

        SetWindowLongPtr( reinterpret_cast<HWND>(m_hWnd), GWLP_USERDATA, (LONG_PTR)this );

        ShowWindow( reinterpret_cast<HWND>( m_hWnd ), nCmdShow );

        // create surface
        {
            auto const createInfo = VkWin32SurfaceCreateInfoKHR{
                VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                nullptr,
                0,
                hInstance,
                (HWND)m_hWnd
            };

            _rts( vkCreateWin32SurfaceKHR( m_inst, &createInfo, _defAlloc, m_surface.set(m_inst) ), "ERROR: Could not create surface." );
        }

        // create swapchain
        uint32_t iQueueFamily = 0;
        {
            auto qProps = vectorizeV<VkQueueFamilyProperties, VkPhysicalDevice>( vkGetPhysicalDeviceQueueFamilyProperties, m_gpu );
            // try to find a queue that can render and swap
            // TODO if that fails, we need to find a combination, i. e, dedicated GPU renders and integrated swaps
            for( uint32_t iE = (uint32_t)qProps.size(); iQueueFamily != iE; iQueueFamily++ )
            {
               VkBool32 canPresent;

               if( VK_SUCCESS == vkGetPhysicalDeviceSurfaceSupportKHR( m_gpu, iQueueFamily, m_surface, &canPresent ) &&
                   canPresent && 
                   ( qProps[iQueueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT ) &&
                   ( qProps[iQueueFamily].queueFlags & VK_QUEUE_TRANSFER_BIT ) )
                    break;
            }
            _rtc( qProps.size() != iQueueFamily, "ERROR: Surface has no queue family, that can render and swap." );

            float const priorities[1] = { 1.0f };

            VkDeviceQueueCreateInfo queues[]{ {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, iQueueFamily, 1, priorities } };

            VkDeviceCreateInfo deviceInfo{
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                nullptr,
                0,
                1,
                queues,
                (uint32_t)m_enabledLayers.size(),
                m_enabledLayers.data(),
                (uint32_t)m_enabledDeviceExtensions.size(),
                m_enabledDeviceExtensions.data(),
                nullptr
            };

            _rts( vkCreateDevice( m_gpu, &deviceInfo, _defAlloc, m_dev.set() ), "ERROR: failed to create device." );

            vkGetDeviceQueue( m_dev, iQueueFamily, 0, m_cq.set() );

        }
    
        VkPhysicalDeviceSurfaceInfo2KHR pdsI{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR, nullptr, m_surface };
        auto surfaceFormats = vectorizeI<VkSurfaceFormat2KHR, VkPhysicalDevice, VkPhysicalDeviceSurfaceInfo2KHR const*>( vkGetPhysicalDeviceSurfaceFormats2KHR, m_gpu, &pdsI, VkSurfaceFormat2KHR{ VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR, nullptr } );
        _rtc( !surfaceFormats.empty(), "ERROR: failed to create device." );

        VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

        if( !surfaceFormats.empty() && VK_FORMAT_UNDEFINED != surfaceFormats.front().surfaceFormat.format )
        {
            format = surfaceFormats.front().surfaceFormat.format; // use preferred, if present
            m_colorSpace = surfaceFormats.front().surfaceFormat.colorSpace;
        }

        // create command pool
        {
            _rts( vkCreateCommandPool( m_dev, &CommandPoolCreateInfo( VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, iQueueFamily), _defAlloc, m_cp.set( m_dev ) ), "ERROR failed to crate command pool." );
        }

        // prepare_swapchain
        {
            auto oldSwapchain = m_sc;

            // Check the surface capabilities and formats
            SurfaceCapabilities2KHR surfCaps;
            _rts( vkGetPhysicalDeviceSurfaceCapabilities2KHR( m_gpu, &pdsI, &surfCaps ), "ERROR: failed to getSurfaceCapabilities." );

            auto presentModes = vectorize<VkPresentModeKHR, VkPhysicalDevice, VkSurfaceKHR>( vkGetPhysicalDeviceSurfacePresentModesKHR, m_gpu, m_surface );
            _rtc( presentModes.end() != find( presentModes.begin(), presentModes.end(), m_presentMode ), "ERROR: falied to getSurfacePresentModesKHR or unsupported present mode." );

            VkExtent2D swapchainExtent;
            // width and height are either both -1, or both not -1.
            if( surfCaps.surfaceCapabilities.currentExtent.width == (uint32_t)-1 ) {
                // If the surface size is undefined, the size is set to
                // the size of the images requested.
                swapchainExtent.width = width;
                swapchainExtent.height = height;
            }
            else {
                // If the surface size is defined, the swap chain size must match
                swapchainExtent = surfCaps.surfaceCapabilities.currentExtent;
                _rtc( width == surfCaps.surfaceCapabilities.currentExtent.width, "ERROR: surface size mismatch." );
                _rtc( height == surfCaps.surfaceCapabilities.currentExtent.height, "ERROR: surface size mismatch." );
                width = swapchainExtent.width;
                height = swapchainExtent.height;
            }

            // Determine the number of VkImages to use in the swap chain.
            // Application desires to acquire 3 images at a time for triple
            // buffering
            if( bufferCount < surfCaps.surfaceCapabilities.minImageCount )
            {
                logStr( "WARNING: number of image buffers extended to capability" );
                bufferCount = surfCaps.surfaceCapabilities.minImageCount;
            }

            // If maxImageCount is 0, we can ask for as many images as we want,
            // otherwise
            // we're limited to maxImageCount
            if( surfCaps.surfaceCapabilities.maxImageCount != 0 &&
                bufferCount > surfCaps.surfaceCapabilities.maxImageCount )
            {
                logStr( "WARNING: number of image buffers reduced to capability" );
                bufferCount = surfCaps.surfaceCapabilities.maxImageCount;
            }

            if( surfCaps.surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) {
                m_preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            }
            else {
                m_preTransform = surfCaps.surfaceCapabilities.currentTransform;
            }

            _rtc( 0 != ( m_alphaMode & surfCaps.surfaceCapabilities.supportedCompositeAlpha ), "compositeAlpha mode not supported." );

            const VkSwapchainCreateInfoKHR swapchainCI{
                VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                nullptr,
                0,
                m_surface,
                bufferCount,
                format,
                m_colorSpace,
                { swapchainExtent.width, swapchainExtent.height },
                1,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                0,
                nullptr,
                m_preTransform,
                m_alphaMode,
                m_presentMode,
                true,
                oldSwapchain
            };

            _rts( vkCreateSwapchainKHR( m_dev, &swapchainCI, nullptr, m_sc.set( m_dev ) ), "could not create swapchain." );

        }

        m_rt = make_unique<BackBuffer>(*this, m_sc, format, VkExtent2D{ uint32_t(width), uint32_t(height) }, withDepth ? VK_FORMAT_D16_UNORM : VK_FORMAT_UNDEFINED);

        mat4x4_frustum( m_mProjection, -0.5f, 0.5f, -0.5f * float(height) / float(width), 0.5f * float(height) / float(width), 0.25f, 1024.25f );
   }

    void OutputWindow::preRender( mat4x4 const& world )
    {
        __super::preRender( world );
    }


    void OutputWindow::postRender( mat4x4 const& world )
    {
        __super::postRender( world );
        // 
        PresentInfoKHR pI;
        // get all finish semaphores
        vector< VkSemaphore > fs; fs.reserve( m_renderers.size() );
        for( auto& renderer : m_renderers )
        {
            SemaphoreH const& h = renderer.get()->getFinishSignal();
            if( h )
                fs.emplace_back( h.hnd );
        }
        pI.waitSemaphoreCount = uint32_t( fs.size() );
        pI.pWaitSemaphores = fs.data();
        pI.swapchainCount = 1;
        pI.pSwapchains = &m_sc.hnd;
        uint32_t ii = m_rt->getCurrentIndex();
        pI.pImageIndices = &ii;

        VkResult result = vkQueuePresentKHR( m_cq, &pI );
    }

};