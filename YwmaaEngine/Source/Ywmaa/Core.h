#pragma once

#ifdef YM_PLATFORM_WINDOWS
	#ifdef YM_BUILD_DLL
		#define YWMAA_API __declspec(dllexport)
	#else
		#define YWMAA_API __declspec(dllimport)
	#endif

#else
	#error Ywmaa only supports Windows!
#endif // YM_PLATFORM_WINDOWS
