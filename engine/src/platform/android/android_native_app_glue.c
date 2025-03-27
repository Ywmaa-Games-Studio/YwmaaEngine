// Android platform layer.
#if defined(PLATFORM_ANDROID)

#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/looper.h>
#include <android/log.h>
#include <pthread.h>
#include <stdlib.h>

struct android_app {
    ANativeActivity* activity;
    ALooper* looper;
    ANativeWindow* window;
    AInputQueue* inputQueue;

    int destroyRequested;
    void (*onAppCmd)(struct android_app* app, int32_t cmd);
    int32_t (*onInputEvent)(struct android_app* app, AInputEvent* event);
};

static void* android_app_entry(void* param) {
    struct android_app* android_app = (struct android_app*)param;
    android_main(android_app);
    return NULL;
}

void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
    LOGI("Creating Android App");

    struct android_app* android_app = calloc(1, sizeof(struct android_app));
    activity->instance = android_app;
    android_app->activity = activity;

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &attr, android_app_entry, android_app);
    pthread_attr_destroy(&attr);
} 

#endif // PLATFORM_ANDROID 