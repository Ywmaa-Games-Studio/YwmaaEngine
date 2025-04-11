#include "core/ystring.h"
#include "core/ymemory.h"
#include "core/logger.h"

#include <stdio.h>
#include <stdarg.h>

#define USE_STD_STR 1
#if USE_STD_STR
#    include <string.h>
#endif


u64 string_length(const char* str) {
    #if USE_STD_STR
    return strlen(str);
#else
    u32 length = string_nlength(str, U32_MAX);
    if (length == U32_MAX) {
        PRINT_WARNING("string_length is returning U32_MAX. Is it possible the string has no null terminator?")
    }
    return length;
#endif
}

char* string_duplicate(const char* str) {
    if (!str) {
        PRINT_WARNING("string_duplicate called with an empty string. 0/null will be returned.");
        return 0;
    }
    u64 length = string_length(str);
    char* copy = yallocate(length + 1, MEMORY_TAG_STRING);
    ycopy_memory(copy, str, length);
    copy[length] = 0;
    return copy;
}

// Case-sensitive string comparison. True if the same, otherwise false.
b8 strings_equal(const char* str0, const char* str1) {
    return strcmp(str0, str1) == 0;
}

// Case-insensitive string comparison. True if the same, otherwise false.
b8 strings_equali(const char* str0, const char* str1) {
    #if defined(__GNUC__)
        return strcasecmp(str0, str1) == 0;
    #elif (defined _MSC_VER)
        return _strcmpi(str0, str1) == 0;
    #endif
}
    

i32 string_format(char* dest, const char* format, ...) {
    if (dest) {
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, format);
        i32 written = string_format_v(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }
    return -1;
}

i32 string_format_v(char* dest, const char* format, void* va_listp) {
    if (dest) {
        // Big, but can fit on the stack.
        char buffer[32000];
        i32 written = vsnprintf(buffer, 32000, format, va_listp);
        buffer[written] = 0;
        ycopy_memory(dest, buffer, written + 1);

        return written;
    }
    return -1;
}