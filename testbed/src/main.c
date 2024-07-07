#include <core/logger.h>
#include <core/asserts.h>

int main(void) {
    PRINT_INFO("Testing Bed is up");
    PRINT_ERROR("test message %f", 3.14f);
    PRINT_INFO("current file %s", __FILE__);
    PRINT_INFO("current line %i", __LINE__);
    PRINT_INFO("size of integer %i", sizeof(f32));
    YASSERT(1==1);
    return 0;
}