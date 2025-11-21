// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#if defined(VIOSOWARPBLEND_EXPORTS)
	#if defined(_WIN32) || defined(__CYGWIN__)
		#define VIOSOWARPBLEND_API(ret,name,args) extern "C" __declspec(dllexport) ret name args
	#else
		#define VIOSOWARPBLEND_API(ret,name,args) extern "C" __attribute__((visibility("default"))) ret name args
	#endif
#elif defined(VIOSOWARPBLEND_DYNAMIC_DEFINE)

	#if defined(_WIN32) || defined(__CYGWIN__)
		extern HMODULE hMVIOSOWARPBLEND_DYNAMIC;
	#else
		extern void *hMVIOSOWARPBLEND_DYNAMIC;
	#endif //defined(_WIN32) || defined(__CYGWIN__)

	#define VIOSOWARPBLEND_API(ret,name,args) \
		typedef ret (*pfn_##name)args;\
		extern "C" pfn_##name name;
	#undef VIOSOWARPBLEND_DYNAMIC_DEFINE

#elif defined(VIOSOWARPBLEND_DYNAMIC_IMPLEMENT)

	#if defined(_WIN32) || defined(__CYGWIN__)
		HMODULE hMVIOSOWARPBLEND_DYNAMIC = 0;
	#else
		void *hMVIOSOWARPBLEND_DYNAMIC = NULL;
	#endif //defined(_WIN32) || defined(__CYGWIN__)

	#define VIOSOWARPBLEND_API(ret,name,args) \
		pfn_##name name = NULL;
	#undef VIOSOWARPBLEND_DYNAMIC_IMPLEMENT

#elif defined(VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT)

	#if defined(_WIN32) || defined(__CYGWIN__)
		HMODULE hMVIOSOWARPBLEND_DYNAMIC = 0;
	#else
		void *hMVIOSOWARPBLEND_DYNAMIC = NULL;
	#endif //defined(_WIN32) || defined(__CYGWIN__)

	#define VIOSOWARPBLEND_API(ret,name,args) \
		typedef ret (*pfn_##name)args;\
		pfn_##name name = NULL;
	#undef VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT

#elif defined(VIOSOWARPBLEND_DYNAMIC_INITIALIZE)

		#if !defined( VIOSOWARPBLEND_FILE )
			#if defined( _M_X64 )
				#define VIOSOWARPBLEND_FILE "ViosoWarpBlend64"
			#else
				#if defined(_WIN32) || defined(__CYGWIN__)
					#define VIOSOWARPBLEND_FILE "ViosoWarpBlend"
				#else
					#define VIOSOWARPBLEND_FILE "libViosoWarpBlend"
				#endif //defined(_WIN32) || defined(__CYGWIN__)
			#endif //def _M_X64
		#endif // !defined( VIOSOWARPBLEND_FILE )

	#if defined(_WIN32) || defined(__CYGWIN__)
		hMVIOSOWARPBLEND_DYNAMIC = ::LoadLibraryA( VIOSOWARPBLEND_FILE );
		#define VIOSOWARPBLEND_API(ret,name,args) \
		name = (pfn_##name)::GetProcAddress( hMVIOSOWARPBLEND_DYNAMIC, #name )
	#else
		hMVIOSOWARPBLEND_DYNAMIC = dlopen( VIOSOWARPBLEND_FILE, RTLD_NOW );
		#define VIOSOWARPBLEND_API(ret,name,args) \
		name = (pfn_##name)dlsym( hMVIOSOWARPBLEND_DYNAMIC, #name )
	#endif //defined(_WIN32) || defined(__CYGWIN__)
	#undef VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#elif defined(VIOSOWARPBLEND_DYNAMIC_UNINITIALIZE)
	if( 0 != hMVIOSOWARPBLEND_DYNAMIC )
	#if defined(_WIN32) || defined(__CYGWIN__)
		FreeLibrary( hMVIOSOWARPBLEND_DYNAMIC );
	#else
		dlclose( hMVIOSOWARPBLEND_DYNAMIC );
	#endif //defined(_WIN32) || defined(__CYGWIN__)
	hMVIOSOWARPBLEND_DYNAMIC = 0;
	#define VIOSOWARPBLEND_API(ret,name,args) \
		name = NULL
	#undef VIOSOWARPBLEND_DYNAMIC_UNINITIALIZE
#elif !defined(VIOSOWARPBLEND_API)
	#if defined(_WIN32) || defined(__CYGWIN__)
		#define VIOSOWARPBLEND_API(ret,name,arg) extern "C" __declspec(dllimport) ret name arg
	#else
		#define VIOSOWARPBLEND_API(ret,name,arg) extern "C" ret name arg
	#endif //defined(_WIN32) || defined(__CYGWIN__)
#endif
