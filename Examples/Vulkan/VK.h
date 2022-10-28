#pragma once

// credits go to VULKAN TUTORIAL
// https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_TYPESAFE_CONVERSION

#include "vulkan/vulkan.h"
#include "vulkan/vk_layer.h"
#include "linmath.h"
#include <string>
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
			throw std::runtime_error( std::string( "RUNTIME_ERROR: " ) + msg );
	}
	template< class T >
	constexpr void _rtc( T const exp, char const* msg ) {
		if( !exp )
			throw std::runtime_error( std::string( "RUNTIME_ERROR: " ) + msg );
	}
	constexpr VkResult _rts( VkResult const exp, char const* msg ) {
		if( VK_SUCCESS != exp )
			throw std::runtime_error( std::string( "RUNTIME_ERROR: (" ) + std::to_string(int(exp)) + ") " + msg);
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

	inline VkResult waitForFenceThrow( DeviceH const& dev, FenceH const& f, uint64_t timeout = -1 )
	{
		VkResult res = vkWaitForFences( dev, 1, &f.hnd, VK_TRUE, -1 );
		if( VK_ERROR_DEVICE_LOST == res )
			throw device_lost( "Device lost while waiting for fence." );
		return res;
	}

	template< class IT >
    inline VkResult waitForFencesThrow( DeviceH const& dev, IT _first, IT _last, bool bAll = true, uint64_t timeout = -1 )
	{
		VkResult res = vkWaitForFences( dev, uint32_t( _last - _first ), _first, bAll ? VK_TRUE : VK_FALSE, timeout );
		if( VK_ERROR_DEVICE_LOST == res )
			throw device_lost( "Device lost while waiting for fence." );
		res = vkResetFences( dev, uint32_t( _last - _first ), _first );
		return res;
	}

	template< class T >
	struct atomicRef
	{
		T tmp;
		std::atomic<T>& v;
		atomicRef( std::atomic<T>& val ) : tmp( val ), v( val ) {}
		operator T* ( ) { return &tmp; }
		~atomicRef() { v = tmp; }
	};

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

	constexpr VkExtent3D to_3D( VkExtent2D const& e2 ) { return VkExtent3D{ e2.width, e2.height, 1 }; }
	constexpr VkExtent2D to_2D( VkExtent3D const& e3 ) { return VkExtent2D{ e3.width, e3.height }; }

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
		VkImageLayout m_finalLayout;
	public:
		Image( VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ) : m_finalLayout( finalLayout ) {}
		Image( Image const& ) = delete;
		Image( Image&& other ) noexcept :
			m_view( std::move( other.m_view )),
			m_sema( std::move( other.m_sema )),
			m_ci(   std::move( other.m_ci )),
			m_finalLayout( std::move( other.m_finalLayout ))
		{}
		virtual ~Image();
		virtual operator VkImage const& () const = 0;
		operator VkImageView const& () const { return m_view; }
		SemaphoreH const& getSema() const { return m_sema; }
		void swapSema( SemaphoreH& other ) { m_sema.swap( other ); }
		ImageCreateInfo const& getCI() const { return m_ci; }
		VkImageLayout const& getFinalLayout() const { return m_finalLayout;	}
		PipelineViewportStateCreateInfo getPipelineViewportStateCreateInfo() const
		{
			return PipelineViewportStateCreateInfo(
				{ { float( 0 ), float( 0 ), float( m_ci.extent.width ), float( m_ci.extent.height ), float( 0 ), float( 1 ) }	},
				{ { int32_t( m_ci.extent.width ), int32_t( m_ci.extent.height ) } },
				true
			);
		}
		VkAttachmentDescription getAttachmentDescription() const
		{
			return AttachmentDescription( m_ci.format, m_finalLayout, m_ci.samples );
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
	/// TextureImage, some texture
	/// </summary>
	class DepthBuffer : public Image {
	protected:
		ImageH m_image;
		DeviceMemoryAH m_mem;
	public:
		DepthBuffer(
			GFX const& gfx,
			VkImageCreateInfo const& ci = ImageCreateInfo(),
			VkMemoryAllocateFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		DepthBuffer( DepthBuffer const&  ) = delete;
		DepthBuffer( DepthBuffer && other ) noexcept 
			: Image( std::move( other ))
			, m_image( std::move( other.m_image ))
			, m_mem( std::move( other.m_mem ))
		{};
		virtual ~DepthBuffer() {};
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
		std::vector< SemaphoreH > m_imageAcquireSemas;
		std::vector<DepthBuffer> m_depthBuffers;

		DeviceWH m_dev;

		VkExtent3D m_extent;
		uint32_t m_mipLevels;
		VkFormat m_format;
		VkFormat m_depthFormat;

		std::atomic<uint32_t> m_currentBuffer;
	public:

		RenderTarget( DeviceH const& dev, std::vector<VkImage> const& images, VkExtent3D const& extent = { 0 }, VkFormat format = VK_FORMAT_UNDEFINED, VkFormat depthFormat = VK_FORMAT_UNDEFINED, uint32_t mipLevels = 1 );
		RenderTarget( RenderTarget const& ) = delete;
		RenderTarget( RenderTarget&& other ) noexcept
			: m_images( std::move( other.m_images ))
			, m_depthBuffers( std::move( other.m_depthBuffers ))
			, m_imageAcquireSemas( std::move( other.m_imageAcquireSemas ))

			, m_dev( std::move( other.m_dev ))

			, m_extent( other.m_extent )
			, m_mipLevels( other.m_mipLevels )
			, m_format( other.m_format )
			, m_depthFormat( other.m_depthFormat )

			, m_currentBuffer( m_currentBuffer.load() )
		{}

		static const VkClearColorValue s_black;
		static const VkClearColorValue s_sky;
		static const VkClearDepthStencilValue s_clearDepth;

		std::map< VkImage, FramebufferH > createFramebufferMap( RenderPassH const& renderPass ) const;

		VkExtent3D const& getExtent() const { return m_extent; }
		uint32_t const& getMipLevels() const { return m_mipLevels; }
		VkFormat const& getColorFormat() const { return m_format; }
		VkFormat const& getDepthFormat() const { return m_depthFormat; }
		uint32_t getCurrentIndex() const { return m_currentBuffer; }
		size_t getNumBuffers() const { return m_images.size(); }
		VkViewport getFullViewport() const { return VkViewport{ 0, 0, ( float )m_extent.width, ( float )m_extent.height, 0, 1 }; }
		VkRect2D getFullScissorRect() const { return VkRect2D{ { 0, 0 }, { m_extent.width, m_extent.height } };	}

		std::vector<VkAttachmentDescription> getAttachmentDescriptions() const {
			std::vector<VkAttachmentDescription> descs;
			if( !m_images.empty() )
				descs.emplace_back( m_images.front()->getAttachmentDescription() );
			if( !m_depthBuffers.empty() )
				descs.emplace_back( m_depthBuffers.front().getAttachmentDescription() );
			return descs;
		};

		std::vector<VkAttachmentReference> getColorAttachmentReferences( uint32_t offs = 0 ) const {
			std::vector<VkAttachmentReference> descs;
			if( !m_images.empty() )
				descs.emplace_back( AttachmentReference( offs + uint32_t(descs.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ) );
			return descs;
		};

		std::unique_ptr<AttachmentReference> getDepthStencilAttachmentReference( uint32_t offs = 0 ) const {
			return m_depthBuffers.empty() ? nullptr : std::make_unique<AttachmentReference>( offs, m_depthBuffers.front().getFinalLayout() );
		};

		std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> getPipelineDepthStencilStateCreateInfo() const {
			return m_depthBuffers.empty() ? nullptr : std::make_unique<PipelineDepthStencilStateCreateInfo>();
		};

		PipelineViewportStateCreateInfo getPipelineViewportStateCreateInfo() const {
			return m_images.empty() ? PipelineViewportStateCreateInfo() : m_images.front()->getPipelineViewportStateCreateInfo();
		}

		SemaphoreH getCurrentImageAquire() const {
			return m_images.size() > m_currentBuffer.load() ? m_images[m_currentBuffer]->getSema() : SemaphoreH();
		}

		virtual Image const& getNextBuffer() {
			assert( !m_images.empty() );
			m_currentBuffer++;
			if( uint32_t( m_images.size() ) <= m_currentBuffer )
				m_currentBuffer = 0;
			return *m_images[m_currentBuffer];
		}

		Image const& getCurrentBuffer() const {
			assert( !m_images.empty() );
			return *m_images[m_currentBuffer];
		}

		virtual void resize( int width, int height, DeviceH& dev, SwapchainKHRH& sc );
		virtual void finish() = 0;
	};

	/// <summary>
	/// A backbuffer; wraps SwapChainImages from a swapchain
	/// </summary>
	class BackBuffer : public RenderTarget
	{
		SwapchainKHRWH m_sc;
	public:
		/// <summary>
		/// Contructor
		/// </summary>
		/// <param name="gfx"></param>
		/// <param name="sc"></param>
		/// <param name="hDepth"></param>
		BackBuffer(GFX const& gfx, SwapchainKHRH const& sc, VkFormat format, VkExtent2D extent, VkFormat depthFormat = VK_FORMAT_UNDEFINED);
		virtual Image const& getNextBuffer();
		virtual void finish() {};
		//virtual VkAttachmentDescription getAttachmentDescription() const {
		//	return AttachmentDescription( m_format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
		//};
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
		virtual void finish() {};
	};

	class Sampler : public Image
	{
	public:
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

	class VertexBuffer : public GPUBuffer
	{
	protected:
		uint32_t m_num;
		uint32_t m_stride;
	public:
		VertexBuffer( GFX const& gfx, uint32_t num, uint32_t stride )
			: GPUBuffer( gfx, size_t(num) * stride, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) 
			, m_num( num )
			, m_stride( stride )
		{}
		VertexBuffer( VertexBuffer const& ) = delete;
		VertexBuffer( VertexBuffer&& other ) noexcept : GPUBuffer( ( GPUBuffer&& )other )
		{
			m_num = other.m_num; 
			m_stride = other.m_stride;
		}

		uint32_t getVertexCount() const { return m_num; }

		virtual VkPipelineVertexInputStateCreateInfo const& getInputLayout() const = 0;
	};

	// vertex buffer that lives on the GPU, updatable by a staging buffer
	template< class V >
	class VertexBufferLoc : public VertexBuffer {
		using type = V;
	public:
		VertexBufferLoc( GFX const& gfx, uint32_t n, V const* pv = nullptr )
		: VertexBuffer( gfx, n, sizeof( V ) )
		{
			if( pv )
				updateStaging( gfx, pv, n * sizeof( V ) );
		}

		VertexBufferLoc( GFX const& gfx, std::vector<V> const& v )
			: VertexBuffer( gfx, v.size(), sizeof( V ))
		{
			updateStaging( gfx, v.data(), uint32_t( v.size() ) );
		}

		template< uint32_t n >
		VertexBufferLoc( GFX const& gfx, V const (&v)[n] = {} )
		: VertexBuffer( gfx, n, sizeof( V ) )
		{
			updateStaging( gfx, v, n );
		}
		VertexBufferLoc( VertexBufferLoc const& ) = delete;

		template< int32_t n>
		void update( GFX const& gfx, V const ( &v )[n] )
		{
			__super::updateStaging( gfx, v, sizeof( V ) * n );
		}
		void update( GFX const& gfx, V const* pv, uint32_t n )
		{
			assert( m_num >= n );
			__super::updateStaging( gfx, pv, sizeof( V ) * n );
		}
		void update( GFX const& gfx, std::vector<V> v )
		{
			assert( size_t(m_num) >= v.size() );
			__super::updateStaging( gfx, v.data(), v.size() * sizeof( V ) );
		}

		virtual VkPipelineVertexInputStateCreateInfo const& getInputLayout() const { return V::getInputLayout(); }
	};

	// host visible vertex buffer, can be mapped and updated
	template< class V >
	class VertexBufferUpd : public VertexBuffer {
	public:
		template< size_t n >
		VertexBufferUpd( GFX const& gfx, V const( &v )[n] )
		: VertexBuffer( gfx, n, sizeof( V ) )
		{
			updateMapped( v, n * sizeof( V ) );
		}

		VertexBufferUpd( GFX const& gfx, std::vector<V> const& v )
		: VertexBuffer( gfx, v.size(), sizeof( V ) )
		{
			updateMapped( v.data(), v.size() * sizeof( V ) );
		}
		
		VertexBufferUpd( GFX const& gfx, V const* pv, size_t n )
		: VertexBuffer( gfx, n, sizeof( V ) )
		{
			if( pv )
				updateMapped( pv, n * sizeof( V ) );
		}

		VertexBufferUpd( VertexBufferUpd const& ) = delete;

		template< size_t n>
		void update( V const ( &v )[n] )
		{
			static_assert( m_num >= n );
			__super::updateMapped( v, n * sizeof( V ) );
		}
		void update( V const* pv, size_t n )
		{
			static_assert( m_num >= n );
			__super::updateMapped( pv, n * sizeof( V ) );
		}
		void update( std::vector<V> const& v )
		{
			static_assert( size_t(m_num) >= v.size() );
			__super::updateMapped( v.data(), v.size() * sizeof( V ) );
		}

		virtual VkPipelineVertexInputStateCreateInfo const& getInputLayout() const { return V::getInputLayout(); }
	};

	class UniformBuffer : public GPUBuffer 
	{
	public:
		UniformBuffer( GFX const& gfx, size_t size )
			: GPUBuffer( gfx, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT )
		{}
		UniformBuffer( UniformBuffer const& ) = delete;
		UniformBuffer( UniformBuffer&& other) : GPUBuffer( std::move(other)){}
	};

	template<class T>
	struct MappedUniformBuffer : public UniformBuffer // this is intended to be used as GPU resource handle, as we have a constant buffer for each queue/backbuffer 
	{
		T* mapped;

		MappedUniformBuffer( GFX const& gfx, T const& init = T{} ) 
		: UniformBuffer( gfx, sizeof( T))
		{
			mapped = (T*)map();
			*mapped = init;
		}
		MappedUniformBuffer( MappedUniformBuffer const& ) = delete;

		MappedUniformBuffer( MappedUniformBuffer&& other ) : UniformBuffer( std::move(other) )
		{
			mapped = other.mapped;
			other.mapped = nullptr;
		}

		~MappedUniformBuffer() {
			if( mapped )
				unmap();
		}

		operator T& () { return *mapped; }
		operator T const& () const { return *mapped; }
		T& operator->() { return *mapped; }
		T const& operator ->() const { return *mapped; }
	};

	class ShaderModule
	{
		PipelineShaderStageCreateInfo m_ci;
		ShaderModuleH m_h;
	public:
		ShaderModule( 
			DeviceH const& dev, 
			uint32_t const* byteCode,
			size_t size,
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT,
			char const* entry = "main" )
		{
			ShaderModuleCreateInfo ci( byteCode, size );
			_rts( vkCreateShaderModule( dev, &ci, VK::_defAlloc, m_h.set( dev ) ), "failed to create shader module." );
			m_ci = PipelineShaderStageCreateInfo( m_h, stage, entry );
		}
		ShaderModule( ShaderModule const& ) = delete;
		ShaderModule( ShaderModule&& other ) noexcept : m_h( std::move( other.m_h ) ), m_ci( other.m_ci ) {}
		operator VkShaderModule const& ( ) const { return m_h; }
		PipelineShaderStageCreateInfo const& getCI() const { return m_ci; }
	};

	/// <summary>
	/// Renderer, to be added to a GFXPipeline, this class can actually draw and adds entries the pipelines queue
	/// </summary>
	class Renderer
	{
	public:
		struct Vertex {
			__declspec( align( 4 ) ) float x, y, z, _padding;
			static const PipelineVertexInputStateCreateInfo _layout;
			Vertex( float _x, float _y, float _z ): x( _x ), y( _y ), z( _z ) {}
			virtual VkPipelineVertexInputStateCreateInfo const& getInputLayout() const { return _layout; }
		};
		struct VertexWCol : Vertex {
			__declspec( align( 4 ) ) float r, g, b, a;
			static const PipelineVertexInputStateCreateInfo _layout;
			VertexWCol( float _x, float _y, float _z, float _r, float _g, float _b, float _a ): Vertex( _x , _y, _z ), r(_r), g(_g), b(_b), a(_a) {}
			virtual VkPipelineVertexInputStateCreateInfo const& getInputLayout() const { return _layout; }
		};
		struct VertexWTex : Vertex {
			__declspec( align( 4 ) )float u, v;
			static const PipelineVertexInputStateCreateInfo _layout;
			VertexWTex( float _x, float _y, float _z, float _u, float _v ): Vertex( _x , _y, _z ), u(_u), v(_v) {}
			virtual VkPipelineVertexInputStateCreateInfo const& getInputLayout() const { return _layout; }
		};
		struct VertexWColTex : VertexWCol {
			__declspec( align( 4 ) ) float u, v;
			static const PipelineVertexInputStateCreateInfo _layout;
			VertexWColTex( float _x, float _y, float _z, float _r, float _g, float _b, float _a, float _u, float _v  ) : VertexWCol( _x , _y, _z, _r, _g, _b, _a ), u(_u), v(_v) {}
			virtual VkPipelineVertexInputStateCreateInfo const& getInputLayout() const { return _layout; }
		};

	protected:
		static uint64_t s_freeID;
		
		uint64_t m_id;

		DeviceWH m_dev; // a weak ref to the device, not really used at the moment TODO check, if that can be removed

		uint32_t m_iFrame; // the current frame resource
		// following attributes must be there per ahead frame
		std::vector<CommandBufferH> m_cbs; // the command buffers
		std::vector<SemaphoreH> m_finishedSemas; // rendering of this pipeline has finished, 
		std::vector<FenceH> m_finishedFences; // rendering of this pipeline has finished, 

		// attachements don't live here, these get only referenced from render target via frame buffers
		std::map< VkImage, FramebufferH > m_framebuffers;

		std::vector<std::shared_ptr<ShaderModule>> m_sss; // shader stages
		std::vector<std::shared_ptr<Sampler>> m_sms; // samplers stages
		std::shared_ptr<UniformBuffer> m_ub; // uniform buffer, TODO check if that must be also there per ahead frame
		std::shared_ptr<VertexBuffer> m_vb; // vertex buffer

		RenderPassH m_renderPass; // the render pass

		DescriptorSetLayoutH m_descriptorSetLayout; // this is the final layout of resources
		PipelineLayoutH m_pipelineLayout; // this is the layout of the (vertex-) input
		PipelineH m_pipeline; // the pipeline, this has to be activated when a renderer submits
	public:
		Renderer( 
			GFX const& gfx,
			std::vector<VkDynamicState> const& dynamicStates,
			std::vector<std::shared_ptr<ShaderModule>>&& shaderStages,
			RenderTarget const& rt,
			std::shared_ptr <UniformBuffer>&& uniformBuffer,
			std::shared_ptr <VertexBuffer>&& vertexBuffer,
			std::vector<std::shared_ptr<Sampler>>&& samplers,
			uint32_t nFramesAhed = 1 );
		Renderer( Renderer const& ) = delete;
		Renderer( Renderer&& other ) noexcept
		: m_id( other.m_id )
		, m_dev( std::move( other.m_dev ) )
		, m_iFrame( other.m_iFrame )
		, m_cbs( std::move( other.m_cbs ) )
		, m_finishedSemas( std::move( other.m_finishedSemas ) )
		, m_finishedFences( std::move( other.m_finishedFences ) )
		, m_framebuffers( std::move( other.m_framebuffers ) )
		, m_sss( std::move( other.m_sss ))
		, m_sms( std::move( other.m_sms ))
		, m_ub( std::move( other.m_ub ))
		, m_vb( std::move( other.m_vb ))
		, m_renderPass( std::move( other.m_renderPass))
		, m_descriptorSetLayout( std::move( other.m_descriptorSetLayout ))
		, m_pipelineLayout( std::move( other.m_pipelineLayout ))
		, m_pipeline( std::move( other.m_pipeline ))
		{}

		//VkResult submit( GFX const& gfx, std::vector<SemaphoreH> const wait = {} ); // call this base class function last or put semaphore in queue in derived class
		virtual void preRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
		virtual void render( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
		virtual void postRender( GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );

		virtual SemaphoreH const& getFinishSignal() const { return m_finishedSemas[m_iFrame]; }
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
