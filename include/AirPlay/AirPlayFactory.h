
#pragma once

#include "AirPlayEx.h"
#if defined(_WIN32) 
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __attribute__((visibility("default")))
#endif

class CAirPlayFactory
{
public:
	EXPORT_API CAirPlayEx* Create();  
	EXPORT_API void Destroy(CAirPlayEx *);

};
