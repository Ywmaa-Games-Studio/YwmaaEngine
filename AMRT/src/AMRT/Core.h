#pragma once

#ifdef AMRT_PLATFORM_WINDOWS
	#ifdef AMRT_BUILD_DLL
		#define AMRT_API __declspec(dllexport)
	#else
		#define AMRT_API __declspec(dllimport)
	#endif

#else
	#error AMRT only supports Windows!
#endif // AMRT_PLATFORM_WINDOWS
