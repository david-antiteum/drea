#pragma once

#ifdef DREA_CORE_DLL
	#ifdef WIN32
		#pragma warning(disable:4251)
		#ifdef DREA_CORE_EXPORTS 
			#define DREA_CORE_API __declspec(dllexport)
		#else
			#define DREA_CORE_API __declspec(dllimport)
		#endif
	#else
		#ifdef DREA_CORE_EXPORTS
			#define DREA_CORE_API __attribute__((visibility("default")))
		#else
			#define DREA_CORE_API
		#endif
	#endif
#else
	#define DREA_CORE_API
#endif
