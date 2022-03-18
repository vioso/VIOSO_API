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

// do not move to header, these are too short names not to clash, rather do copy-paste, if you need in another module
#define RTCR( res, exp, msg ) if( res != (exp) )\
        throw runtime_error( "RUNTIME_ERROR: "##msg )

#define RTCS( exp, msg ) if( VK_SUCCESS != (exp) )\
        throw runtime_error( "RUNTIME_ERROR: "##msg )

#define RTC( exp, msg ) if( !(exp)  )\
        throw runtime_error( "RUNTIME_ERROR: "##msg )

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
    {
        m_ci = ci;
        if( ci.extent.width * ci.extent.height * ci.extent.depth )
        {
            RTCS( vkCreateImage( gfx.getDevice(), &ci, _defAlloc, m_image.set( gfx.getDevice() ) ), "Failed to create image." );

            VkMemoryRequirements mem_reqs;
            vkGetImageMemoryRequirements( gfx.getDevice(), m_image, &mem_reqs );

            VkMemoryAllocateInfo mem_alloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0 };
            mem_alloc.allocationSize = mem_reqs.size;
            mem_alloc.memoryTypeIndex = gfx.findMemTypeFromProps( mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            RTC( mem_alloc.memoryTypeIndex < VK_MAX_MEMORY_TYPES, "required memory type for depth buffer not available." );

            RTCS( vkAllocateMemory( gfx.getDevice(), &mem_alloc, _defAlloc, m_mem.set( gfx.getDevice() ) ), "failed to allocate image memory for depth buffer." );

            RTCS( vkBindImageMemory( gfx.getDevice(), m_image, m_mem, 0 ), "failed to bind image memory to depth buffer." );

            ImageViewCreateInfo ivCI(
                m_image,
                { VkImageAspectFlags( ( ci.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT ), 0, ci.mipLevels, 0, 1 },
                ci.format );
            RTCS( vkCreateImageView( gfx.getDevice(), &ivCI, _defAlloc, m_view.set( gfx.getDevice() ) ), "failed to create view for deth buffer." );
        }
    }


    //--------------------------------------------------------------------------------------
    // SwapChainImage
    //--------------------------------------------------------------------------------------
    SwapchainImage::SwapchainImage( GFX const& gfx, VkImage image, VkFormat format, uint32_t width, uint32_t height )
    {
        m_ci = ImageCreateInfo( width, height, format, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT );
        ImageViewCreateInfo ivCI( image, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, format );

        m_image.reset( image );

        RTCS( vkCreateImageView( gfx.getDevice(), &ivCI, _defAlloc, m_view.set( gfx.getDevice() ) ), "could not create image view." );
    }

    SwapchainImage::~SwapchainImage()
    {
    }

    //--------------------------------------------------------------------------------------
   // RenderTarget
   //--------------------------------------------------------------------------------------
    const VkClearValue RenderTarget::s_black{ { { 0.0f, 0.0f, 0.0f, 0.0f } } };
    const VkClearValue RenderTarget::s_sky{ { { 0.59f, 0.86f, 1.0f, 1.0f } } };

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
        RTCS( vkCreateBuffer( gfx.getDevice(), &ci, _defAlloc, m_hBuf.set( gfx.getDevice() ) ), "Failed to vkCreateBuffer.");

        VkMemoryRequirements mr;
        BufferMemoryRequirementsInfo2 mri( m_hBuf );
        vkGetBufferMemoryRequirements( gfx.getDevice(), m_hBuf, &mr );
   	    RTC( size <= mr.size, "Constant buffer GPU memory size differs." );

        MemoryAllocateInfo mi; mi.allocationSize = mr.size;
        mi.memoryTypeIndex = -1;
        RTC( -1 != ( mi.memoryTypeIndex = gfx.findMemTypeFromProps( mr.memoryTypeBits, memFlags ) ), "Could not find requested GPU heap for buffer." );

        RTCS( vkAllocateMemory( gfx.getDevice(), &mi, _defAlloc, m_hMem.set( gfx.getDevice() ) ), "Could not allocate GPU memory for buffer." );

        RTCS( vkBindBufferMemory( gfx.getDevice(), m_hBuf, m_hMem, 0 ), "Failed to bind GPU memory to buffer.");
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
    BackBuffer::BackBuffer( GFX const& gfx, SwapchainKHRH const& sc, VkFormat format, VkExtent2D extent, VkFormat depthFormat )
        : RenderTarget( gfx.getDevice() )
        , m_sc( sc )
    {
        m_extent = VkExtent3D{ extent.width, extent.height, 1 };
        m_format = format;
        m_depthFormat = depthFormat;

        auto images = vectorize<VkImage, VkDevice, VkSwapchainKHR>( vkGetSwapchainImagesKHR, gfx.getDevice(), sc );

        RTC( !images.empty(), "could not get images from swapchain." );

        m_images.clear();

        SemaphoreCreateInfo scI;
        for( auto& image : images ) {
            m_images.emplace_back( make_unique<SwapchainImage>( gfx, image, m_format, extent.width, extent.height ) );
            RTCR( VK_SUCCESS, vkCreateSemaphore( gfx.getDevice(), &scI, _defAlloc, m_presentComplete.emplace_back().set( gfx.getDevice() ) ), "ERROR failed to crate image acquire semahore." );

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
        VkResult res = vkAcquireNextImageKHR( m_dev, m_sc, UINT64_MAX, m_presentComplete[m_currentBuffer], 0, (uint32_t*)&m_currentBuffer._Storage );
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
    VkPipelineVertexInputStateCreateInfo Renderer::VertexWTex::getInputLayout()
    {
        static const VkVertexInputBindingDescription _b[] =
        {
            VkVertexInputBindingDescription{ 0, sizeof( VertexWTex ), VK_VERTEX_INPUT_RATE_VERTEX }
        };
        static const VkVertexInputAttributeDescription _a[] =
        {
            VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexWTex, x ) },
            VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof( VertexWTex, u ) }
        };

        return PipelineVertexInputStateCreateInfo( _b, _a );
    }

    VkPipelineVertexInputStateCreateInfo Renderer::VertexWColTex::getInputLayout()
    {
        static const VkVertexInputBindingDescription _b[] =
        {
            VkVertexInputBindingDescription{ 0, sizeof( VertexWColTex ), VK_VERTEX_INPUT_RATE_VERTEX }
        };
        static const VkVertexInputAttributeDescription _a[] =
        {
            VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexWColTex, x ) },
            VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof( VertexWColTex, r ) },
            VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof( VertexWColTex, u ) }
        };
        return PipelineVertexInputStateCreateInfo( _b, _a );
    }

    Renderer::Renderer( 
        GFX const& gfx,
        std::vector<std::unique_ptr<Image>> const& attachments,
        AttachmentDescription const& desc,
        std::vector<ShaderModule> const& shaderStages,
        VkPipelineVertexInputStateCreateInfo const& inputLayout,
        bool withSignal )
    : m_id( ++s_freeID )
    , m_dev( gfx.getDevice() )
    , m_cbs( {gfx.createCommandBuffer()})
    , m_sss( shaderStages )
    {
        assert( !attachments.empty() );
        
         _rts( vkCreateRenderPass( 
            gfx.getDevice(),
            &RenderPassCreateInfo<1, 1, 0>( 
                { desc },
                { SubpassDescription<0, 1>( {}, { AttachmentReference() } ) } ),
            _defAlloc, 
            m_renderPass.set( gfx.getDevice() )
        ), "Failed to create renderPass." );

        _rts( vkCreateDescriptorSetLayout(
            gfx.getDevice(),
            &DescriptorSetLayoutCreateInfo( { DescriptorSetLayoutBinding() } ),
            _defAlloc,
            m_descriptorSetLayout.set( gfx.getDevice() )
        ), "failed to vkCreateDescriptorSetLayout." );

        _rts( vkCreatePipelineLayout(
            gfx.getDevice(),
            &PipelineLayoutCreateInfo<1>( { m_descriptorSetLayout } ),
            _defAlloc,
            m_pipelineLayout.set( gfx.getDevice() )
        ), "failed to vkCreatePipelineLayout." );

        vector< VkPipelineShaderStageCreateInfo > pssCIs;
        for( auto& sm : m_sss )
            pssCIs.push_back( sm.getCI() );

        _rts( vkCreateGraphicsPipelines( 
            gfx.getDevice(), 0, 1,
            &GraphicsPipelineCreateInfo(
                m_renderPass, 0, m_pipelineLayout,
                uint32_t( pssCIs.size() ), pssCIs.data(),
                &inputLayout, &PipelineInputAssemblyStateCreateInfo(),
                nullptr,
                &attachments.front()->getPipelineViewportStateCreateInfo(),
                &PipelineRasterizationStateCreateInfo(),
                &PipelineMultisampleStateCreateInfo(),
                nullptr,
                &PipelineColorBlendStateCreateInfo( { PipelineColorBlendAttachmentState( false ) } ),
                &PipelineDynamicStateCreateInfo<0>()
            ),
            _defAlloc, m_pipeline.set( gfx.getDevice() )
        ), "Failed to create pipeline for Cubes Renderer." );

        for( auto& image : attachments )
        {
            _rts( vkCreateFramebuffer(
                m_dev,
                &FramebufferCreateInfo<1>(
                    m_renderPass,
                    { *image },
                    attachments.front()->getCI().extent.width, attachments.front()->getCI().extent.height, attachments.front()->getCI().extent.depth ),
                _defAlloc,
                m_framebuffers[*image].set( m_dev )
            ), "failed to create framebuffer." );
        }

        if( withSignal )
        {
            const SemaphoreCreateInfo semaphoreCI;
            _rts( vkCreateSemaphore( gfx.getDevice(), &semaphoreCI, _defAlloc, m_sema.set( gfx.getDevice() ) ), "ERROR failed to crate render complete semahore." );
        }

    }

    VkResult Renderer::submit( GFX const& gfx, std::vector<SemaphoreH> const wait )
    {
        if( m_sema )
            return gfx.submit( m_cbs, 0, wait, { m_sema } );
        else
            return gfx.submit( m_cbs, 0, wait, {} );
    }

    void Renderer::preRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
    {
        const CommandBufferBeginInfo cmdBI( VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT );
        for( auto& cb : m_cbs )
        {
            vkResetCommandBuffer( cb, 0 );
            _rts( vkBeginCommandBuffer( m_cbs[0], &cmdBI ), "beginCommandBuffer failed." );
        }

        vkCmdBeginRenderPass(
            m_cbs[0],
            &VK::RenderPassBeginInfo(
                m_renderPass,
                m_framebuffers[gfx.getRT().getCurrentBuffer()],
                { {0,0}, VK::ext3Dto2D( gfx.getRT().getExtent() ) },
                { VK::RenderTarget::s_sky }
            ),
            VK_SUBPASS_CONTENTS_INLINE
        );
        vkCmdBindPipeline( m_cbs[0], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );
    }

    void Renderer::render( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
    {
    }

    void Renderer::postRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
    {
        for( auto& cb : m_cbs )
            _rts( vkEndCommandBuffer( cb ), "endCommandBuffer failed." );

        gfx.submit( m_cbs );
    }

    SemaphoreH const& Renderer::getFinishSignal() const
    {
	    return m_sema;
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

            RTC( m_validation, "vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
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

            RTC( m_enabledInstanceExtensions.size() >= 5,
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
            RTC( int(gpus.size()) > iGPU,
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

            RTC( m_gpu, "Could not find a dedicated or integrated GPU." );

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
        RTCS( vkAllocateCommandBuffers( m_dev, &cmdAI, raw.data() ), "ERROR failed to crate command buffer." );

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
        RTC( gpuCan( VK_KHR_SWAPCHAIN_EXTENSION_NAME ), "ERROR: GPU does not support swapchain." );
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
        RTC( m_hWnd, "failed to create window" );

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

            RTCR( VK_SUCCESS, vkCreateWin32SurfaceKHR( m_inst, &createInfo, _defAlloc, m_surface.set(m_inst) ), "ERROR: Could not create surface." );
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
            RTC( qProps.size() != iQueueFamily, "ERROR: Surface has no queue family, that can render and swap." );

            float const priorities[1] = { 0.0 };

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

            RTCR( VK_SUCCESS, vkCreateDevice( m_gpu, &deviceInfo, _defAlloc, m_dev.set() ), "ERROR: failed to create device." );

            vkGetDeviceQueue( m_dev, iQueueFamily, 0, m_cq.set() );

        }
    
        VkPhysicalDeviceSurfaceInfo2KHR pdsI{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR, nullptr, m_surface };
        auto surfaceFormats = vectorizeI<VkSurfaceFormat2KHR, VkPhysicalDevice, VkPhysicalDeviceSurfaceInfo2KHR const*>( vkGetPhysicalDeviceSurfaceFormats2KHR, m_gpu, &pdsI, VkSurfaceFormat2KHR{ VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR, nullptr } );
        RTC( !surfaceFormats.empty(), "ERROR: failed to create device." );

        VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

        if( !surfaceFormats.empty() && VK_FORMAT_UNDEFINED != surfaceFormats.front().surfaceFormat.format )
        {
            format = surfaceFormats.front().surfaceFormat.format; // use preferred, if present
            m_colorSpace = surfaceFormats.front().surfaceFormat.colorSpace;
        }

        // create command pool
        {
            const VkCommandPoolCreateInfo cpI{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, 0, iQueueFamily };
            RTCR( VK_SUCCESS, vkCreateCommandPool( m_dev, &cpI, _defAlloc, m_cp.set( m_dev ) ), "ERROR failed to crate command pool." );
        }

        // prepare_swapchain
        {
            auto oldSwapchain = m_sc;

            // Check the surface capabilities and formats
            SurfaceCapabilities2KHR surfCaps;
            RTCR( VK_SUCCESS, vkGetPhysicalDeviceSurfaceCapabilities2KHR( m_gpu, &pdsI, &surfCaps ), "ERROR: failed to getSurfaceCapabilities." );

            auto presentModes = vectorize<VkPresentModeKHR, VkPhysicalDevice, VkSurfaceKHR>( vkGetPhysicalDeviceSurfacePresentModesKHR, m_gpu, m_surface );
            RTC( presentModes.end() != find( presentModes.begin(), presentModes.end(), m_presentMode ), "ERROR: falied to getSurfacePresentModesKHR or unsupported present mode." );

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
                RTC( width == surfCaps.surfaceCapabilities.currentExtent.width, "ERROR: surface size mismatch." );
                RTC( height == surfCaps.surfaceCapabilities.currentExtent.height, "ERROR: surface size mismatch." );
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

            RTC( ( m_alphaMode & surfCaps.surfaceCapabilities.supportedCompositeAlpha ), "compositeAlpha mode not supported." );

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

            RTCS( vkCreateSwapchainKHR( m_dev, &swapchainCI, nullptr, m_sc.set( m_dev ) ), "could not create swapchain." );

        }

        m_rt = make_unique<BackBuffer>( *this, m_sc, format, VkExtent2D{ uint32_t(width), uint32_t(height) } );

        mat4x4_frustum( m_mProjection, -0.5f, 0.5f, -0.5f * float(height) / float(width), 0.5f * float(height) / float(width), 0.25f, 1024.25f );
   }

    void OutputWindow::preRender( mat4x4 const& world )
    {
        uint32_t imageIndex;
        vkAcquireNextImageKHR( m_dev, m_sc, UINT64_MAX, m_rt.get()->getCurrentImageAquire(), VK_NULL_HANDLE, &imageIndex );
        __super::preRender( world );
    }


    void OutputWindow::postRender( mat4x4 const& world )
    {
        __super::postRender( world );
        // 
        PresentInfoKHR pI;
        // get last finish handle
        auto it = m_renderers.rbegin();
        while( m_renderers.rend() != it )
        {
            SemaphoreH const& h = it->get()->getFinishSignal();
            if( h )
            {
                pI.waitSemaphoreCount = 1;
                pI.pWaitSemaphores = &h.hnd;
                break;
            }
            it++;
        }
        pI.swapchainCount = 1;
        pI.pSwapchains = &m_sc.hnd;

        VkResult result = vkQueuePresentKHR( m_cq, &pI );
        m_frame++;
    }

};