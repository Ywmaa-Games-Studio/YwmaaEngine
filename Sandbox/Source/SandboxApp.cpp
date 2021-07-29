#include <Ywmaa.h>


class Sandbox : public Ywmaa::Application {
public:
	Sandbox(){}
	~Sandbox(){}
};


Ywmaa::Application* Ywmaa::CreateApplication() {
	return new Sandbox();
}