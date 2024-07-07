#include <core/logger.h>
#include <core/asserts.h>

#include <platform/platform.h>

int main(void) {
    PRINT_INFO("Testing Bed is up");
    PRINT_ERROR("test message %f", 3.14f);
    PRINT_INFO("current file %s", __FILE__);
    PRINT_INFO("current line %i", __LINE__);
    PRINT_INFO("size of integer %i", sizeof(f32));
    PRINT_WARNING("sizes of integer %i", sizeof(f32));
    YASSERT(1==1);

    PLATFORM_STATE platform_state;
    if (platform_startup(&platform_state, "Ywmaa Engine testbed", 100, 100, 1280, 720)) {
        while(TRUE) {
            platform_pump_messages(&platform_state);
        }
    }
    platform_shutdown(&platform_state);

    return 0;
}