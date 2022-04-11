#pragma once
#ifdef AMRT_PLATFORM_WINDOWS


extern AMRT::Application* AMRT::CreateApplication();


int main(int argc, char** argv) {
	printf("AMRT Engine is Ready!");
	auto app = AMRT::CreateApplication();
	app->Run();
	delete app;
}

#endif // AMRT_PLATFORM_WINDOWS
