#pragma once
#ifdef YM_PLATFORM_WINDOWS


extern Ywmaa::Application* Ywmaa::CreateApplication();


int main(int argc, char** argv) {
	printf("Ywmaa Engine is Ready!");
	auto app = Ywmaa::CreateApplication();
	app->Run();
	delete app;
}

#endif // YM_PLATFORM_WINDOWS
