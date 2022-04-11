#pragma once
#include "Core.h"

namespace AMRT {

class AMRT_API Application
{
public:
	Application();
	virtual ~Application();
	void Run();
};


// To be defined in CLIENT
Application* CreateApplication();
}
