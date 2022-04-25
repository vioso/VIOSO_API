#pragma once

// credits go to VULKAN TUTORIAL
// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_TYPESAFE_CONVERSION

#define VK_MAX_FRAME_LAG 2

#include "vulkan/vulkan.h"
#include "vulkan/vk_layer.h"
#include "linmath.h"
#include <array>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <cassert>
#include "VK_structures.h"
#include "VK_handles.h"

inline void logStr( const char* str )
{
	{
		FILE* f;
		{
			if( 0 == fopen_s( &f, "VIOSOWarpBlend.log", "a" ) )
			{
				fputs( str, f );
				fclose( f );
			}
		}
	}
}

inline mat4x4& cpy( mat4x4& target, mat4x4 const& source ) { memcpy( &target, &source, sizeof( target ) ); }

namespace VK
{
	template< class T >
	constexpr void _rtc( T const res, T const exp, char const* msg ) {
		if( res != exp )
			throw std::runtime_error( std::string("RUNTIME_ERROR: ") + msg );
	}

	constexpr VkResult _rts( VkResult const exp, char const* msg ) {
		if( VK_SUCCESS != exp )
			throw std::runtime_error( std::string("RUNTIME_ERROR: ") + msg );
		return exp;
	}

	bool test();

	class device_lost : public std::runtime_error {
	public:
		device_lost( std::string const& error ) : std::runtime_error( error ) {}
		device_lost( char const* error ) : std::runtime_error( error ) {}
	};

	class out_of_date : public std::runtime_error {
	public:
		out_of_date( std::string const& error ) : std::runtime_error( error ) {}
		out_of_date( char const* error ) : std::runtime_error( error ) {}
	};

	inline VkResult waitForFenceThrow( DeviceH const& dev, FenceH const& f )
	{
		VkResult res = vkWaitForFences( dev, 1, &f.hnd, VK_TRUE, -1 );
		if( VK_ERROR_DEVICE_LOST == res )
			throw device_lost( "Device lost while waiting for fence." );
		return res;
	}

	template< class T >
	std::vector< T > vectorize( VkResult( *fn )( uint32_t* cnt, T* p ) )
	{
		std::vector< T > res;
		uint32_t c = 0;
		if( VK_SUCCESS == fn( &c, NULL ) && c )
		{
			res.resize( c );
			fn( &c, res.data() );
		}
		return res;
	}

	template< class T, class... P >
	std::vector< T > vectorize( VkResult( *fn )( P... a, uint32_t* cnt, T* p ), P... args )
	{
		std::vector< T > res;
		uint32_t c = 0;
		VkResult s = fn( args..., &c, NULL );
		if( VK_SUCCESS == s && c )
		{
			res.resize( c );
			fn( args..., &c, res.data() );
		}
		return res;
	}

	// enumerator to list functions
	template< class T, class... P >
	std::vector< T > vectorizeI( VkResult( *fn )( P... a, uint32_t* cnt, T* p ), P... args, T const& init = T{} )
	{
		std::vector< T > res;
		uint32_t c = 0;
		VkResult s = fn( args..., &c, NULL );
		if( VK_SUCCESS == s && c )
		{
			res.resize( c );
			for( auto& r : res )
				r = init;
			fn( args..., &c, res.data() );
		}
		return res;
	}

	template< class T >
	std::vector< T > vectorizeV( void( *fn )( uint32_t* cnt, T* p ) )
	{
		std::vector< T > res;
		uint32_t c = 0;
		fn( &c, NULL );
		if( c )
		{
			res.resize( c );
			fn( &c, res.data() );
		}
		return res;
	}

	template< class T, class... P >
	std::vector< T > vectorizeV( void( *fn )( P... a, uint32_t* cnt, T* p ), P... args )
	{
		std::vector< T > res;
		uint32_t c = 0;
		fn( args..., &c, NULL );
		if( c )
		{
			res.resize( c );
			fn( args..., &c, res.data() );
		}
		return res;
	}

	template< class T1, class T2, class T3, size_t n >
	std::vector< T3 > filter( std::vector< T1 > const& exts, T2 T1::* attr, std::array<T3, n> filters ) {
		std::vector< T3 > res;
		for( auto const& f : filters )
		{
			for( auto const& ext : exts )
			{
				if( !strcmp( ext.*attr, f ) )
					res.push_back( f );
			}
		}
		return res;
	}

	constexpr VkExtent3D ext2Dto3D( VkExtent2D const& e2 ) { return VkExtent3D{ e2.width, e2.height, 1 }; }
	constexpr VkExtent2D ext3Dto2D( VkExtent3D const& e3 ) { return VkExtent2D{ e3.width, e3.height }; }

	/////////////////////////////////////////////////////////////////////////
	// High level

	///  Forward definitions
	class GFX;

	/// <summary>
	/// Image, base class for image buffers
	/// </summary>
	class Image {
	protected:
		ImageViewH m_view;
		SemaphoreH m_sema; // use this to avoide resource conflicts, this way we can wait for availability on a command
		ImageCreateInfo m_ci;
	public:
		Image() {}
		Image( Image const& ) = delete;
		Image( Image&& other ) noexcept :
			m_view( std::move( other.m_view )),
			m_sema( std::move( other.m_sema )),
			m_ci(   std::move( other.m_ci ))
		{}
		virtual ~Image();
		virtual operator VkImage const& () const = 0;
		//virtual ImageViewH const& getView() const { return m_view; }
		virtual operator VkImageView const& () const { return m_view; }
		virtual SemaphoreH const& getSema() const { return m_sema; }
		virtual ImageCreateInfo const& getCI() const { return m_ci; }
		PipelineViewportStateCreateInfo getPipelineViewportStateCreateInfo() const
		{
			return PipelineViewportStateCreateInfo(
				{ { float( 0 ), float( 0 ), float( m_ci.extent.width ), float( m_ci.extent.height ), float( 0 ), float( 1 ) }	},
				{ { int32_t( m_ci.extent.width ), int32_t( m_ci.extent.height ) } } 
			);
		}
	};

	/// <summary>
	/// TextureImage, some texture
	/// </summary>
	class TextureImage : public Image {
	protected:
		ImageH m_image;
		DeviceMemoryAH m_mem;
		ImageH m_staging;
	public:
		TextureImage(
			GFX const& gfx,
			VkImageCreateInfo const& ci = ImageCreateInfo(),
			VkMemoryAllocateFlags = ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) );
		TextureImage( TextureImage const&  ) = delete;
		TextureImage( TextureImage && other ) noexcept 
			: Image( std::move( other ))
			, m_image( std::move( other.m_image ))
			, m_mem( std::move( other.m_mem ))
			, m_staging( std::move( other.m_staging))
		{};
		virtual ~TextureImage() {};
		virtual operator VkImage const&() const { return m_image; }
		virtual DeviceMemoryAH const& getMem() const { return m_mem; }
	};

	/// <summary>
	/// SwapChainImage, derived image from a SwapChain
	/// </summary>
	class SwapchainImage : public Image {
	public:
		SCImageH m_image;
	public:
		SwapchainImage(
			GFX const& gfx,
			VkImage image,
			VkFormat format,
			uint32_t width,
			uint32_t height );
		SwapchainImage( SwapchainImage const& ) = delete;
		SwapchainImage( SwapchainImage&& other ) noexcept : 
			Image( std::move( other ) ),
			m_image( std::move( other.m_image ) )
		{}

		virtual ~SwapchainImage();
		virtual operator VkImage const&() const { return m_image; }
	};

	/// <summary>
	/// RenderTarget, base class for rendering into
	/// </summary>
	class RenderTarget
	{
	protected:
		std::vector< std::unique_ptr<Image> > m_images;
		std::vector<TextureImage> m_depthBuffers;
		std::vector<SemaphoreH> m_imageAcquire;

		DeviceWH m_dev;

		VkExtent3D m_extent;
		uint32_t m_mipLevels;
		VkFormat m_format;
		VkFormat m_depthFormat;

		std::atomic<uint32_t> m_currentBuffer;
	public:

		RenderTarget(DeviceH const& dev, std::vector<VkImage> const& images, VkExtent3D const& extent = { 0 }, VkFormat format = VK_FORMAT_UNDEFINED, VkFormat depthFormat = VK_FORMAT_UNDEFINED, uint32_t mipLevels = 1)
			: m_currentBuffer( 0 )
			, m_extent( extent )
			, m_mipLevels( mipLevels )
			, m_format( format )
			, m_depthFormat( depthFormat )
			, m_dev( dev )
		{}
		RenderTarget( RenderTarget const& ) = delete;
		RenderTarget( RenderTarget&& other ) noexcept
			: m_images( std::move( other.m_images ))
			, m_depthBuffers( std::move( other.m_depthBuffers ))
			, m_imageAcquire( std::move( other.m_imageAcquire ))

			, m_dev( std::move( other.m_dev ))

			, m_extent( other.m_extent )
			, m_mipLevels( other.m_mipLevels )
			, m_format( other.m_format )
			, m_depthFormat( other.m_depthFormat )

			, m_currentBuffer( m_currentBuffer.load() )
		{}

		static const VkClearValue s_black;
		static const VkClearValue s_sky;

		VkExtent3D const& getExtent() const { return m_extent; }
		uint32_t const& getMipLevels() const { return m_mipLevels; }
		VkFormat const& getColorFormat() const { return m_format; }
		VkFormat const& getDepthFormat() const { return m_depthFormat; }

		VkViewport getFullViewport() const {
			return VkViewport{ 0, 0, (float)m_extent.width, (float)m_extent.height, 0, 1 };
		}

		VkRect2D getFullScissorRect() const {
			return VkRect2D{ { 0, 0 }, { m_extent.width, m_extent.height } };
		};

		virtual void resize( int width, int height, DeviceH& dev, SwapchainKHRH& sc );

		virtual Image const& getNextBuffer()
		{
			assert( !m_images.empty() );
			m_currentBuffer++;
			if( uint32_t( m_images.size() ) <= m_currentBuffer )
				m_currentBuffer = 0;
			return *m_images[m_currentBuffer];
		}

		virtual Image const& getCurrentBuffer() const
		{
			assert( !m_images.empty() );
			return *m_images[m_currentBuffer];
		}

		uint32_t getCurrentIndex() const { return m_currentBuffer; }
		virtual VkAttachmentDescription getAttachmentDescription() const = 0;
		std::vector<std::unique_ptr<Image>> const& getAttachments() const { return m_images; }
		std::vector<TextureImage> const& getDepthBuffers() const { return m_depthBuffers; }

		SemaphoreH const& getCurrentImageAquire() {
			return m_imageAcquire[m_currentBuffer];
		}
	};

	/// <summary>
	/// A backbuffer; wraps SwapChainImages from a swapchain
	/// </summary>
	class BackBuffer : public RenderTarget
	{
		std::vector<SemaphoreH> m_presentComplete;
		SwapchainKHRWH m_sc;
	public:
		/// <summary>
		/// Contructor
		/// </summary>
		/// <param name="gfx"></param>
		/// <param name="sc"></param>
		/// <param name="hDepth"></param>
		BackBuffer(GFX const& gfx, SwapchainKHRH const& sc, VkFormat format, VkExtent2D extent, VkFormat depthFormat = VK_FORMAT_D16_UNORM);
		virtual Image const& getNextBuffer();
		virtual VkAttachmentDescription getAttachmentDescription() const {
			return AttachmentDescription( m_format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
		};
	};

	/// <summary>
	/// RenderTexture, wraps a TextureImage 
	/// !!STUB!!
	/// </summary>
	class RenderTexture : public RenderTarget
	{
	public:
		RenderTexture(DeviceH const& dev, std::vector<VkImage> const& images) : RenderTarget(dev, images) {}
		virtual VkAttachmentDescription getkAttachmentDescription() const {
			return AttachmentDescription( m_format );
		};
	};

	class GPUBuffer
	{
	protected:
		BufferH m_hBuf;
		DeviceMemoryAH m_hMem;
	public:
		GPUBuffer( GFX const& gfx, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE );

		GPUBuffer( GPUBuffer const& ) = delete;
		GPUBuffer( GPUBuffer&& other ) noexcept :
			m_hBuf( std::move( other.m_hBuf ) ),
			m_hMem( std::move( other.m_hMem ) )
		{
		}

		void* map()
		{
			void* mapped = nullptr;
			DeviceH dev = get<0>( m_hBuf.getArgs() );
			if( !dev || VK_SUCCESS !=  vkMapMemory( dev, m_hMem, 0, VK_WHOLE_SIZE, 0, &mapped ) )
				throw std::runtime_error( "Failed to map buffer" );
			return mapped;
		}

		void unmap() noexcept
		{
			if( m_hMem && m_hBuf )
			{
				DeviceH dev = get<0>( m_hBuf.getArgs() );
				if( dev )
					vkUnmapMemory( dev, m_hMem );
			}
		}

		void updateMapped( void const* data, size_t size ) noexcept
		{
			void* mapped = map();
			memcpy( mapped, data, size );
			unmap();
		}

		void updateStaging( GFX const& gfx, void const* data, size_t size );

		operator BufferH const& ( ) const { return m_hBuf; }

	};

	// vertex buffer that lives on the GPU, updatable by staging texture
	template< class V >
	class VertexBufferLoc : public GPUBuffer {
	protected:
		uint32_t m_size;
		using type = V;
	public:
		VertexBufferLoc( GFX const& gfx, uint32_t n, V const* pv = nullptr )
		: GPUBuffer( gfx, sizeof( V ) * n, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
		, m_size( n )
		{
			if( pv )
				updateStaging( gfx, pv, n * sizeof( V ) );
		}

		VertexBufferLoc( GFX const& gfx, std::vector<V> const& v )
			: GPUBuffer( gfx, v.size() * sizeof( V ), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
			, m_size( uint32_t( v.size() ) )
		{
			updateStaging( gfx, v.data(), uint32_t( v.size() ) );
		}

		template< uint32_t n >
		VertexBufferLoc( GFX const& gfx, V const (&v)[n] = {} )
		: GPUBuffer( gfx, sizeof( V ) * n, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
		, m_size( n )
		{
			updateStaging( gfx, v, n );
		}
		VertexBufferLoc( VertexBufferLoc const& ) = delete;
		VertexBufferLoc( VertexBufferLoc&& other ) noexcept : GPUBuffer( (GPUBuffer&&)other ) { m_size = other.m_size; }

		template< int32_t n>
		void updateStaging( GFX const& gfx, V const ( &v )[n] )
		{
			__super::updateStaging( gfx, v, sizeof( V ) * n );
		}
		void updateStaging( GFX const& gfx, V const* pv, uint32_t n )
		{
			assert( m_size >= n );
			__super::updateStaging( gfx, pv, sizeof( V ) * n );
		}

		static VkPipelineVertexInputStateCreateInfo getInputLayout() { return V::getInputLayout(); }

		uint32_t getVertexCount() const { return m_size; }
	};

	// host visible vertex buffer, can be mapped and updated
	template< class V >
	class VertexBufferUpd : public GPUBuffer {
	protected:
		size_t m_size;
	public:
		template< size_t n >
		VertexBufferUpd( GFX const& gfx, V const( &v )[n] )
		: GPUBuffer( gfx, n * sizeof( V ), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
		, m_size( n )
		{
			updateMapped( v, n * sizeof( V ) );
		}

		VertexBufferUpd( GFX const& gfx, std::vector<V> const& v )
			: GPUBuffer( gfx, v.size() * sizeof( V ), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
			, m_size( v.size() )
		{
			updateMapped( v.data(), v.size() * sizeof( V ) );
		}
		
		VertexBufferUpd( GFX const& gfx, V const* pv, size_t n )
		: GPUBuffer( gfx, n * sizeof( V ), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
		, m_size( n )
		{
			if( pv )
				updateMapped( pv, n * sizeof( V ) );
		}

		VertexBufferUpd( VertexBufferUpd const& ) = delete;
		VertexBufferUpd( VertexBufferUpd&& other ) noexcept : GPUBuffer( std::move(other) ) { m_size = other.m_size; }

		template< size_t n>
		void updateMapped( V const ( &v )[n] )
		{
			static_assert( m_size >= n );
			__super::updateMapped( v, n * sizeof( V ) );
		}
		void updateMapped( V const* pv, size_t n )
		{
			static_assert( m_size >= n );
			__super::updateMapped( pv, n * sizeof( V ) );
		}

		VkPipelineVertexInputStateCreateInfo getInputLayout() { return V::getInputLayout(); }
	};

	template<class T>
	struct MappedUniformBuffer : GPUBuffer // this is intended to be used as GPU resource handle, as we have a constant buffer for each queue/backbuffer 
	{
		T* mapped;

		MappedUniformBuffer( GFX const& gfx, T const& init = T{} ) 
		: GPUBuffer( gfx, sizeof( T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
		{
			mapped = (T*)map();
			*mapped = init;
		}
		MappedUniformBuffer( MappedUniformBuffer const& ) = delete;

		MappedUniformBuffer( MappedUniformBuffer&& other ) : GPUBuffer( (GPUBuffer&&)other )
		{
			mapped = other.mapped;
			other.mapped = nullptr;
		}

		~MappedUniformBuffer() {
			if( mapped )
				unmap();
		}

		operator T& ( ) { return *mapped; }
		operator T const& ( ) const { return *mapped; }
	};

	class ShaderModule
	{
		PipelineShaderStageCreateInfo m_ci;
		ShaderModuleH m_h;
	public:
		ShaderModule( DeviceH const& dev, uint32_t const* byteCode, size_t size, VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT, char const* entry = "main" )
		{
			ShaderModuleCreateInfo ci( byteCode, size );
			_rts( vkCreateShaderModule( dev, &ci, VK::_defAlloc, m_h.set( dev ) ), "failed to create shader module." );
			m_ci = PipelineShaderStageCreateInfo( m_h, stage, entry );
		}
		operator VkShaderModule const& ( ) const { return m_h; }
		PipelineShaderStageCreateInfo const& getCI() const { return m_ci; }
	};

	/// <summary>
	/// Renderer, to be added to a GFXPipeline, this class can actually draw and adds entries the pipelines queue
	/// </summary>
	class Renderer
	{
	public:
		struct __declspec( align( 4 ) ) VertexWTex {
			float x, y, z, _padding;
			float u, v;
			static VkPipelineVertexInputStateCreateInfo getInputLayout();
		};
		struct __declspec( align( 4 ) ) VertexWColTex {
			float x, y, z, _padding;
			float r, g, b, a;
			float u, v;
			static VkPipelineVertexInputStateCreateInfo getInputLayout();
		};
	protected:
		static uint64_t s_freeID;
		
		uint64_t m_id;
		DeviceWH m_dev;
		std::vector<CommandBufferH> m_cbs; // normally one, reusable
		std::vector<ShaderModule> m_sss; // shader stages
		SemaphoreH m_sema;
		RenderPassH m_renderPass;
		DescriptorSetLayoutH m_descriptorSetLayout;
		PipelineLayoutH m_pipelineLayout;
		PipelineH m_pipeline;
		std::map< VkImage, FramebufferH > m_framebuffers;
	public:
		Renderer( 
			GFX const& gfx,
			std::vector<std::unique_ptr<Image>> const& attachments,
			AttachmentDescription const& desc,
			std::vector<ShaderModule> const& shaderStages,
			VkPipelineVertexInputStateCreateInfo const& inputLayout,
			bool withSignal = false );
		Renderer( Renderer const& ) = delete;
		Renderer( Renderer&& other ) noexcept
		: m_id( other.m_id )
		, m_dev( std::move( other.m_dev ))
		, m_sema( std::move( other.m_sema ))
		, m_cbs( std::move( other.m_cbs ))
		{}

		VkResult submit( GFX const& gfx, std::vector<SemaphoreH> const wait = {} ); // call this base class function last or put semaphore in queue in derived class
		virtual void preRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
		virtual void render( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
		virtual void postRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );

		virtual SemaphoreH const& getFinishSignal() const;
	};


	/// <summary>
	/// GFXPipeline, base of every engine
	/// </summary>
	class GFX
	{
	protected:
		bool m_validation;

		InstanceH m_inst;
		PhysicalDeviceH m_gpu;
		DeviceH m_dev;  // initialized by derived class
		CommandPoolH m_cp; // initialized in derived class
		QueueH m_cq;

		PhysicalDeviceProperties2 m_props;
		PhysicalDeviceMemoryProperties2 m_memProps;

		std::vector< char const* > m_enabledLayers;
		std::vector< char const* > m_enabledInstanceExtensions;
		std::vector< char const* > m_enabledDeviceExtensions; // used in derived class

		std::vector< std::shared_ptr<Renderer> > m_renderers;

		std::array<FenceH, VK_MAX_FRAME_LAG> m_fences; // initialized in deriving class

		std::unique_ptr< RenderTarget > m_rt;

		mat4x4 m_mView;
		mat4x4 m_mProjection;
		uint64_t m_frame;

	public:
		GFX( int32_t iGPU = -1, const char* name = "VIOSO VULKAN APP", VkOffset2D const& windowCoord = {INT_MAX,INT_MAX}, bool validate = false );
		virtual ~GFX();

		uint32_t findMemTypeFromProps( uint32_t typeBits, VkMemoryPropertyFlags requirementMask ) const;
		bool instanceCan( const char* extensionName ) const;
		bool gpuCan( const char* extensionName ) const;

		void setMView( mat4x4 const& m ) { cpy( m_mView, m ); }
		mat4x4 const& getMView() const { return m_mView; }
		mat4x4& MView() { return m_mView; }

		DeviceH const & getDevice() const { return m_dev; }
		CommandPoolH const& getPool() const { return m_cp; }
		QueueH const& getQueue() const { return m_cq; }

		void setMProjection( mat4x4 const& m ) { cpy( m_mProjection, m ); }
		mat4x4 const& getMProjection() const { return m_mProjection; }
		mat4x4& mProjection() { return m_mProjection; }


		CommandBufferH createCommandBuffer() const;
		std::vector<CommandBufferH> createCommandBuffers( uint32_t n ) const;

		VkResult submitBufferCopy( GPUBuffer const& src, GPUBuffer const& dst, VkDeviceSize size ) const;
		VkResult submitImageCopy( Image const& src, Image const& dst, VkDeviceSize size ) const;
		VkResult submit(
			std::vector<CommandBufferH> const& cbs,
			FenceH const& f = 0,
			std::vector<SemaphoreH> const& wait = {},
			std::vector<SemaphoreH> const& signal = {} ) const;

		ShaderModule createVSPassthrough() const;
		ShaderModule createFSPassthrough() const;

		virtual void addRenderer( std::shared_ptr< Renderer > renderer );

		virtual void preRender( mat4x4 const& world );
		virtual void render( mat4x4 const& world );
		virtual void postRender( mat4x4 const& world );

		RenderTarget const& getRT() const { return *m_rt.get(); }
	};

	/// <summary>
	/// OutputWindow, a window with attached pipeline. Use direct or derive your special class from it
	/// </summary>
	class OutputWindow : public GFX
	{
		static LRESULT CALLBACK  _WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	protected:
		intptr_t m_hWnd;
		VkSurfaceTransformFlagBitsKHR m_preTransform;
		SurfaceKHRH m_surface;
		SwapchainKHRH m_sc;
		VkCompositeAlphaFlagBitsKHR m_alphaMode;
		VkPresentModeKHR m_presentMode;
		VkColorSpaceKHR m_colorSpace;
		virtual intptr_t wndProc( intptr_t hWnd, uint32_t msg, intptr_t wParam, intptr_t lParam );

	public:
		OutputWindow( HINSTANCE hInstance, const char* windowName, int x, int y, int width, int height, int nCmdShow, VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR, uint32_t bufferCount = 3, VkCompositeAlphaFlagBitsKHR alphaMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, bool withDepth = true, int iGPU = -1, bool validate = false );


		virtual void preRender( mat4x4 const& world ); /// this will clear
		virtual void postRender( mat4x4 const& world ); /// this will swap

	};

} // namespace VK;
