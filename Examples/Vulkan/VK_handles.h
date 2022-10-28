#pragma once
#include "vulkan/vulkan.h"
#include <array>
#include <vector>
#include <memory>

namespace VK 
{
	static VkAllocationCallbacks* _defAlloc = nullptr;
	template<class H>
	void _dummyDeleter( H, VkAllocationCallbacks* ) {};
	template<class H, auto dfn>
	void _listDeleter( H& h, VkAllocationCallbacks* c ) {
		dfn( 1, &h, c );
	}
	template<class H, auto dfn, class F>
	void _listDeleter( F const& f, H const& h, VkAllocationCallbacks* c ) {
		dfn( f, 1, &h, c );
	}
	template<class H, auto dfn, class F1, class F2 >
	void _listDeleter( F1 const& f1, F2 const& f2, H const& h, VkAllocationCallbacks* ) {
		dfn( f1, f2, 1, &h );
	}

	template<typename T, size_t s>
	constexpr std::array<T, s> to_array( T const* p ) { return *reinterpret_cast<std::array<T, s> const*>( p ); }

	template< typename H, auto dfn, typename... E>
	struct SHnd
	{
		H hnd;
		volatile unsigned int* piRef;
		volatile unsigned int* piWRef;
		typedef std::tuple< E...> Tuple;
		Tuple params;

		SHnd( H const h = 0, Tuple const args = {} )
		{
			if( h )
			{
				piRef = new volatile unsigned int;
				*piRef = 1;
				piWRef = new volatile unsigned int;
				*piWRef = 0;
			}
			else
			{
				piRef = nullptr;
				piWRef = nullptr;
			}
			hnd = h;
			params = args;
		}

		SHnd( SHnd const& other )
		{
			if( other.piRef )
				::InterlockedIncrement( other.piRef );
			hnd = other.hnd;
			piRef = other.piRef;
			piWRef = other.piWRef;
			params = other.params;
		}

		SHnd( SHnd&& other ) noexcept
		: params( std::move( other.params ) )
		{
			piRef = other.piRef;
			other.piRef = nullptr;
			hnd = other.hnd;
			other.hnd = 0;
			piWRef = other.piWRef;
			other.piWRef = nullptr;
		}

		constexpr void swap( SHnd& other ) noexcept
		{
			if( this != &other )
			{
				std::swap( piRef, other.piRef );
				std::swap( piWRef, other.piWRef );
				std::swap( hnd, other.hnd );
				std::swap( params, other.params );
			}
		}

		template< int... S >
		constexpr void _dfn( H h, Tuple args, std::index_sequence<S...> )
		{
			dfn( std::get<S>( args )..., h, _defAlloc );
		}

		unsigned int _destroy()
		{
			if( piRef &&
				0 == ::InterlockedDecrement( piRef ) )
			{
				if( 0 == piWRef )
				{
					delete piRef;
					delete piWRef;
				}
				if( hnd )
					_dfn( hnd, params, std::make_index_sequence< sizeof...( E )>() );
			}
			return piRef ? *piRef : 0;
		}

		~SHnd() {
			_destroy();
		}

		unsigned int reset( H const& h = 0, Tuple const& args = {} )
		{
			_destroy();
			hnd = h;
			params = args;
			if( hnd )
			{
				piRef = new volatile unsigned int;
				*piRef = 1;
				piWRef = new volatile unsigned int;
				*piWRef = 0;
			}
			else
			{
				piRef = NULL;
				piWRef = NULL;
			}
			return piRef ? *piRef : 0;
		}

		SHnd& operator=( SHnd const& other )
		{
			_destroy();
			if( other.piRef )
				::InterlockedIncrement( other.piRef );
			hnd = other.hnd;
			piRef = other.piRef;
			piWRef = other.piWRef;
			params = other.params;
			return *this;
		}

		SHnd& operator=( H const& other )
		{
			reset( other, Tuple{ 0 } );
			return *this;
		}

		H* detach()
		{
			static_assert( 1 == *piRef && 0 == *piWRef );
			H ret = hnd;
			hnd = 0;
			reset();
			return ret;
		}

		operator H const& ( ) const { return hnd; }
		operator bool() const { return 0 != hnd && piRef && 0 != *piRef; }
		bool operator==( SHnd const& other ) const { return hnd == other.hnd; }
		bool operator==( H const& other ) const { return hnd == hnd; }
		bool operator!=( SHnd const& other ) const { return hnd != other.hnd; }
		bool operator!=( H const& other ) const { return hnd != other; }
		//		H const* operator&() const { return &hnd; } // this interferes with the move function, use set() to get a writable ref to the hnd and the operator H for the value

		H* set( Tuple const& args = {} )
		{
			_destroy();
			hnd = 0;
			params = args;
			piRef = new volatile unsigned int;
			*piRef = 1;
			piWRef = new volatile unsigned int;
			*piWRef = 0;
			return &hnd;
		}

		Tuple const& getArgs() const { return params; }
	};

	template< typename H, auto dfn, typename... E>
	class WHnd
	{
	protected:
		H hnd;
		volatile unsigned int* piRef;
		volatile unsigned int* piWRef;
		typedef std::tuple< E...> Tuple;
		Tuple params;
	public:
		WHnd( SHnd<H, dfn, E...> const& s = SHnd<H, dfn, E...>() )
		{
			if( s.piWRef )
				::InterlockedIncrement( s.piWRef );
			hnd = s.hnd;
			piRef = s.piRef;
			piWRef = s.piWRef;
			params = s.params;
		}

		WHnd( WHnd const& s )
		{
			if( s.piWRef )
				::InterlockedIncrement( s.piWRef );
			hnd = s.hnd;
			piRef = s.piRef;
			piWRef = s.piWRef;
			params = s.params;
		}

		WHnd( WHnd&& other ) noexcept
		: params( std::move( other.params ) )
		{
			piRef = other.piRef;
			other.piRef = nullptr;
			hnd = other.hnd;
			other.hnd = 0;
			piWRef = other.piWRef;
			other.piWRef = nullptr;
		}

		void _destroy()
		{
			if( piWRef && 0 == ::InterlockedDecrement( piWRef ) && 0 == *piRef )
			{
				delete piRef;
				delete piWRef;
			}
		}

		WHnd& operator = ( WHnd const& s )
		{
			_destroy();
			if( s.piWRef )
				::InterlockedIncrement( s.piWRef );
			hnd = s.hnd;
			piRef = s.piRef;
			piWRef = s.piWRef;
			params = s.params;
			return *this;
		}

		~WHnd()
		{
			_destroy();
		}

		operator SHnd<H, dfn, E...>() const {
			return piRef && *piRef ? SHnd<H, dfn, E...>( *reinterpret_cast<SHnd<H, dfn, E...> const*>( this ) ) : SHnd<H, dfn, E...>();
		};
		operator H ( ) const {
			return piRef && *piRef ? hnd : 0;
		}
		operator bool() const { return hnd && *piRef; }

		Tuple const& getArgs() const { return params; }
	};

	#define VK_MKSHND( h ) typedef SHnd<Vk##h,vkDestroy##h> h##H; typedef WHnd<Vk##h,vkDestroy##h> h##WH
	#define VK_MKSFHND( h, fac ) typedef SHnd<Vk##h,vkDestroy##h,fac##WH> h##H; typedef WHnd<Vk##h,vkDestroy##h,fac##WH> h##WH

	VK_MKSHND( Instance );
	VK_MKSHND( Device );
	VK_MKSFHND( Image, Device );
	VK_MKSFHND( ImageView, Device );
	VK_MKSFHND( CommandPool, Device );
	VK_MKSFHND( SwapchainKHR, Device );
	VK_MKSFHND( SurfaceKHR, Instance );
	VK_MKSFHND( Semaphore, Device );
	VK_MKSFHND( Fence, Device );
	VK_MKSFHND( Buffer, Device );
	VK_MKSFHND( ShaderModule, Device );
	VK_MKSFHND( RenderPass, Device );
	VK_MKSFHND( DescriptorSetLayout, Device );
	VK_MKSFHND( PipelineLayout, Device );
	VK_MKSFHND( Pipeline, Device );
	VK_MKSFHND( Framebuffer, Device );


	typedef SHnd<VkPhysicalDevice, _dummyDeleter<VkPhysicalDevice>> PhysicalDeviceH; typedef WHnd<VkPhysicalDevice, _dummyDeleter<VkPhysicalDevice>> PhysicalDeviceWH;
	typedef SHnd<VkDeviceMemory, vkFreeMemory, DeviceWH> DeviceMemoryAH; typedef WHnd<VkDeviceMemory, vkFreeMemory, DeviceWH> DeviceMemoryAWH;
	typedef SHnd<VkQueue, _dummyDeleter<VkQueue>> QueueH; typedef WHnd<VkQueue, _dummyDeleter<VkQueue>> QueueWH;
	typedef SHnd<VkCommandBuffer, _listDeleter<VkCommandBuffer, vkFreeCommandBuffers, VkDevice, VkCommandPool>, DeviceWH, CommandPoolWH> CommandBufferH;
	typedef WHnd<VkCommandBuffer, _listDeleter<VkCommandBuffer, vkFreeCommandBuffers, VkDevice, VkCommandPool>, DeviceWH, CommandPoolWH> CommandBufferWH;
	typedef SHnd<VkImage, _dummyDeleter<VkImage>> SCImageH; typedef WHnd<VkImage, _dummyDeleter<VkImage>> SCImageWH;
};