#pragma once
#include "Core.h"

namespace Ywmaa {

class YWMAA_API Application
{
public:
	Application();
	virtual ~Application();
	void Run();
};


// To be defined in CLIENT
Application* CreateApplication();
}
