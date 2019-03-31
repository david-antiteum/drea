#pragma once

#ifdef DREA_CORE_DLL
	#ifdef  DREA_CORE_EXPORTS 
		#define DREA_CORE_API __declspec(dllexport)
	#else
		#define DREA_CORE_API __declspec(dllimport)
	#endif
#else
	#define DREA_CORE_API
#endif
