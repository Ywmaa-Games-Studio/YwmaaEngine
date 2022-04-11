#include <AMRT.h>


class Sandbox : public AMRT::Application {
public:
	Sandbox(){}
	~Sandbox(){}
};


AMRT::Application* AMRT::CreateApplication() {
	return new Sandbox();
}